//
// Created by User on 18.05.2026.
//

#ifndef PROJECT_NAME_NETWORKPACKETS_H
#define PROJECT_NAME_NETWORKPACKETS_H

#include <cstdint>

enum class PacketType : uint8_t
{
    Connect = 0,
    PlayerMove = 1,
    PlayerJoined = 2,
    Disconnect   = 3,
    BattleStart  = 4,  // кто-то начал бой — рассылаем врагов
    BattleAction = 5,  // игрок отправил действие
    BattleSnapshot = 6, // сервер рассылает состояние боя
    BattleEnd = 7,
    MapChange = 8,
    PartyInfo = 9
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

struct PlayerMovePacket {
    PacketHeader header;
    uint32_t playerId;
    uint16_t mapId;
    int16_t  cellX, cellY, targetX, targetY;
    uint8_t  direction;
    uint8_t  moving;
};

struct EnemyData
{
    uint8_t  enemyId;
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

struct MapChangePacket
{
    PacketHeader header;
    uint16_t mapId;
    int16_t  cellX, cellY;
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

struct HeroInfo
{
    uint8_t classId;      // SpecializationNames как uint8
    uint8_t level;
    int32_t currentHp;
    int32_t currentMp;
    char    name[13];
};

struct PartyInfoPacket
{
    PacketHeader header;
    uint32_t playerId;
    uint8_t  heroCount;   // максимум 4
    HeroInfo heroes[4];
};

#pragma pack(pop)


constexpr uint32_t kMaxPacket = 512;

inline uint32_t expectedSize(PacketType t) {
    switch (t) {
    case PacketType::Connect:        return sizeof(ConnectPacket);
    case PacketType::PlayerMove:     return sizeof(PlayerMovePacket);
    case PacketType::PlayerJoined:   return sizeof(PlayerJoinedPacket);
    case PacketType::Disconnect:     return sizeof(DisconnectPacket);
    case PacketType::BattleStart:    return sizeof(BattleStartPacket);
    case PacketType::BattleAction:   return sizeof(BattleActionPacket);
    case PacketType::BattleSnapshot: return sizeof(BattleSnapshotPacket);
    case PacketType::BattleEnd:      return sizeof(BattleEndPacket);
    case PacketType::MapChange:      return sizeof(MapChangePacket);
    case PacketType::PartyInfo:      return sizeof(PartyInfoPacket);
    }
    return 0;   // неизвестный тип
}
static_assert(sizeof(PacketHeader) == 5, "pack(1) сломан");
static_assert(sizeof(ConnectPacket)        <= kMaxPacket);
static_assert(sizeof(PlayerMovePacket)     <= kMaxPacket);
static_assert(sizeof(PlayerJoinedPacket)   <= kMaxPacket);
static_assert(sizeof(DisconnectPacket)     <= kMaxPacket);
static_assert(sizeof(BattleStartPacket)    <= kMaxPacket);
static_assert(sizeof(BattleActionPacket)   <= kMaxPacket);
static_assert(sizeof(BattleSnapshotPacket) <= kMaxPacket);
static_assert(sizeof(BattleEndPacket)      <= kMaxPacket);
static_assert(sizeof(MapChangePacket)      <= kMaxPacket);
static_assert(sizeof(PartyInfoPacket) <= kMaxPacket);

#endif //PROJECT_NAME_NETWORKPACKETS_H