#include "gamePerson.h"

NotGamePerson::NotGamePerson(std::string name, std::vector<std::string> dialogs) : name(std::move(name)), dialogs(std::move(dialogs)) {}

Hero::Hero(Specialization spec) : spec(spec) {}

void Hero::levelUp()
{
    ++level;
    maxHp += 10;
    maxMp += 5;
    attack += 2;
    defence += 1;
    agility += 1;
    resurrectPerson(maxHp);
}

uint32_t Hero::getAttackPower(const Skill* skill) const
{
    if (skill && skill->isDamaging)
        return attack + skill->damage;
    return attack;
}

Enemy::Enemy(EnemiesNames enemy, const Player&) : name(enemy)
{
    resurrectPerson();
}

uint32_t Enemy::getAttackPower(const Skill* skill) const
{
    return skill ? attack + skill->damage : attack;
}

