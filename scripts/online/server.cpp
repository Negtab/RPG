#include "server.h"

#include <algorithm>
#include <iostream>
#include <ws2tcpip.h>

#include "networkPackets.h"
#include "SDL_log.h"

Server::Server() {}

Server::~Server() {
    stop();
}

bool Server::start(uint16_t port) {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        return false;
    }

    running = true;
    acceptThread = std::thread(&Server::acceptLoop, this);

    std::cout << "[Server] Started on port " << port << "\n";
    return true;
}

void Server::stop() {
    if (!running)
        return;

    running = false;

    closesocket(listenSocket);

    if (acceptThread.joinable())
        acceptThread.join();

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& c : clients)
        closesocket(c.socket);

    clients.clear();
    WSACleanup();

    std::cout << "[Server] Stopped\n";
}

void Server::acceptLoop() {
    while (running) {
        sockaddr_in addr{};
        int len = sizeof(addr);

        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&addr, &len);
        if (clientSocket == INVALID_SOCKET)
            continue;

        Client client{
            clientSocket,
            nextClientId++
        };

        // Отправляем новому клиенту его ID
        ConnectPacket packet{};
        packet.header.type = PacketType::Connect;
        packet.header.size = sizeof(ConnectPacket);
        packet.assignedId  = client.id;
        ::send(clientSocket, reinterpret_cast<const char*>(&packet), sizeof(packet), 0);

        // ✅ Уведомляем всех остальных что кто-то подключился
        PlayerJoinedPacket joinPacket{};
        joinPacket.header.type = PacketType::PlayerJoined;
        joinPacket.header.size = sizeof(PlayerJoinedPacket);
        joinPacket.playerId = client.id;
        broadcastBinary(clientSocket, reinterpret_cast<const char*>(&joinPacket), sizeof(joinPacket));

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
        }

        std::thread(&Server::clientLoop, this, client).detach();
    }
}

void Server::broadcastBinary(SOCKET sender, const char* data, int size) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& c : clients) {
        if (c.socket == sender)
            continue; // не отправляем обратно отправителю

        ::send(c.socket, data, size, 0);
    }
}

void Server::clientLoop(Client client) {
    char buffer[1024];

    while (running) {
        // Сначала читаем заголовок
        int received = recv(client.socket, buffer, sizeof(PacketHeader), MSG_WAITALL);
        if (received <= 0)
            break;

        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

        // Читаем остаток пакета
        int remaining = header->size - sizeof(PacketHeader);
        if (remaining > 0 && remaining < (int)sizeof(buffer) - (int)sizeof(PacketHeader)) {
            recv(client.socket, buffer + sizeof(PacketHeader), remaining, MSG_WAITALL);
        }

        // Ретранслируем всем остальным бинарно
        broadcastBinary(client.socket, buffer, header->size);
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(
            std::remove_if(
                clients.begin(),
                clients.end(),
                [&](const Client& c) { return c.id == client.id; }),
            clients.end());
    }

    DisconnectPacket packet{};
    packet.header.type = PacketType::Disconnect;
    packet.header.size = sizeof(DisconnectPacket);
    packet.playerId = client.id;
    broadcastBinary(INVALID_SOCKET, reinterpret_cast<const char*>(&packet), sizeof(packet));

    SDL_Log("[Server] Client disconnected (id=%d)", client.id);
}

void Server::processCommand(uint32_t clientId, const std::string& cmd) {
    std::cout << "[Input] Client " << clientId << ": " << cmd << "\n";

    std::string snapshot = "SNAPSHOT player=" + std::to_string(clientId) + " cmd=" + cmd;

    broadcast(snapshot);
}

void Server::handlePacket(SOCKET sender, const char* data, int size)
{
    std::lock_guard lock(clientsMutex);

    for (auto& client : clients)
    {
        if (client.socket == sender)
            continue;

        send(client.socket, data, size, 0);
    }
}

void Server::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& c : clients) {
        send(c.socket, msg.c_str(), (int)msg.size(), 0);
    }
}
