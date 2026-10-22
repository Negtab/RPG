#ifndef PROJECT_NAME_GAMEDATA_H
#define PROJECT_NAME_GAMEDATA_H

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <SDL_pixels.h>
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
    std::string name;                       // место для бестиария ("Marsh Cave"); пусто — по id
    std::vector<Formation> formations;
    int totalWeight = 0;
    std::vector<std::string> landmarks;     // порталы карты мира, у которых группа водится ("near" в JSON; near — макрос Windows)
    std::string terrain = "land";           // land, sea (корабль) или river (каноэ)
    std::string background;                 // задник боя для этой группы (подземелья), пусто — по местности
};

// Слой боевого эффекта: покадровый лист или одна картинка, которая растёт и гаснет (см. effects.json)
struct EffectLayer
{
    std::string sheet;
    int cols = 1, rows = 1, frames = 1;
    float fps = 12.0f, duration = 0.4f, delay = 0.0f;
    float scale = 1.0f, scaleEnd = -1.0f;   // scaleEnd < 0 — размер не меняется
    float angle = 0.0f;
    int dx = 0, dy = 0;
    bool add = true, fade = false;
    SDL_Color color{255, 255, 255, 255};

    [[nodiscard]] float length() const { return frames > 1 ? frames / fps : duration; }
};

struct EffectDef
{
    std::string id;
    std::vector<EffectLayer> layers;
};

struct HeroClassDef
{
    SpecializationNames id = SpecializationNames::None;
    std::string name;
    int hp = 0, mp = 0, attack = 0, defence = 0, agility = 0;
    int hpPerLevel = 0, mpPerLevel = 0, attackPerLevel = 0, defencePerLevel = 0, agilityPerLevel = 0;
    std::vector<Elemental> resists, immunities, vulnerabilities;
    std::vector<int> skills;
    std::vector<int> startEquipment;                       // id предметов, надетых в новой игре
    uint32_t xpToFirstLevel = 100;
    float xpGrowth = 1.3f;
};

struct Drop
{
    int itemId = 0;
    int chance = 0;         // процент за каждого побеждённого врага
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
    [[nodiscard]] const std::vector<Drop>& drops(int enemyId) const;
    [[nodiscard]] const EffectDef* effect(const std::string& id) const;   // nullptr, если такого нет

    [[nodiscard]] bool hasEncounter(const std::string& id) const { return encounterMap.contains(id); }
    [[nodiscard]] std::vector<int> pickFormation(const std::string& id) const;   // пусто, если группы нет
    // Группа для места без зоны Tiled: по ближайшему порталу и тому, на чём едет отряд. "" — такой нет
    [[nodiscard]] std::string groupNear(const std::string& landmark, const std::string& terrain) const;
    [[nodiscard]] std::string groupBackground(const std::string& id) const;
    // Где водится враг: названия групп встреч в порядке encounters.json, без повторов
    [[nodiscard]] const std::vector<std::string>& habitats(int enemyId) const;

private:
    void loadSkills(const std::filesystem::path& path);
    void loadItems(const std::filesystem::path& path);
    void loadEnemies(const std::filesystem::path& path);
    void loadEncounters(const std::filesystem::path& path);
    void loadHeroes(const std::filesystem::path& path);
    void loadDrops(const std::filesystem::path& path);
    void loadEffects(const std::filesystem::path& path);

    std::map<int, Item> itemMap;
    std::map<int, Skill> skillMap;
    std::map<int, EnemyDef> enemyMap;
    std::vector<int> enemyIdList;
    std::map<std::string, EncounterGroup> encounterMap;
    std::map<int, std::vector<std::string>> habitatMap;
    std::map<SpecializationNames, HeroClassDef> heroMap;
    std::map<int, std::vector<Drop>> dropMap;
    std::map<std::string, EffectDef> effectMap;
};

#endif