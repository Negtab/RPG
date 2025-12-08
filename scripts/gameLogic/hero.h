//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_HERO_H
#define PROJECT_NAME_HERO_H
#include "gamePerson.h"

class Hero : public GamePerson
{
public:
    explicit Hero(Specialization spec);
    ~Hero() override = default;

    void levelUp();

    uint32_t getAttackPower(const Skill* skill) const override;
    uint32_t getDefense() const override { return defence; }
    uint32_t getSpeed() const override { return agility; }
    std::vector<int> getAvailableSkills() const override { return skills; }

    void addXP(const uint32_t& xp);

    uint32_t xpToNextLevel;

    Specialization spec;
};

#endif //PROJECT_NAME_HERO_H