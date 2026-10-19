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

uint32_t Hero::getAttackPower(const Skill* skill) const
{
    if (skill && skill->isDamaging)
        return attack + skill->power;
    return attack;
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