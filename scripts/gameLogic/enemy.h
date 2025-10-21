//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_ENEMY_H
#define PROJECT_NAME_ENEMY_H
#include "gamePerson.h"

class Enemy : public GamePerson
{
public:
    explicit Enemy(EnemiesNames enemy, const class Player& player);
    ~Enemy() override = default;

    uint32_t getAttackPower(const Skill* skill) const override;
    uint32_t getDefense() const override { return defence; }
    uint32_t getSpeed() const override { return agility; }
    std::vector<int> getAvailableSkills() const override { return skills; }

private:
    EnemiesNames name{};
    uint32_t experience{0}, gold{0};
    std::vector<uint8_t> items;
};

#endif //PROJECT_NAME_ENEMY_H