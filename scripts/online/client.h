#ifndef PROJECT_NAME_CLIENT_H
#define PROJECT_NAME_CLIENT_H

#include <winsock2.h>

#include <thread>
#include <atomic>
#include <string>
#include <mutex>

class Client {
public:
    Client();
    ~Client();

    bool connectTo(const std::string& ip, uint16_t port = 54000);
    void disconnect();

    void sendInput(const std::string& input);

private:
    void receiveLoop();
    void handleServerMessage(const std::string& msg);

private:
    SOCKET socket_ = INVALID_SOCKET;

    std::atomic<bool> connected{false};
    std::thread receiveThread;

    std::mutex sendMutex;
};

#endif //PROJECT_NAME_CLIENT_H