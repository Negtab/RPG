#include "gameData.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;
using std::runtime_error;

namespace {

const std::vector<std::pair<std::string, Elemental>> kElementals = {
    {"Fire", Elemental::Fire}, {"Water", Elemental::Water}, {"Earth", Elemental::Earth},
    {"Air", Elemental::Air}, {"Ice", Elemental::Ice}, {"Lava", Elemental::Lava},
    {"Dark", Elemental::Dark}, {"Light", Elemental::Light}, {"None", Elemental::Physical},
    {"Physical", Elemental::Physical}, {"Poison", Elemental::Poison}, {"Lightning", Elemental::Lightning}};

const std::vector<std::pair<std::string, TargetType>> kTargets = {
    {"Enemy", TargetType::Enemy}, {"Ally", TargetType::Ally}, {"Self", TargetType::Self},
    {"AllEnemies", TargetType::AllEnemies}, {"AllAllies", TargetType::AllAllies}};

const std::vector<std::pair<std::string, SkillEffectType>> kSkillEffects = {
    {"Damage", SkillEffectType::Damage}, {"Heal", SkillEffectType::Heal}, {"Buff", SkillEffectType::Buff},
    {"Debuff", SkillEffectType::Debuff}, {"None", SkillEffectType::None}};

const std::vector<std::pair<std::string, ItemEffectType>> kItemEffects = {
    {"Heal", ItemEffectType::Heal}, {"Damage", ItemEffectType::Damage}, {"Buff", ItemEffectType::Buff},
    {"Debuff", ItemEffectType::Debuff}, {"RestoreMana", ItemEffectType::RestoreMana}, {"None", ItemEffectType::None}};

const std::vector<std::pair<std::string, EquipSlot>> kSlots = {
    {"Weapon", EquipSlot::Weapon}, {"Shield", EquipSlot::Shield}, {"Head", EquipSlot::Head},
    {"Body", EquipSlot::Body}, {"Arms", EquipSlot::Arms}};

const std::vector<std::pair<std::string, SpecializationNames>> kClasses = {
    {"Archer", SpecializationNames::Archer}, {"Magician", SpecializationNames::Magician},
    {"Warrior", SpecializationNames::Warrior}, {"Thief", SpecializationNames::Thief}};

template <class E>
E parseEnum(const std::string& s, const std::vector<std::pair<std::string, E>>& table, const char* field)
{
    for (const auto& [name, value] : table)
        if (name == s) return value;

    std::string options;
    for (const auto& entry : table) options += (options.empty() ? "" : ", ") + entry.first;
    throw runtime_error("unknown value '" + s + "' in '" + field + "' (allowed: " + options + ")");
}

template <class E>
E getEnum(const json& j, const char* key, const std::vector<std::pair<std::string, E>>& table)
{
    return parseEnum(j.at(key).get<std::string>(), table, key);
}

std::vector<Elemental> getElementals(const json& j, const char* key)
{
    std::vector<Elemental> out;
    if (!j.contains(key)) return out;
    for (const auto& v : j.at(key))
        out.push_back(parseEnum(v.get<std::string>(), kElementals, key));
    return out;
}

int getInt(const json& j, const char* key, int lo, int hi)
{
    const int v = j.at(key).get<int>();
    if (v < lo || v > hi)
        throw runtime_error(std::string("'") + key + "' = " + std::to_string(v) +
                            " is out of range " + std::to_string(lo) + ".." + std::to_string(hi));
    return v;
}

int getIntOr(const json& j, const char* key, int lo, int hi, int def)
{
    return j.contains(key) ? getInt(j, key, lo, hi) : def;
}

Growth getGrowth(const json& j, const char* key)
{
    const auto& a = j.at(key);
    if (!a.is_array() || a.size() != 2)
        throw runtime_error(std::string("'") + key + "' must be [base, perLevel]");
    return { a[0].get<int>(), a[1].get<int>() };
}

std::vector<int> getIdList(const json& j, const char* key, const std::map<int, Skill>& skills)
{
    std::vector<int> out;
    if (!j.contains(key)) return out;
    for (const auto& v : j.at(key))
    {
        const int id = v.get<int>();
        if (!skills.contains(id)) throw runtime_error("unknown skill id " + std::to_string(id) + " in '" + key + "'");
        out.push_back(id);
    }
    return out;
}

json readJson(const fs::path& path)
{
    std::ifstream f(path);
    if (!f) throw runtime_error("cannot open " + path.string());
    try { return json::parse(f, nullptr, true, true); }        // комментарии // и /* */ разрешены
    catch (const json::parse_error& e) { throw runtime_error(path.filename().string() + ": " + e.what()); }
}

template <class Fn>
void forEachEntry(const fs::path& path, Fn&& fn)
{
    const json root = readJson(path);
    if (!root.is_array()) throw runtime_error(path.filename().string() + ": expected a JSON array");

    int index = 0;
    for (const auto& entry : root)
    {
        try { fn(entry); }
        catch (const std::exception& e)                        // json::exception и runtime_error
        {
            throw runtime_error(path.filename().string() + ", entry #" + std::to_string(index) + ": " + e.what());
        }
        ++index;
    }
}

} // namespace

