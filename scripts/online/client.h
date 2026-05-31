#ifndef PROJECT_NAME_CLIENT_H
#define PROJECT_NAME_CLIENT_H

#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class Client
{
public:
    Client();
    ~Client();

    bool connectTo(const std::string& ip, int port);

    bool send(const char* data, int size);
    int receive(char* buffer, int maxSize);

    void disconnect();

    bool isConnected() const;

private:
    SOCKET clientSocket = INVALID_SOCKET;
};

#endif