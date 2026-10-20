#include "hero.h"

#include <stdexcept>
#include <algorithm>
#include "gameData.h"

Hero::Hero(Specialization spec) : spec(spec)
{
    const HeroClassDef* def = GameData::instance().heroClass(spec.name);
    if (!def)
        throw std::runtime_error("No hero class data for specialization " + std::to_string(static_cast<int>(spec.name)));

    name = def->name;
    level = 1;
    experience = 0;
    xpToNextLevel = def->xpToFirstLevel;

    attack  = static_cast<uint32_t>(def->attack);
    defence = static_cast<uint32_t>(def->defence);
    agility = static_cast<uint32_t>(def->agility);
    maxHp   = static_cast<uint32_t>(def->hp);
    maxMp   = static_cast<uint32_t>(def->mp);

    resists       = def->resists;
    vulnerability = def->vulnerabilities;
    skills.clear();
    for (int id : def->skills)
        skills.push_back(SkillInstance{id, 0});

    currentHp = maxHp;
    currentMp = maxMp;
    isAlive = true;
}

void Hero::levelUp()
{
    const HeroClassDef* def = GameData::instance().heroClass(spec.name);
    if (!def) return;

    ++level;
    attack  += static_cast<uint32_t>(def->attackPerLevel);
    defence += static_cast<uint32_t>(def->defencePerLevel);
    agility += static_cast<uint32_t>(def->agilityPerLevel);
    maxHp   += static_cast<uint32_t>(def->hpPerLevel);
    maxMp   += static_cast<uint32_t>(def->mpPerLevel);

    currentHp = maxHp;
    currentMp = maxMp;

    xpToNextLevel = static_cast<uint32_t>(xpToNextLevel * def->xpGrowth);
}

void Hero::setLevel(int target)
{
    target = std::clamp(target, 1, 99);
    while (level < target)
        levelUp();                           // levelUp сам добавляет статы класса и восстанавливает HP/MP
}

int32_t Hero::equipBonus(int32_t Item::*stat) const
{
    int32_t sum = 0;
    const auto& items = GameData::instance().items();
    for (int id : equipment)
        if (const auto it = items.find(id); id != 0 && it != items.end())
            sum += it->second.*stat;
    return sum;
}

namespace {
    uint32_t withBonus(uint32_t base, int32_t bonus)       // бонус бывает отрицательным (тяжёлая броня)
    {
        return static_cast<uint32_t>(std::max<int64_t>(0, static_cast<int64_t>(base) + bonus));
    }
}

uint32_t Hero::getAttackPower(const Skill* skill) const
{
    const uint32_t total = withBonus(attack, equipBonus(&Item::attack));
    if (skill && skill->isDamaging)
        return total + skill->power;
    return total;
}

uint32_t Hero::getDefense() const { return withBonus(defence, equipBonus(&Item::defence)); }
uint32_t Hero::getSpeed() const   { return std::max<uint32_t>(1, withBonus(agility, equipBonus(&Item::agility))); }

int Hero::getEquipped(EquipSlot slot) const
{
    const int i = static_cast<int>(slot);
    return i >= 0 && i < kEquipSlots ? equipment[i] : 0;
}

int Hero::equip(EquipSlot slot, int itemId)
{
    const int i = static_cast<int>(slot);
    if (i < 0 || i >= kEquipSlots) return 0;
    if (itemId != 0)                                       // чужой слот или класс: не надеваем
    {
        const auto& items = GameData::instance().items();
        const auto it = items.find(itemId);
        if (it == items.end() || it->second.slot != slot || !it->second.canEquip(spec.name)) return 0;
    }
    const int old = equipment[i];
    equipment[i] = itemId;
    return old;
}

void Hero::equipStarting()
{
    if (const HeroClassDef* def = GameData::instance().heroClass(spec.name))
        for (int id : def->startEquipment)
            if (const auto it = GameData::instance().items().find(id); it != GameData::instance().items().end())
                equip(it->second.slot, id);
}

void Hero::addXP(const uint32_t& xp)
{
    experience += xp;
    while (experience >= xpToNextLevel)
    {
        experience -= xpToNextLevel;
        levelUp();
    }
}