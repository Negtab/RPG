#ifndef PROJECT_NAME_ENEMY_H
#define PROJECT_NAME_ENEMY_H

#include "gamePerson.h"

class Enemy : public GamePerson
{
public:
    Enemy(int enemyId, int level);
    ~Enemy() override = default;

    [[nodiscard]] int getEnemyId() const noexcept { return enemyId; }
    [[nodiscard]] const std::string& getTextureName() const noexcept { return textureName; }
    [[nodiscard]] uint32_t getAttackPower(const Skill* skill) const override;
    [[nodiscard]] uint32_t getDefense() const override { return withBattleBonus(defence, battleDefence); }
    [[nodiscard]] uint32_t getSpeed() const override { return agility; }
    [[nodiscard]] std::vector<SkillInstance> getAvailableSkills() const override { return skills; }

private:
    int enemyId{};
    std::string textureName;
};

#endif