#ifndef PROJECT_NAME_TYPES_H
#define PROJECT_NAME_TYPES_H
#include <unordered_map>
#include <string>
#include <vector>

#include "../gameLogic/action.h"

constexpr int MAX_ENEMY_COUNT = 9;
constexpr int MAX_HERO_COUNT = 4;

enum class BattleAction { Attack, Magic, Item, Run };
enum class BattlePhase { PlayerTurn, EnemyTurn, EndBattle };
enum class BattleState { Start, EnemyTurn, PlayerChoose, ExecuteActions, Animation, EscapeResult, Result, WaitingForHost, Victory };
enum class TurnOwner { Player, Enemy };


enum class UIChooseState { ChooseAction, ChooseSkill, ChooseItem, ChooseTarget };

enum class Elemental { Fire, Water, Earth, Air, Ice, Lava, Dark, Light, Physical, Poison, Lightning };
enum class SpecializationNames { Archer, Magician, Warrior, Thief, None };
const std::unordered_map<SpecializationNames, std::string> specializationNamesString = {
    {SpecializationNames::Archer, "Archer"},
    {SpecializationNames::Magician, "Magician"},
    {SpecializationNames::Warrior, "Warrior"},
    {SpecializationNames::Thief, "Thief"}
};

struct Point { int32_t x{0}, y{0}; };

enum class Direction
{
    Up,
    Down,
    Left,
    Right,
    Idle
};

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
    Elemental element{Elemental::Physical};

    int32_t power{0};
};

enum class ItemEffectType
{
    Heal,
    Damage,
    Buff,
    Debuff,
    RestoreMana,
    None
};

struct Item
{
    int id{0};
    int32_t cost{0}, power{0};
    std::string name;

    bool consumable{true};
    ItemEffectType effectType{ItemEffectType::None};
    TargetType targetType{TargetType::Self};
};

struct Specialization {
    SpecializationNames name{SpecializationNames::None};
    uint32_t buffHp{0}, buffMp{0};
    explicit Specialization(const SpecializationNames name) : name(name) {}
};

struct LevelUpInfo { std::string name; int newLevel = 0; };

struct BattleReport
{
    uint32_t exp = 0;          // всего за бой
    uint32_t expEach = 0;      // каждому живому герою
    uint32_t gold = 0;         // этому игроку
    std::vector<LevelUpInfo> levelUps;
};

inline int heroLook(SpecializationNames c)        // номер картинки ImageN и анимации HeroNAnimation
{
    switch (c)
    {
    case SpecializationNames::Archer:   return 1;
    case SpecializationNames::Magician: return 2;
    case SpecializationNames::Thief:    return 3;
    case SpecializationNames::Warrior:  return 4;
    default:                            return 1;
    }
}

inline std::string sanitizeHeroName(const std::string& raw)   // только печатный ASCII, до 12 символов
{
    std::string s;
    for (char c : raw)
        if (c >= 32 && c <= 126) s += c;
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    while (!s.empty() && s.back() == ' ') s.pop_back();
    if (s.size() > 12) s.resize(12);
    return s;
}

enum class GameState { CreatePlayer, Menu, Battle, Inventory, Options, Map, Online};
const std::unordered_map<GameState, std::string> gameStateString =  {{GameState::CreatePlayer, "CreatePlayer"}, {GameState::Menu, "Menu"}, {GameState::Battle, "Battle"}, {GameState::Inventory, "Inventory"}, {GameState::Map, "Map"}, {GameState::Options, "Options"}, {GameState::Online, "Online"}};


#endif