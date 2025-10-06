#ifndef PROJECT_NAME_GAMEPERSON_H
#define PROJECT_NAME_GAMEPERSON_H

#include "types.h"
#include <vector>
#include <map>

class GamePerson
{
public:
    GamePerson() = default;
    virtual ~GamePerson() = default;

    [[nodiscard]] uint8_t getLevel() const noexcept { return level; }
    [[nodiscard]] virtual uint32_t getAttackPower(const Skill* skill) const = 0;
    [[nodiscard]] virtual uint32_t getDefense() const = 0;
    [[nodiscard]] virtual uint32_t getSpeed() const = 0;
    [[nodiscard]] virtual std::vector<int> getAvailableSkills() const = 0;

    void killPerson() noexcept { isAlive = false; currentHp = 0; }
    void resurrectPerson(uint32_t hp = 1) noexcept { isAlive = true; currentHp = hp; }

    [[nodiscard]] uint32_t getMaxHp() const noexcept { return maxHp; }
    [[nodiscard]] uint32_t getCurrentHp() const noexcept { return currentHp; }
    void setCurrentHp(uint32_t hp) noexcept { currentHp = (hp < maxHp) ? hp : maxHp; }

    [[nodiscard]] uint32_t getMaxMp() const noexcept { return maxMp; }
    [[nodiscard]] uint32_t getCurrentMp() const noexcept { return currentMp; }
    void setCurrentMp(uint32_t mp) noexcept { currentMp = (mp < maxMp) ? mp : maxMp; }

protected:
    std::string name;
    uint32_t attack{0}, defence{0}, agility{0};
    bool isAlive{false};
    uint32_t currentHp{0}, maxHp{0};
    uint32_t currentMp{0}, maxMp{0};
    uint8_t level{1};
    std::vector<int> skills;
    std::vector<Elemental> resists, vulnerability;
    std::map<int, uint16_t> currentCooldowns;
};

class NotGamePerson
{
public:
    explicit NotGamePerson(std::string name = "", std::vector<std::string> dialogs = {});
    ~NotGamePerson() = default;

private:
    std::string name;
    std::vector<std::string> dialogs;
};

class ShopPerson : public GamePerson
{
public:
    ShopPerson() = default;
    ~ShopPerson() override = default;

    uint32_t getAttackPower(const Skill*) const override { return 0; }
    [[nodiscard]] uint32_t getDefense() const override { return 0; }
    [[nodiscard]] uint32_t getSpeed() const override { return 0; }
    [[nodiscard]] std::vector<int> getAvailableSkills() const override { return {}; }

private:
    std::map<int, int> itemKeys;
};

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

    Specialization spec;
};

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

#endif //PROJECT_NAME_GAMEPERSON_H