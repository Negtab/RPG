//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_HERO_H
#define PROJECT_NAME_HERO_H
#include <cstdint>

#include "gamePerson.h"
#include "../main/types.h"

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

    [[nodiscard]] SpecializationNames getClass() const { return spec.name; }
    void setLevel(int target);               // повышает до уровня target (понижать нельзя)

    void addXP(const uint32_t& xp);


private:
    uint32_t xpToNextLevel;
    Specialization spec;
};

#endif //PROJECT_NAME_HERO_H