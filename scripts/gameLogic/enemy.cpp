#include "enemy.h"

Enemy::Enemy(const EnemyName name, Player *player) : player(player), enemyName(name)
{
    switch (name)
    {
        case EnemyName::Gargoyle: makeGargoyle(); break;
        case EnemyName::Knight:   makeKnight(); break;
        case EnemyName::Ogr:      makeOgr(); break;
        case EnemyName::Ghost:    makeGhost(); break;
    }
    this->name = enemyNameToString(name);
}


void Enemy::makeGargoyle()
{
    const int lvl = player->getAverageLevel();
    level = lvl;

    maxHp = 40 + lvl * 12;
    currentHp = maxHp;

    attack   = 10 + lvl * 3;
    defence  = 6 + lvl * 2;
    agility  = 4 + lvl * 1;

    experience = 15 + lvl * 5;
    gold       = 5 + lvl * 2;

    skills = {0}; // базовый удар
}

void Enemy::makeGhost()
{
    const int lvl = player->getAverageLevel();
    level = lvl;

    maxHp = 25 + lvl * 8;
    currentHp = maxHp;

    attack   = 8 + lvl * 2;
    defence  = 4 + lvl * 1;
    agility  = 10 + lvl * 3;

    experience = 12 + lvl * 5;
    gold = 3 + lvl * 2;

    skills = {1}; // яд/магия
}


void Enemy::makeKnight() {
    //
}

void Enemy::makeOgr() {
    //
}


uint32_t Enemy::getAttackPower(const Skill* skill) const
{
    return skill ? attack + skill->damage : attack;
}

