
#ifndef PROJECT_NAME_SERVER_H
#define PROJECT_NAME_SERVER_H

#include <winsock2.h>

#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <future>
#include <string>

class Server {
public:
    Server();
    ~Server();

    bool start(uint16_t port = 54000);
    void stop();

private:
    struct Client {
        SOCKET socket;
        uint32_t id;
    };

    void acceptLoop();
    void clientLoop(Client client);
    void processCommand(uint32_t clientId, const std::string& cmd);
    void broadcast(const std::string& msg);
    void handlePacket(SOCKET sender, const char* data, int size);
    void broadcastBinary(SOCKET sender, const char* data, int size);

    std::promise<void> readyPromise;

    SOCKET listenSocket = INVALID_SOCKET;

    std::vector<Client> clients;
    std::mutex clientsMutex;

    std::atomic<bool> running{false};
    std::thread acceptThread;

    uint32_t nextClientId = 1;
};

#endif //PROJECT_NAME_SERVER_H