//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_ENEMY_H
#define PROJECT_NAME_ENEMY_H
#include <memory>
#include "../player.h"
#include "gamePerson.h"

class Enemy : public GamePerson
{
public:
    explicit Enemy(EnemyName name, Player *player);
    ~Enemy() override = default;

    [[nodiscard]] uint32_t getAttackPower(const Skill* skill) const override;
    [[nodiscard]] uint32_t getDefense() const override { return defence; }
    [[nodiscard]] uint32_t getSpeed() const override { return agility; }
    [[nodiscard]] std::vector<SkillInstance> getAvailableSkills() const override { return skills; }

private:
    void makeGhost();
    void makeKnight();
    void makeOgr();
    void makeGargoyle();
    Player *player;
    EnemyName enemyName{};

    std::vector<uint8_t> items;
};

#endif //PROJECT_NAME_ENEMY_H