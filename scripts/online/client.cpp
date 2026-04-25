#include "client.h"

#include <iostream>
#include <ws2tcpip.h>
Client::Client() {}

Client::~Client() {
    disconnect();
}

bool Client::connectTo(const std::string& ip, uint16_t port) {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[Client] WSAStartup failed\n";
        return false;
    }

    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        std::cerr << "[Client] Socket creation failed\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "[Client] Invalid IP address\n";
        return false;
    }

    if (connect(socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[Client] Connection failed\n";
        return false;
    }

    connected = true;
    receiveThread = std::thread(&Client::receiveLoop, this);

    std::cout << "[Client] Connected to " << ip << ":" << port << "\n";
    return true;
}

void Client::disconnect() {
    if (!connected)
        return;

    connected = false;

    closesocket(socket_);

    if (receiveThread.joinable())
        receiveThread.join();

    WSACleanup();
    std::cout << "[Client] Disconnected\n";
}

void Client::sendInput(const std::string& input) {
    if (!connected)
        return;

    std::lock_guard<std::mutex> lock(sendMutex);
    send(socket_, input.c_str(), (int)input.size(), 0);
}

void Client::receiveLoop() {
    char buffer[1024];

    while (connected) {
        int received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0)
            break;

        buffer[received] = '\0';
        handleServerMessage(buffer);
    }

    connected = false;
}

void Client::handleServerMessage(const std::string& msg) {
    std::cout << "[Server] " << msg << "\n";

    // Примеры будущей логики:
    //
    // if (msg.starts_with("SNAPSHOT")) {
    //     applySnapshot(msg);
    // }
    //
    // if (msg.starts_with("MODE TURN_BASED")) {
    //     enterTurnBased();
    // }
}
