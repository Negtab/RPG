#include "server.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <ws2tcpip.h>

#include "networkPackets.h"
#include "SDL_log.h"

namespace {
// Если у игрока до 4 героев, а в BattleSnapshotPacket места на 8, то реально хватит двух игроков:
// поставь 2 или увеличь массивы в снапшоте.
constexpr size_t kMaxPlayers = 4;

bool readExact(SOCKET s, char* dst, int n) {
    while (n > 0) {
        int r = recv(s, dst, n, 0);
        if (r <= 0) return false;
        dst += r; n -= r;
    }
    return true;
}
}

Server::Server() = default;
Server::~Server() { stop(); }

bool Server::start(uint16_t port) {
    if (running) return false;

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        SDL_Log("[Server] WSAStartup failed");
        return false;
    }

    auto fail = [&](const char* what) {
        const int err = WSAGetLastError();
        SDL_Log("[Server] %s (error %d)", what, err);
        if (listenSocket != INVALID_SOCKET) { closesocket(listenSocket); listenSocket = INVALID_SOCKET; }
        WSACleanup();
        return false;
    };

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) return fail("socket() failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        return fail("bind() failed (порт занят?)");
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        return fail("listen() failed");

    nextClientId = 1;
    hostId = 0;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        lastMove.clear();
    }

    running = true;
    acceptThread = std::thread(&Server::acceptLoop, this);
    SDL_Log("[Server] Started on port %d", port);
    return true;
}

void Server::stop() {
    if (!running.exchange(false)) return;

    if (listenSocket != INVALID_SOCKET) {              // будит accept()
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }
    if (acceptThread.joinable()) acceptThread.join();  // после этого workers больше никто не меняет

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& c : clients) shutdown(c.socket, SD_BOTH);   // будит recv() в clientLoop
    }
    for (auto& t : workers)                            // join БЕЗ мьютекса: потоки берут его при выходе
        if (t.joinable()) t.join();
    workers.clear();

    WSACleanup();
    SDL_Log("[Server] Stopped");
}

void Server::acceptLoop() {
    while (running) {
        SOCKET s = accept(listenSocket, nullptr, nullptr);
        if (s == INVALID_SOCKET) {
            if (!running) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));   // не крутим CPU при ошибке
            continue;
        }

        {   // лимит игроков
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (clients.size() >= kMaxPlayers) {
                SDL_Log("[Server] rejected connection: server full");
                closesocket(s);
                continue;
            }
        }

        const DWORD sendTimeoutMs = 1000;   // зависший клиент не должен замораживать рассылку
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeoutMs), sizeof(sendTimeoutMs));
        const int on = 1;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

        Client client{ s, nextClientId++ };

        ConnectPacket hello{};
        hello.header.type = PacketType::Connect;
        hello.header.size = sizeof(ConnectPacket);
        hello.assignedId  = client.id;
        if (::send(s, reinterpret_cast<const char*>(&hello), sizeof(hello), 0) != sizeof(hello)) {
            closesocket(s);
            continue;
        }

        if (hostId == 0) hostId = client.id;   // первый подключившийся = хост (он коннектится сразу после start)

        PlayerJoinedPacket joined{};
        joined.header.type = PacketType::PlayerJoined;
        joined.header.size = sizeof(PlayerJoinedPacket);
        joined.playerId = client.id;
        broadcastBinary(s, reinterpret_cast<const char*>(&joined), sizeof(joined));

        {   // новичок получает последние позиции стоящих игроков и попадает в список одним действием
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (const auto& c : clients)                      // кто уже в игре
            {
                PlayerJoinedPacket pj{};
                pj.header.type = PacketType::PlayerJoined;
                pj.header.size = sizeof(PlayerJoinedPacket);
                pj.playerId = c.id;
                ::send(s, reinterpret_cast<const char*>(&pj), sizeof(pj), 0);
            }
            for (const auto& [id, mv] : lastMove)
                ::send(s, reinterpret_cast<const char*>(&mv), sizeof(mv), 0);
            clients.push_back(client);
        }

        workers.emplace_back(&Server::clientLoop, this, client);
        SDL_Log("[Server] Client connected (id=%u)", client.id);
    }
}

void Server::broadcastBinary(SOCKET sender, const char* data, int size) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& c : clients) {
        if (c.socket == sender) continue;
        if (::send(c.socket, data, size, 0) != size)
            shutdown(c.socket, SD_BOTH);       // его clientLoop увидит ошибку и сам уберёт клиента
    }
}

void Server::clientLoop(Client client) {
    char buf[kMaxPacket];

    while (running) {
        if (!readExact(client.socket, buf, sizeof(PacketHeader))) break;
        auto* h = reinterpret_cast<PacketHeader*>(buf);

        const uint32_t expected = expectedSize(h->type);
        if (expected == 0 || h->size != expected) {
            SDL_Log("[Server] bad packet (type=%d size=%u) from %u, disconnecting",
                    (int)h->type, h->size, client.id);
            break;
        }
        if (!readExact(client.socket, buf + sizeof(PacketHeader), h->size - sizeof(PacketHeader))) break;

        // пакет прочитан целиком, поэтому continue ниже не ломает границы потока
        switch (h->type) {
            case PacketType::PlayerMove: {
                auto* p = reinterpret_cast<PlayerMovePacket*>(buf);
                p->playerId = client.id;                       // чужому id не верим
                std::lock_guard<std::mutex> lock(clientsMutex);
                lastMove[client.id] = *p;
                break;
            }
            case PacketType::BattleAction:
                reinterpret_cast<BattleActionPacket*>(buf)->playerId = client.id;
                break;

            case PacketType::BattleStart:
            case PacketType::BattleSnapshot:
            case PacketType::BattleEnd:
                if (client.id != hostId) {
                    SDL_Log("[Server] client %u sent host-only packet %d, dropped", client.id, (int)h->type);
                    continue;
                }
                break;
            case PacketType::MapChange:
                if (client.id != hostId) { continue; }
                break;

            default:   // Connect / PlayerJoined / Disconnect шлёт только сервер
                SDL_Log("[Server] client %u sent server-only packet %d, dropped", client.id, (int)h->type);
                continue;
        }

        broadcastBinary(client.socket, buf, h->size);
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::erase_if(clients, [&](const Client& c) { return c.id == client.id; });
        lastMove.erase(client.id);
    }
    closesocket(client.socket);   // раньше сокет нигде не закрывался: утечка дескриптора

    if (running) {
        DisconnectPacket packet{};
        packet.header.type = PacketType::Disconnect;
        packet.header.size = sizeof(DisconnectPacket);
        packet.playerId = client.id;
        broadcastBinary(INVALID_SOCKET, reinterpret_cast<const char*>(&packet), sizeof(packet));
    }
    SDL_Log("[Server] Client disconnected (id=%u)", client.id);
}