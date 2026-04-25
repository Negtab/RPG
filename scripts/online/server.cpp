#include "server.h"

#include <algorithm>
#include <iostream>
#include <ws2tcpip.h>

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

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
        }

        std::cout << "[Server] Client connected (id=" << client.id << ")\n";

        std::thread(&Server::clientLoop, this, client).detach();
    }
}

void Server::clientLoop(Client client) {
    char buffer[1024];

    while (running) {
        int received = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0)
            break;

        buffer[received] = '\0';
        processCommand(client.id, buffer);
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

    closesocket(client.socket);
    std::cout << "[Server] Client disconnected (id=" << client.id << ")\n";
}


void Server::processCommand(uint32_t clientId, const std::string& cmd) {
    std::cout << "[Input] Client " << clientId << ": " << cmd << "\n";

    // ❗ Здесь ДОЛЖНА быть:
    // - валидация
    // - очередь команд
    // - симуляция
    // - snapshot

    // Пока просто ретрансляция
    std::string snapshot = "SNAPSHOT player=" + std::to_string(clientId) + " cmd=" + cmd;

    broadcast(snapshot);
}

void Server::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& c : clients) {
        send(c.socket, msg.c_str(), (int)msg.size(), 0);
    }
}
