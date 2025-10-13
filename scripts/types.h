#ifndef PROJECT_NAME_TYPES_H
#define PROJECT_NAME_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

enum class GameState { CreatePlayer, Menu, Battle, Inventory, Instruction, Map };
enum class BattleAction { Attack, Magic, Item, Run };
enum class BattleState { SelectAction, SelectTarget, SelectSkill, SelectItem, Animation, Result };
enum class BattlePhase { PlayerTurn, EnemyTurn, EndBattle };
enum class Elemental { Fire, Water, Earth, Air, Ice, Lava, Dark, Light, None };
enum class SpecializationNames { Archer, Magician, Warrior, Thief, None };
enum class EnemiesNames {};

struct Point { int32_t x{0}, y{0}; };
struct Location { Point coord{}; std::string name; int32_t h{0}, w{0}; bool isSafe{false}; };
struct Skill { int id{0}; std::string name; uint32_t damage{0}, manaCost{0}; uint8_t cooldown{0}, aimCount{0}; bool isDamaging{true}; Elemental damageType{Elemental::None}; };
struct Specialization { SpecializationNames name{SpecializationNames::None}; uint32_t buffXp{0}, buffCurrentMp{0}; };

inline std::vector<std::string> gameStateString {"CreatePlayer", "Menu", "Battle", "Inventory", "Map", "Instruction", "None"};

inline const std::string& gameStateToString(const GameState state) {
    switch (state) {
        case GameState::CreatePlayer: return gameStateString.at(0);
        case GameState::Menu: return gameStateString.at(1);
        case GameState::Battle: return gameStateString.at(2);
        case GameState::Inventory: return gameStateString.at(3);
        case GameState::Map: return gameStateString.at(4);
        case GameState::Instruction: return gameStateString.at(5);
        default: return gameStateString.at(6);
    }
}

#endif