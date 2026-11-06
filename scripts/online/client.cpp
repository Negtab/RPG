#include "client.h"

#include <ws2tcpip.h>
#include <mstcpip.h>

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

bool Client::connectTo(const std::string& ip, int port, int timeoutMs)
{
    disconnect();

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
        return false;                                       // кривой IP

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
        return false;

    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);              // сразу неблокирующий: connect с таймаутом через select

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR &&
        WSAGetLastError() != WSAEWOULDBLOCK)
    {
        disconnect();
        return false;
    }

    fd_set writable, failed;
    FD_ZERO(&writable); FD_ZERO(&failed);
    FD_SET(clientSocket, &writable);
    FD_SET(clientSocket, &failed);
    timeval tv{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };

    if (select(0, nullptr, &writable, &failed, &tv) <= 0 || FD_ISSET(clientSocket, &failed))
    {
        disconnect();                                       // таймаут или отказ
        return false;
    }

    int soError = 0;
    int len = sizeof(soError);
    getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &len);
    if (soError != 0)
    {
        disconnect();
        return false;
    }

    const int on = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    // Keepalive: обрыв без закрытия (выдернули кабель, завис хост) обнаружится примерно за 15 секунд
    tcp_keepalive ka{1, 5000, 1000};
    DWORD ret = 0;
    WSAIoctl(clientSocket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &ret, nullptr, nullptr);

    return true;
}

bool Client::send(const char* data, int size)
{
    if (clientSocket == INVALID_SOCKET || size <= 0)
        return false;

    if (outbox.size() + static_cast<size_t>(size) > kMaxOutbox)
    {
        broken = true;                                      // flush() вернёт false, Game закроет соединение
        return false;
    }

    outbox.insert(outbox.end(), data, data + size);
    return flush();
}

bool Client::flush()
{
    if (clientSocket == INVALID_SOCKET || broken)
        return false;

    while (!outbox.empty())
    {
        const int n = ::send(clientSocket, outbox.data(), static_cast<int>(outbox.size()), 0);
        if (n == SOCKET_ERROR)
            return WSAGetLastError() == WSAEWOULDBLOCK;     // буфер полон: доотправим в следующем кадре
        outbox.erase(outbox.begin(), outbox.begin() + n);
    }
    return true;
}

RecvResult Client::receive(char* buffer, int maxSize)
{
    if (clientSocket == INVALID_SOCKET)
        return {RecvStatus::Closed};

    const int n = recv(clientSocket, buffer, maxSize, 0);

    if (n > 0)  return {RecvStatus::Data, n};
    if (n == 0) return {RecvStatus::Closed};                // собеседник закрыл соединение

    return WSAGetLastError() == WSAEWOULDBLOCK ? RecvResult{RecvStatus::NoData}
                                               : RecvResult{RecvStatus::Error};
}

void Client::disconnect()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    outbox.clear();
    broken = false;
}

bool Client::isConnected() const
{
    return clientSocket != INVALID_SOCKET;
}