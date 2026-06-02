//
// Created by User on 18.05.2026.
//

#ifndef PROJECT_NAME_NETWORKPACKETS_H
#define PROJECT_NAME_NETWORKPACKETS_H

#include "../main/types.h"

enum class PacketType : uint8_t
{
    Connect = 0,
    PlayerMove = 1,
    PlayerJoined = 2,
    Disconnect   = 3,
    BattleStart  = 4,  // кто-то начал бой — рассылаем врагов
    BattleAction = 5,  // игрок отправил действие
    BattleSnapshot = 6 // сервер рассылает состояние боя
};

#pragma pack(push, 1)

struct PacketHeader
{
    PacketType type;
    uint32_t size;
};

struct ConnectPacket
{
    PacketHeader header;
    uint32_t assignedId;
};

struct PlayerJoinedPacket
{
    PacketHeader header;
    uint32_t playerId; // ID подключившегося игрока
};

struct DisconnectPacket
{
    PacketHeader header;
    uint32_t playerId;
};

struct PlayerMovePacket
{
    PacketHeader header;

    uint32_t playerId;

    int32_t x;
    int32_t y;

    Direction direction;
};


#pragma pack(pop)

#endif //PROJECT_NAME_NETWORKPACKETS_H