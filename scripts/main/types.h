#ifndef PROJECT_NAME_TYPES_H
#define PROJECT_NAME_TYPES_H
#include <unordered_map>
#include <string>

#include "SDL_rect.h"
#include "../gameLogic/action.h"

constexpr int MAX_ENEMY_COUNT = 9;
constexpr int MAX_HERO_COUNT = 4;

enum class BattleAction { Attack, Magic, Item, Run };
enum class BattlePhase { PlayerTurn, EnemyTurn, EndBattle };
enum class BattleState { Start, EnemyTurn, PlayerChoose, ExecuteActions, Animation, EscapeResult, Result, WaitingForHost };
enum class TurnOwner { Player, Enemy };


enum class UIChooseState { ChooseAction, ChooseSkill, ChooseItem, ChooseTarget };

enum class Elemental { Fire, Water, Earth, Air, Ice, Lava, Dark, Light, None, Physical, Poison, Lightning };
enum class SpecializationNames { Archer, Magician, Warrior, Thief, None };
const std::unordered_map<SpecializationNames, std::string> specializationNamesString = {
    {SpecializationNames::Archer, "Archer"},
    {SpecializationNames::Magician, "Magician"},
    {SpecializationNames::Warrior, "Warrior"},
    {SpecializationNames::Thief, "Thief"}
};

struct Point { int32_t x{0}, y{0}; };
struct Location { Point coord{}; std::string name; int32_t h{0}, w{0}; bool isSafe{false}; };

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
    Elemental element{Elemental::None};

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
    int8_t cost{0}, power{0};
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

enum class EnemyName {Ghost, Knight, Ogr, Gargoyle};
const std::unordered_map<EnemyName, SDL_Rect> enemiesSize = {
    {EnemyName::Ghost, {0,0,84,94},},
    {EnemyName::Gargoyle, {0,0,108,144}},
    {EnemyName::Knight, {0,0,108,123},},
    {EnemyName::Ogr, {0,0,165,174},}
};
const std::unordered_map<EnemyName, std::string> enemyNameString = {
    {EnemyName::Ghost, "Ghost"},
    {EnemyName::Knight, "Knight"},
    {EnemyName::Ogr, "Ogr"},
    {EnemyName::Gargoyle, "Gargoyle"}
};


enum class GameState { CreatePlayer, Menu, Battle, Inventory, Options, Map, Online};
const std::unordered_map<GameState, std::string> gameStateString =  {{GameState::CreatePlayer, "CreatePlayer"}, {GameState::Menu, "Menu"}, {GameState::Battle, "Battle"}, {GameState::Inventory, "Inventory"}, {GameState::Map, "Map"}, {GameState::Options, "Options"}, {GameState::Online, "Online"}};

#endif