#ifndef PROJECT_NAME_SERVER_H
#define PROJECT_NAME_SERVER_H

#include <winsock2.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "networkPackets.h"

class Server {
public:
    Server();
    ~Server();

    bool start(uint16_t port = 54000);
    void stop();

    [[nodiscard]] bool isRunning() const { return running; }

private:
    struct Client {
        SOCKET socket;
        uint32_t id;
    };

    void acceptLoop();
    void clientLoop(Client client);
    void broadcastBinary(SOCKET sender, const char* data, int size);

    SOCKET listenSocket = INVALID_SOCKET;

    std::vector<Client> clients;                              // всё ниже, где не сказано иное, под clientsMutex
    std::unordered_map<uint32_t, PlayerMovePacket> lastMove;  // последняя позиция каждого игрока, для новичков
    std::unordered_map<uint32_t, PartyInfoPacket> lastParty;
    std::mutex clientsMutex;

    std::atomic<bool> running{false};
    std::atomic<uint32_t> nextClientId{1};   // 0 в Game значит «id не выдан», поэтому с 1
    std::atomic<uint32_t> hostId{0};         // первый подключившийся клиент

    std::thread acceptThread;
    std::vector<std::thread> workers;        // трогают только acceptLoop и stop() (после join accept-потока)
};

#endif //PROJECT_NAME_SERVER_H