#include "enemy.h"

Enemy::Enemy(EnemiesNames enemy, const Player&) : name(enemy)
{
    resurrectPerson();
}

uint32_t Enemy::getAttackPower(const Skill* skill) const
{
    return skill ? attack + skill->damage : attack;
}