GameData& GameData::instance()
{
    static GameData data;
    return data;
}

void GameData::loadAll(const fs::path& dir)
{
    itemMap.clear(); skillMap.clear(); enemyMap.clear(); enemyIdList.clear(); heroMap.clear(); encounterMap.clear();

    loadSkills(dir / "skills.json");
    loadItems(dir / "items.json");
    loadEnemies(dir / "enemies.json");
    loadEncounters(dir / "encounters.json");     // после врагов: формации ссылаются на их id
    loadHeroes(dir / "heroes.json");
    loadDrops(dir / "drops.json");
    loadEffects(dir / "effects.json");
}

void GameData::loadSkills(const fs::path& path)
{
    forEachEntry(path, [&](const json& e) {
        Skill s;
        s.id          = getInt(e, "id", 1, 100000);
        s.name        = e.at("name").get<std::string>();
        s.manaCost    = static_cast<uint32_t>(getInt(e, "manaCost", 0, 100000));
        s.maxCooldown = static_cast<uint8_t>(getInt(e, "maxCooldown", 0, 255));
        s.targetType  = getEnum(e, "targetType", kTargets);
        s.targetCount = static_cast<uint8_t>(getIntOr(e, "targetCount", 0, 255, 1));
        s.effectType  = getEnum(e, "effectType", kSkillEffects);
        s.element     = e.contains("element") ? getEnum(e, "element", kElementals) : Elemental::Physical;
        s.power       = getInt(e, "power", 0, 1000000);
        s.isDamaging  = s.effectType == SkillEffectType::Damage;
        s.effect      = e.value("effect", std::string{});
        if (!skillMap.emplace(s.id, s).second)
            throw runtime_error("duplicate skill id " + std::to_string(s.id));
    });
}

void GameData::loadItems(const fs::path& path)
{
    forEachEntry(path, [&](const json& e) {
        Item it;
        it.id         = getInt(e, "id", 1, 100000);
        it.name       = e.at("name").get<std::string>();
        it.cost       = getInt(e, "cost", 0, 1000000);
        it.power      = getIntOr(e, "power", 0, 1000000, 0);      // у экипировки силы эффекта нет
        it.consumable = e.value("consumable", true);
        it.effectType = e.contains("effectType") ? getEnum(e, "effectType", kItemEffects) : ItemEffectType::None;
        it.targetType = e.contains("targetType") ? getEnum(e, "targetType", kTargets) : TargetType::Self;
        if (e.contains("slot"))                                  // экипировка
        {
            it.slot       = getEnum(e, "slot", kSlots);
            it.consumable = false;
            it.attack     = getIntOr(e, "attack", -1000, 100000, 0);
            it.defence    = getIntOr(e, "defence", -1000, 100000, 0);
            it.agility    = getIntOr(e, "agility", -1000, 100000, 0);
            if (e.contains("classes"))
                for (const auto& c : e.at("classes"))
                    it.classes.push_back(parseEnum(c.get<std::string>(), kClasses, "classes"));
        }
        it.startCount = getIntOr(e, "start", 0, 99, 0);
        it.effect     = e.value("effect", std::string{});
        it.hitEffect  = e.value("hitEffect", std::string{});
        it.swing      = getIntOr(e, "swing", -1, 999, -1);
        if (!itemMap.emplace(it.id, it).second)
            throw runtime_error("duplicate item id " + std::to_string(it.id));
    });
}

