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
    BattleSnapshot = 6, // сервер рассылает состояние боя
    BattleEnd = 7
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

struct EnemyData
{
    uint8_t  enemyName;   // EnemyName as uint8
    int32_t  currentHp;
    int32_t  maxHp;
};

struct BattleStartPacket
{
    PacketHeader header;
    uint8_t  enemyCount;
    EnemyData enemies[4]; // максимум 4 врага
};

struct BattleActionPacket
{
    PacketHeader header;
    uint32_t playerId;
    uint8_t  actionType;   // ActionType as uint8
    uint8_t  targetType;   // TargetType as uint8
    int32_t  actorIndex;
    int32_t  targetIndex;
    int32_t  payloadId;    // skillId или itemId
};

// Состояние одного героя/врага после действия
struct CharacterState
{
    int32_t currentHp;
    int32_t currentMana;
};

struct BattleSnapshotPacket
{
    PacketHeader header;
    uint8_t  heroCount;
    uint8_t  enemyCount;
    CharacterState heroes[8];
    uint32_t heroOwner[8];  // ✅ ID владельца каждого героя
    CharacterState enemies[4];
    uint8_t  battleState;
    uint8_t  currentHeroIndex;
    uint8_t currentTurn;
    uint8_t currentActionIndex;
};

struct BattleEndPacket
{
    PacketHeader header;
    uint8_t isWin; // 1 = победа, 0 = побег/поражение
};

#pragma pack(pop)

#endif //PROJECT_NAME_NETWORKPACKETS_H