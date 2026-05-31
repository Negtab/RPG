#include "client.h"

#include <ws2tcpip.h>

Client::Client()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

Client::~Client()
{
    disconnect();
    WSACleanup();
}

bool Client::connectTo(const std::string& ip, int port)
{
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    // ✅ Переводим в неблокирующий режим
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    return true;
}

bool Client::send(const char* data, int size)
{
    if (clientSocket == INVALID_SOCKET)
        return false;

    int result = ::send(clientSocket, data, size, 0);

    return result != SOCKET_ERROR;
}

int Client::receive(char* buffer, int maxSize)
{
    if (clientSocket == INVALID_SOCKET)
        return 0;

    int result = recv(clientSocket, buffer, maxSize, 0);

    if (result == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
            return 0; // ← данных нет, это нормально — не блокируем
        return -1;    // реальная ошибка
    }

    return result;
}

void Client::disconnect()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
}

bool Client::isConnected() const
{
    return clientSocket != INVALID_SOCKET;
}