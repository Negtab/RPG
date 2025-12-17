#ifndef PROJECT_NAME_TYPES_H
#define PROJECT_NAME_TYPES_H

#include <cstdint>
#include <string>
#include "gameLogic/action.h"

constexpr int MAX_ENEMY_COUNT = 9;
constexpr int MAX_HERO_COUNT = 4;

enum class BattleAction { Attack, Magic, Item, Run };
enum class BattleState
{
    Start,
    EnemyTurn,
    PlayerChoose,
    ExecuteActions,
    Animation,
    EscapeResult,
    Result
};
enum class UIChooseState
{
    ChooseAction,   // Attack / Magic / Item / Skip
    ChooseSkill,    // список скиллов
    ChooseItem,     // инвентарь
    ChooseTarget    // враг / союзник / AoE
};

enum class BattlePhase { PlayerTurn, EnemyTurn, EndBattle };
enum class Elemental { Fire, Water, Earth, Air, Ice, Lava, Dark, Light, None };
enum class SpecializationNames { Archer, Magician, Warrior, Thief, None };

struct Point { int32_t x{0}, y{0}; };
struct Location { Point coord{}; std::string name; int32_t h{0}, w{0}; bool isSafe{false}; };

struct SkillInstance
{
    int skillId{0};
    uint8_t cooldownLeft{0};
};

enum class SkillEffectType
{
    Damage,
    Heal,
    Buff,
    Debuff,
    None
};

struct Skill
{
    int id{0};
    bool isDamaging{false};
    std::string name;

    uint32_t manaCost{0};
    uint8_t maxCooldown{0};

    uint8_t targetCount{1};
    TargetType targetType{TargetType::Enemy};

    SkillEffectType effectType{SkillEffectType::Damage};
    Elemental element{Elemental::None};

    int32_t power{0};
};
struct Specialization { SpecializationNames name{SpecializationNames::None}; uint32_t buffXp{0}, buffCurrentMp{0}; };

enum class EnemyName {Ghost, Knight, Ogr, Gargoyle};
const std::string EnemyNameString[] = {"Ghost", "Knight", "Ogr", "Gargoyle", "None"};
inline const std::string& enemyNameToString(const EnemyName name)
{
    switch (name)
    {
        case EnemyName::Ghost: return EnemyNameString[0];
        case EnemyName::Knight: return EnemyNameString[1];
        case EnemyName::Ogr: return EnemyNameString[2];
        case EnemyName::Gargoyle: return EnemyNameString[3];
        default: return EnemyNameString[4];
    }
}

enum class GameState { CreatePlayer, Menu, Battle, Inventory, Instruction, Map };
const std::string GameStateString[] =  {"CreatePlayer", "Menu", "Battle", "Inventory", "Map", "Instruction", "None"};
inline const std::string& gameStateToString(const GameState state)
{
    switch (state)
    {
        case GameState::CreatePlayer: return GameStateString[0];
        case GameState::Menu: return GameStateString[1];
        case GameState::Battle: return GameStateString[2];
        case GameState::Inventory: return GameStateString[3];
        case GameState::Map: return GameStateString[4];
        case GameState::Instruction: return GameStateString[5];
        default: return GameStateString[6];
    }
}
#endif