void GameData::loadEnemies(const fs::path& path)
{
    forEachEntry(path, [&](const json& e) {
        EnemyDef d;
        d.id      = getInt(e, "id", 0, 255);                       // уходит в сеть как uint8_t
        d.name    = e.at("name").get<std::string>();
        d.texture = e.value("texture", d.name);

        const auto& r = e.at("srcRect");
        if (!r.is_array() || r.size() != 4) throw runtime_error("'srcRect' must be [x, y, w, h]");
        d.srcRect = { r[0].get<int>(), r[1].get<int>(), r[2].get<int>(), r[3].get<int>() };

        d.hp = getGrowth(e, "hp");           d.attack = getGrowth(e, "attack");
        d.defence = getGrowth(e, "defence"); d.agility = getGrowth(e, "agility");
        d.exp = getGrowth(e, "exp");         d.gold = getGrowth(e, "gold");
        if (d.hp.at(1) < 1) throw runtime_error("'hp' must be positive");

        d.hits            = getIntOr(e, "hits", 1, 9, 1);
        d.resists         = getElementals(e, "resists");
        d.immunities      = getElementals(e, "immunities");
        d.vulnerabilities = getElementals(e, "vulnerabilities");

        if (e.contains("abilities"))
            for (const auto& a : e.at("abilities"))
            {
                EnemyAbility ab;
                ab.weight = getInt(a, "weight", 1, 1000);
                if (a.contains("skill"))
                {
                    ab.skillId = getInt(a, "skill", 1, 100000);
                    if (!skillMap.contains(ab.skillId))
                        throw runtime_error("unknown skill id " + std::to_string(ab.skillId) + " in 'abilities'");
                }
                d.abilities.push_back(ab);
            }
        if (d.abilities.empty()) d.abilities.push_back({-1, 1});    // по умолчанию только удар

        if (!enemyMap.emplace(d.id, d).second)
            throw runtime_error("duplicate enemy id " + std::to_string(d.id));
        enemyIdList.push_back(d.id);
    });

    if (enemyIdList.empty()) throw runtime_error(path.filename().string() + ": no enemies defined");
}

void GameData::loadEncounters(const fs::path& path)
{
    if (!fs::exists(path)) return;                                  // файл необязателен

    forEachEntry(path, [&](const json& e) {
        EncounterGroup g;
        g.id = e.at("id").get<std::string>();

        for (const auto& f : e.at("formations"))
        {
            Formation fm;
            fm.weight = getInt(f, "weight", 1, 1000);
            for (const auto& v : f.at("enemies"))
            {
                const int id = v.get<int>();
                if (!enemyMap.contains(id)) throw runtime_error("unknown enemy id " + std::to_string(id));
                fm.enemies.push_back(id);
            }
            if (fm.enemies.empty() || fm.enemies.size() > MAX_ENEMY_COUNT)
                throw runtime_error("a formation must have 1.." + std::to_string(MAX_ENEMY_COUNT) + " enemies");
            g.totalWeight += fm.weight;
            g.formations.push_back(std::move(fm));
        }
        if (g.formations.empty()) throw runtime_error("group '" + g.id + "' has no formations");
        if (e.contains("near"))
            for (const auto& n : e.at("near")) g.landmarks.push_back(n.get<std::string>());
        g.terrain = e.value("terrain", std::string("land"));
        g.background = e.value("background", std::string{});

        if (!encounterMap.emplace(g.id, g).second)
            throw runtime_error("duplicate encounter group '" + g.id + "'");
    });
}

std::vector<int> GameData::pickFormation(const std::string& id) const
{
    const auto it = encounterMap.find(id);
    if (it == encounterMap.end() || it->second.totalWeight <= 0) return {};

    int roll = rand() % it->second.totalWeight;
    for (const Formation& f : it->second.formations)
    {
        if (roll < f.weight) return f.enemies;
        roll -= f.weight;
    }
    return {};
}

std::string GameData::groupBackground(const std::string& id) const
{
    const auto it = encounterMap.find(id);
    return it != encounterMap.end() ? it->second.background : std::string{};
}

std::string GameData::groupNear(const std::string& landmark, const std::string& terrain) const
{
    for (const auto& [id, g] : encounterMap)
        if (g.terrain == terrain && std::find(g.landmarks.begin(), g.landmarks.end(), landmark) != g.landmarks.end())
            return id;
    return {};
}

