#ifndef PROJECT_NAME_GAMEPERSON_H
#define PROJECT_NAME_GAMEPERSON_H

#include "../types.h"
#include <vector>
#include <map>

class GamePerson
{
public:
    GamePerson() = default;
    virtual ~GamePerson() = default;

    [[nodiscard]] std::string getName() const { return name; }
    [[nodiscard]] uint8_t getLevel() const noexcept { return level; }
    [[nodiscard]] virtual uint32_t getAttackPower(const Skill* skill) const = 0;
    [[nodiscard]] virtual uint32_t getDefense() const = 0;
    [[nodiscard]] virtual uint32_t getSpeed() const = 0;
    [[nodiscard]] virtual std::vector<int> getAvailableSkills() const = 0;

    void killPerson() noexcept { isAlive = false; currentHp = 0; }
    void resurrectPerson(uint32_t hp = 1) noexcept { isAlive = true; currentHp = hp; }

    [[nodiscard]] uint32_t getMaxHp() const noexcept { return maxHp; }
    void setMaxHp(uint32_t hp) noexcept { maxHp = hp; }

    [[nodiscard]] uint32_t getCurrentHp() const noexcept { return currentHp; }
    void setCurrentHp(uint32_t hp) noexcept { currentHp = (hp < maxHp) ? hp : maxHp; }


    [[nodiscard]] uint32_t getMaxMp() const noexcept { return maxMp; }
    void setMaxMp(uint32_t mp) noexcept { maxMp = mp; }

    [[nodiscard]] uint32_t getCurrentMp() const noexcept { return currentMp; }
    void setCurrentMp(uint32_t mp) noexcept { currentMp = (mp < maxMp) ? mp : maxMp; }

    [[nodiscard]] uint32_t getExperience() const noexcept { return experience; }
    void setExperience(uint32_t exp) noexcept { experience = exp; }

    [[nodiscard]] uint32_t getGold() const noexcept { return gold; }
    void setGold(uint32_t gold) noexcept { gold = gold; }

protected:
    std::string name;
    uint32_t attack{0}, defence{0}, agility{0};
    bool isAlive{false};
    uint32_t currentHp{0}, maxHp{0};
    uint32_t currentMp{0}, maxMp{0};
    uint32_t experience{0}, gold{0};
    uint8_t level{1};
    std::vector<int> skills;
    std::vector<Elemental> resists, vulnerability;
    std::map<int, uint16_t> currentCooldowns;
};

#endif //PROJECT_NAME_GAMEPERSON_H