//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_ENEMY_H
#define PROJECT_NAME_ENEMY_H
#include "gamePerson.h"

class Enemy : public GamePerson
{
public:
    explicit Enemy(std::string  name, const class Player& player);
    ~Enemy() override = default;

    [[nodiscard]] uint32_t getAttackPower(const Skill* skill) const override;
    [[nodiscard]] uint32_t getDefense() const override { return defence; }
    [[nodiscard]] uint32_t getSpeed() const override { return agility; }
    [[nodiscard]] std::vector<int> getAvailableSkills() const override { return skills; }

private:
    std::string name{};
    uint32_t experience{0}, gold{0};
    std::vector<uint8_t> items;
};

#endif //PROJECT_NAME_ENEMY_H