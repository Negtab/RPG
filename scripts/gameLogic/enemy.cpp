#include "enemy.h"

#include <utility>

Enemy::Enemy(std::string  enemy, const Player&) : name(std::move(enemy))
{
    resurrectPerson();
}

uint32_t Enemy::getAttackPower(const Skill* skill) const
{
    return skill ? attack + skill->damage : attack;
}

