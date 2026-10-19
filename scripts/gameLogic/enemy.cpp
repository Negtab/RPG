#include "enemy.h"

#include <algorithm>
#include "gameData.h"

Enemy::Enemy(int id, int lvl) : enemyId(id)
{
    const EnemyDef* def = GameData::instance().enemy(id);
    if (!def) throw std::runtime_error("Unknown enemy id " + std::to_string(id));

    const int L = std::max(1, lvl);
    level = static_cast<uint8_t>(std::min(L, 255));

    name        = def->name;
    textureName = def->texture;
    maxHp       = static_cast<uint32_t>(std::max(1, def->hp.at(L)));
    currentHp   = maxHp;
    attack      = static_cast<uint32_t>(std::max(0, def->attack.at(L)));
    defence     = static_cast<uint32_t>(std::max(0, def->defence.at(L)));
    agility     = static_cast<uint32_t>(std::max(0, def->agility.at(L)));
    experience  = static_cast<uint32_t>(std::max(0, def->exp.at(L)));
    gold        = static_cast<uint32_t>(std::max(0, def->gold.at(L)));

    resists       = def->resists;
    immunities    = def->immunities;
    vulnerability = def->vulnerabilities;

    for (const auto& a : def->abilities)                       // кулдауны ведутся по умениям из списка способностей
        if (a.skillId >= 0 &&
            std::none_of(skills.begin(), skills.end(), [&](const SkillInstance& s) { return s.skillId == a.skillId; }))
            skills.push_back(SkillInstance{a.skillId, 0});

    isAlive = true;
}

uint32_t Enemy::getAttackPower(const Skill* skill) const
{
    return skill ? attack + skill->power : attack;
}