void GameData::loadHeroes(const fs::path& path)
{
    forEachEntry(path, [&](const json& e) {
        HeroClassDef c;
        c.name = e.at("class").get<std::string>();
        c.id   = parseEnum(c.name, kClasses, "class");

        const auto& b = e.at("base");
        const auto& p = e.at("perLevel");
        c.hp = getInt(b, "hp", 1, 1000000);      c.mp = getInt(b, "mp", 0, 1000000);
        c.attack = getInt(b, "attack", 0, 100000); c.defence = getInt(b, "defence", 0, 100000);
        c.agility = getInt(b, "agility", 0, 100000);
        c.hpPerLevel = getInt(p, "hp", 0, 100000);       c.mpPerLevel = getInt(p, "mp", 0, 100000);
        c.attackPerLevel = getInt(p, "attack", 0, 100000); c.defencePerLevel = getInt(p, "defence", 0, 100000);
        c.agilityPerLevel = getInt(p, "agility", 0, 100000);

        c.resists         = getElementals(e, "resists");
        c.vulnerabilities = getElementals(e, "vulnerabilities");
        c.skills          = getIdList(e, "skills", skillMap);
        if (e.contains("startEquipment"))
            for (const auto& v : e.at("startEquipment"))
            {
                const int id = v.get<int>();
                const auto it = itemMap.find(id);
                if (it == itemMap.end() || !it->second.canEquip(c.id))
                    throw runtime_error("item " + std::to_string(id) + " in 'startEquipment' is not equipment for " + c.name);
                c.startEquipment.push_back(id);
            }

        c.xpToFirstLevel = static_cast<uint32_t>(getIntOr(e, "xpToFirstLevel", 1, 1000000000, 100));
        c.xpGrowth       = e.value("xpGrowth", 1.3f);
        if (c.xpGrowth < 1.0f || c.xpGrowth > 10.0f)             // меньше 1: уровни дешевеют, а при 0 цикл addXP не кончается
            throw runtime_error("'xpGrowth' must be between 1 and 10");

        if (!heroMap.emplace(c.id, c).second)
            throw runtime_error("duplicate hero class " + c.name);
    });

    for (const auto& [name, id] : kClasses)
        if (!heroMap.contains(id))
            throw runtime_error(path.filename().string() + ": class '" + name + "' is missing");
}

void GameData::loadDrops(const fs::path& path)
{
    dropMap.clear();
    if (!fs::exists(path)) return;                               // трофеи необязательны
    forEachEntry(path, [&](const json& e) {
        const int enemyId = getInt(e, "enemy", 0, 255);
        if (!enemyMap.contains(enemyId)) throw runtime_error("unknown enemy id " + std::to_string(enemyId));
        auto& list = dropMap[enemyId];
        for (const auto& d : e.at("drops"))
        {
            Drop drop{getInt(d, "item", 1, 100000), getInt(d, "chance", 1, 100)};
            if (!itemMap.contains(drop.itemId)) throw runtime_error("unknown item id " + std::to_string(drop.itemId));
            list.push_back(drop);
        }
    });
}

void GameData::loadEffects(const fs::path& path)
{
    effectMap.clear();
    if (!fs::exists(path)) return;                               // без эффектов бой просто тише
    forEachEntry(path, [&](const json& e) {
        EffectDef d;
        d.id = e.at("id").get<std::string>();
        for (const auto& l : e.at("layers"))
        {
            EffectLayer L;
            L.sheet    = l.at("sheet").get<std::string>();
            L.cols     = getIntOr(l, "cols", 1, 64, 1);
            L.rows     = getIntOr(l, "rows", 1, 64, 1);
            L.frames   = getIntOr(l, "frames", 1, L.cols * L.rows, 1);
            L.fps      = l.value("fps", 12.0f);
            L.duration = l.value("duration", 0.4f);
            L.delay    = l.value("delay", 0.0f);
            L.scale    = l.value("scale", 1.0f);
            L.scaleEnd = l.value("scaleEnd", -1.0f);
            L.angle    = l.value("angle", 0.0f);
            L.dx       = l.value("dx", 0);
            L.dy       = l.value("dy", 0);
            L.add      = l.value("add", true);
            L.fade     = l.value("fade", false);
            if (l.contains("color"))
            {
                const auto& c = l.at("color");
                if (!c.is_array() || c.size() != 3) throw runtime_error("'color' must be [r, g, b]");
                L.color = {static_cast<Uint8>(c[0].get<int>()), static_cast<Uint8>(c[1].get<int>()),
                           static_cast<Uint8>(c[2].get<int>()), 255};
            }
            if (L.fps <= 0.0f || L.duration <= 0.0f) throw runtime_error("'fps' and 'duration' must be positive");
            d.layers.push_back(std::move(L));
        }
        if (!effectMap.emplace(d.id, d).second) throw runtime_error("duplicate effect '" + d.id + "'");
    });
}

const EffectDef* GameData::effect(const std::string& id) const
{
    const auto it = effectMap.find(id);
    return it != effectMap.end() ? &it->second : nullptr;
}

const std::vector<Drop>& GameData::drops(int enemyId) const
{
    static const std::vector<Drop> none;
    const auto it = dropMap.find(enemyId);
    return it != dropMap.end() ? it->second : none;
}

const EnemyDef* GameData::enemy(int id) const
{
    const auto it = enemyMap.find(id);
    return it != enemyMap.end() ? &it->second : nullptr;
}

const HeroClassDef* GameData::heroClass(SpecializationNames id) const
{
    const auto it = heroMap.find(id);
    return it != heroMap.end() ? &it->second : nullptr;
}