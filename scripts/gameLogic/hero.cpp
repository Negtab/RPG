#include "hero.h"

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
        return attack + skill->power;
    return attack;
}

void Hero::addXP(const uint32_t &xp)
{
    this->experience += xp;
    if (this->experience > xpToNextLevel)
    {
        experience -= xpToNextLevel;
        this->levelUp();
    }
}
