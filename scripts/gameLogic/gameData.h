#ifndef PROJECT_NAME_GAMEDATA_H
#define PROJECT_NAME_GAMEDATA_H

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <SDL_rect.h>

#include "../main/types.h"

struct Growth
{
    int base = 0, perLevel = 0;
    [[nodiscard]] int at(int level) const { return base + perLevel * level; }
};

struct EnemyAbility
{
    int skillId = -1;       // -1: обычная атака
    int weight = 1;
};

struct EnemyDef
{
    int id = 0;
    std::string name;       // как показывается в бою
    std::string texture;    // имя PNG без расширения
    SDL_Rect srcRect{0, 0, 0, 0};
    Growth hp, attack, defence, agility, exp, gold;
    int hits = 1;           // ударов обычной атакой за ход
    std::vector<Elemental> resists, immunities, vulnerabilities;
    std::vector<EnemyAbility> abilities;
};

struct Formation
{
    int weight = 1;
    std::vector<int> enemies;
};

struct EncounterGroup
{
    std::string id;
    std::vector<Formation> formations;
    int totalWeight = 0;
};

struct HeroClassDef
{
    SpecializationNames id = SpecializationNames::None;
    std::string name;
    int hp = 0, mp = 0, attack = 0, defence = 0, agility = 0;
    int hpPerLevel = 0, mpPerLevel = 0, attackPerLevel = 0, defencePerLevel = 0, agilityPerLevel = 0;
    std::vector<Elemental> resists, immunities, vulnerabilities;
    std::vector<int> skills;
    uint32_t xpToFirstLevel = 100;
    float xpGrowth = 1.3f;
};

class GameData
{
public:
    static GameData& instance();

    void loadAll(const std::filesystem::path& dir);

    [[nodiscard]] const std::map<int, Item>& items() const { return itemMap; }
    [[nodiscard]] const std::map<int, Skill>& skills() const { return skillMap; }
    [[nodiscard]] const EnemyDef* enemy(int id) const;
    [[nodiscard]] const std::vector<int>& enemyIds() const { return enemyIdList; }
    [[nodiscard]] const HeroClassDef* heroClass(SpecializationNames id) const;

    [[nodiscard]] bool hasEncounter(const std::string& id) const { return encounterMap.contains(id); }
    [[nodiscard]] std::vector<int> pickFormation(const std::string& id) const;   // пусто, если группы нет

private:
    void loadSkills(const std::filesystem::path& path);
    void loadItems(const std::filesystem::path& path);
    void loadEnemies(const std::filesystem::path& path);
    void loadEncounters(const std::filesystem::path& path);
    void loadHeroes(const std::filesystem::path& path);

    std::map<int, Item> itemMap;
    std::map<int, Skill> skillMap;
    std::map<int, EnemyDef> enemyMap;
    std::vector<int> enemyIdList;
    std::map<std::string, EncounterGroup> encounterMap;
    std::map<SpecializationNames, HeroClassDef> heroMap;
};

#endif