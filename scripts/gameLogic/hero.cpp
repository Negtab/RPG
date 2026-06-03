#include "hero.h"

Hero::Hero(Specialization spec) : spec(spec)
{
    level = 1;
    experience = 0;
    xpToNextLevel = 100;

    switch (spec.name)
    {
        case SpecializationNames::Warrior:  makeBasicKnight();   break;
        case SpecializationNames::Magician: makeBasicMagician(); break;
        case SpecializationNames::Archer:   makeBasicArcher();   break;
        case SpecializationNames::Thief:    makeBasicThief();    break;
        default: break;
    }

    name = specializationNamesString.at(spec.name);

    currentHp = maxHp;
    currentMp = maxMp;
    isAlive = true;
}

void Hero::makeBasicKnight()
{
    spec.name   = SpecializationNames::Warrior;
    spec.buffHp = 30;
    spec.buffMp = 0;

    attack   = 12;
    defence  = 18;
    agility  = 6;

    maxHp = 150 + spec.buffHp;
    maxMp = 30  + spec.buffMp;

    resists = { Elemental::Physical };
    vulnerability.clear();
    skills = {{1,0}, {5, 0}};
}


void Hero::makeBasicMagician()
{
    spec.name   = SpecializationNames::Magician;
    spec.buffHp = 0;
    spec.buffMp = 40;

    attack   = 22;
    defence  = 6;
    agility  = 8;

    maxHp = 85  + spec.buffHp;
    maxMp = 140 + spec.buffMp;

    resists = { Elemental::Fire };
    vulnerability = { Elemental::Physical };
    skills = {{2,0}, {4,0}};
}

void Hero::makeBasicArcher()
{
    spec.name   = SpecializationNames::Archer;
    spec.buffHp = 10;
    spec.buffMp = 10;

    attack   = 16;
    defence  = 9;
    agility  = 16;

    maxHp = 110 + spec.buffHp;
    maxMp = 60  + spec.buffMp;

    resists = { Elemental::Air };
    vulnerability.clear();
    skills = {{3,0}};
}

void Hero::makeBasicThief()
{
    spec.name   = SpecializationNames::Thief;
    spec.buffHp = 5;
    spec.buffMp = 5;

    attack   = 15;
    defence  = 7;
    agility  = 20;

    maxHp = 95 + spec.buffHp;
    maxMp = 50 + spec.buffMp;

    resists = { Elemental::Poison };
    vulnerability = { Elemental::Fire };
    skills = {{6,0}};
}

void Hero::levelUp()
{
    ++level;

    switch (spec.name)
    {
        case SpecializationNames::Warrior:
            attack  += 2;
            defence += 3;
            agility += 1;

            maxHp += 15 + spec.buffHp;
            maxMp += 2  + spec.buffMp;
            break;

        case SpecializationNames::Magician:
            attack  += 4;
            defence += 1;
            agility += 1;

            maxHp += 6  + spec.buffHp;
            maxMp += 15 + spec.buffMp;
            break;

        case SpecializationNames::Archer:
            attack  += 3;
            defence += 2;
            agility += 3;

            maxHp += 10 + spec.buffHp;
            maxMp += 6  + spec.buffMp;
            break;

        case SpecializationNames::Thief:
            attack  += 3;
            defence += 1;
            agility += 4;

            maxHp += 8  + spec.buffHp;
            maxMp += 4  + spec.buffMp;
            break;

        default:
            break;
    }

    currentHp = maxHp;
    currentMp = maxMp;

    xpToNextLevel = static_cast<uint32_t>(xpToNextLevel * 1.3f);
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
