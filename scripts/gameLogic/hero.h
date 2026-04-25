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
    [[nodiscard]] uint32_t getDefense() const override { return defence; }
    [[nodiscard]] uint32_t getSpeed() const override { return agility; }
    [[nodiscard]] std::vector<SkillInstance> getAvailableSkills() const override { return skills; }

    void addXP(const uint32_t& xp);

private:
    void makeBasicKnight();
    void makeBasicMagician();
    void makeBasicArcher();
    void makeBasicThief();

    uint32_t xpToNextLevel;
    Specialization spec;
};

#endif //PROJECT_NAME_HERO_H