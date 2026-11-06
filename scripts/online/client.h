#ifndef PROJECT_NAME_CLIENT_H
#define PROJECT_NAME_CLIENT_H

#include <winsock2.h>
#include <string>
#include <vector>

enum class RecvStatus { Data, NoData, Closed, Error };

struct RecvResult {
    RecvStatus status;
    int bytes = 0;
};

class Client
{
public:
    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // Блокирует вызывающий поток не дольше timeoutMs (пока без отдельного потока)
    bool connectTo(const std::string& ip, int port, int timeoutMs = 3000);

    bool send(const char* data, int size);   // кладёт в очередь и пробует отправить
    bool flush();                            // дослать остаток; false = соединение сломано
    RecvResult receive(char* buffer, int maxSize);

    void disconnect();
    [[nodiscard]] bool isConnected() const;

private:
    static constexpr size_t kMaxOutbox = 1u << 20;   // 1 MiB: если столько не уходит, собеседник не читает

    SOCKET clientSocket = INVALID_SOCKET;
    std::vector<char> outbox;
    bool broken = false;
};

#endif