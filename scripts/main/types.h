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

inline const char* elementName(Elemental e)
{
    static const char* names[] = {"Fire", "Water", "Earth", "Air", "Ice", "Lava", "Dark", "Light",
                                  "Physical", "Poison", "Lightning"};
    return names[static_cast<int>(e)];
}

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
    std::string effect;                                     // боевой эффект из effects.json; пусто — по стихии и типу
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

// Слоты экипировки как в FF1: оружие, щит, голова, тело, руки
enum class EquipSlot { Weapon, Shield, Head, Body, Arms, None };
constexpr int kEquipSlots = 5;
inline const char* equipSlotName(EquipSlot s)
{
    switch (s)
    {
    case EquipSlot::Weapon: return "Weapon";
    case EquipSlot::Shield: return "Shield";
    case EquipSlot::Head:   return "Head";
    case EquipSlot::Body:   return "Body";
    case EquipSlot::Arms:   return "Arms";
    default:                return "";
    }
}

struct Item
{
    int id{0};
    int32_t cost{0}, power{0};
    std::string name;

    bool consumable{true};
    ItemEffectType effectType{ItemEffectType::None};
    TargetType targetType{TargetType::Self};

    // Экипировка: slot != None. Бонусы складываются со статами героя
    EquipSlot slot{EquipSlot::None};
    int32_t attack{0}, defence{0}, agility{0};
    std::vector<SpecializationNames> classes;              // кто может надеть; пусто — все
    int startCount{0};                                     // сколько выдаётся в новой игре
    std::string effect;                                    // боевой эффект при применении (effects.json)
    std::string hitEffect;                                 // оружие: эффект попадания по врагу
    int swing{-1};                                         // оружие: замах SwingNN.png, -1 — без замаха (лук)

    [[nodiscard]] bool isEquipment() const { return slot != EquipSlot::None; }
    [[nodiscard]] bool canEquip(SpecializationNames c) const
    {
        if (!isEquipment()) return false;
        if (classes.empty()) return true;
        for (auto k : classes) if (k == c) return true;
        return false;
    }
    // Можно применить вне боя: лечение и мана на своего героя
    [[nodiscard]] bool usableInField() const
    {
        return !isEquipment() && (effectType == ItemEffectType::Heal || effectType == ItemEffectType::RestoreMana);
    }
    [[nodiscard]] bool usableInBattle() const { return !isEquipment() && effectType != ItemEffectType::None; }
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
    std::vector<std::string> items;   // трофеи, уже с количеством: "Health Potion x2"
};

constexpr int kHeroFrame = 32;                  // кадр героя в бою: 32x32 (спрайты Pixel Remaster)

// Набор спрайтов FF1 для класса игры: Walk<набор>.png на карте, Battle<набор>.png в бою
inline const char* heroSpriteSet(SpecializationNames c)
{
    switch (c)
    {
    case SpecializationNames::Warrior:  return "Warrior";
    case SpecializationNames::Thief:    return "Thief";
    case SpecializationNames::Archer:   return "RedMage";
    case SpecializationNames::Magician: return "BlackMage";
    default:                            return "Warrior";
    }
}

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

// Транспорт на карте мира. Порядок совпадает с индексом в Player::vehicles (минус 1)
enum class Vehicle : uint8_t { None, Canoe, Ship, Airship };
constexpr int kVehicleKinds = 3;
inline int vehicleIndex(Vehicle v) { return static_cast<int>(v) - 1; }
inline const char* vehicleName(Vehicle v)          // совпадает с именем PNG и маркера в Tiled (в нижнем регистре)
{
    switch (v)
    {
    case Vehicle::Canoe:   return "Canoe";
    case Vehicle::Ship:    return "Ship";
    case Vehicle::Airship: return "Airship";
    default:               return "";
    }
}

struct VehicleState                                 // где стоит транспорт, пока на нём не едут
{
    bool      available{false};                     // есть на карте (маркер или запасное место нашлось)
    uint16_t  mapId{0};
    Point     cell{};
    Direction dir{Direction::Down};
};

enum class GameState { CreatePlayer, Menu, Battle, Inventory, Options, Map, Online, Saves, GameMenu, Shop };
const std::unordered_map<GameState, std::string> gameStateString =  {{GameState::CreatePlayer, "CreatePlayer"}, {GameState::Menu, "Menu"}, {GameState::Battle, "Battle"}, {GameState::Inventory, "Inventory"}, {GameState::Map, "Map"}, {GameState::Options, "Options"}, {GameState::Online, "Online"}, {GameState::Saves, "Saves"}, {GameState::GameMenu, "GameMenu"}, {GameState::Shop, "Shop"}};


#endif