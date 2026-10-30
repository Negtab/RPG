#ifndef PROJECT_NAME_GAMEPERSON_H
#define PROJECT_NAME_GAMEPERSON_H

#include <algorithm>
#include <vector>
#include <map>
#include <string>

#include "../main/types.h"

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
    [[nodiscard]] virtual std::vector<SkillInstance> getAvailableSkills() const = 0;

    void killPerson() noexcept { isAlive = false; currentHp = 0; statuses = 0; }
    void resurrectPerson(uint32_t hp = 1) noexcept { isAlive = true; currentHp = hp; statuses = 0; }

    // Статусы
    [[nodiscard]] bool hasStatus(Status s) const noexcept { return statuses & statusBit(s); }
    void addStatus(Status s) noexcept { statuses |= statusBit(s); }
    void clearStatus(Status s) noexcept { statuses &= static_cast<uint16_t>(~statusBit(s)); }
    [[nodiscard]] uint16_t getStatuses() const noexcept { return statuses; }
    void setStatuses(uint16_t mask) noexcept { statuses = mask & kStatusMask; }
    // Может ли ходить: сон, паралич и камень лишают хода
    [[nodiscard]] bool canAct() const noexcept
    {
        return currentHp > 0 && !hasStatus(Status::Sleep) && !hasStatus(Status::Paralysis) && !hasStatus(Status::Stone);
    }

    [[nodiscard]] uint32_t getMaxHp() const noexcept { return maxHp; }
    void setMaxHp(uint32_t hp) noexcept { maxHp = hp; }

    [[nodiscard]] uint32_t getCurrentHp() const noexcept { return currentHp; }
    void setCurrentHp(uint32_t hp) noexcept { currentHp = hp < maxHp ? hp : maxHp; isAlive = currentHp > 0; }

    [[nodiscard]] uint32_t getMaxMp() const noexcept { return maxMp; }
    void setMaxMp(uint32_t mp) noexcept { maxMp = mp; }

    [[nodiscard]] uint32_t getCurrentMp() const noexcept { return currentMp; }
    void setCurrentMp(uint32_t mp) noexcept { currentMp = (mp < maxMp) ? mp : maxMp; }

    [[nodiscard]] uint32_t getExperience() const noexcept { return experience; }
    void setExperience(uint32_t exp) noexcept { experience = exp; }

    [[nodiscard]] uint32_t getGold() const noexcept { return gold; }
    void setGold(uint32_t newgold) noexcept { gold = newgold; }

    [[nodiscard]] bool getIsAlive() const noexcept { return isAlive; }
    void heal(uint32_t hp)
    {
        isAlive = true;
        if (maxHp <= hp + currentHp)
            currentHp = maxHp;
        else
            currentHp += hp;
    }
    void takeDamage(uint32_t damage)
    {
        if (currentHp <= damage)
        {
            currentHp = 0;
            isAlive = false;
            statuses = 0;
        }
        else
            currentHp -= damage;
        if (damage > 0) clearStatus(Status::Sleep);          // удар будит
    }
    void takeDamage(uint32_t damage, Elemental type)
    {
        const auto has = [type](const std::vector<Elemental>& list) {
            return std::find(list.begin(), list.end(), type) != list.end();
        };
        if (has(immunities))    return;                      // иммунитет: урона нет
        if (has(resists) || has(battleWards)) damage /= 2;   // сопротивление: половина
        if (has(vulnerability)) damage = damage * 3 / 2;     // уязвимость: x1.5
        takeDamage(damage);
    }

    [[nodiscard]] bool isSkillReady(int skillId) const
    {
        for (const auto& s : skills)
            if (s.skillId == skillId) return s.cooldownLeft == 0;
        return false;
    }
    void restoreMana(uint32_t mana) { currentMp = static_cast<uint32_t>(std::min<uint64_t>(maxMp, static_cast<uint64_t>(currentMp) + mana));}
    void spendMana(uint32_t mana) { currentMp < mana ? 0 : currentMp -= mana; }

    void setCooldown(int skillId, uint8_t value)
    {
        for (auto& s : skills)
            if (s.skillId == skillId)
            {
                s.cooldownLeft = value;
                return;
            }
    }
    void tickCooldowns()
    {
        for (auto& s : skills)
        {
            if (s.cooldownLeft > 0)
                --s.cooldownLeft;
        }
    }

    bool canCast(const Skill& skill) const
    {
        if (currentMp < skill.manaCost || hasStatus(Status::Silence))
            return false;

        for (const auto& s : skills)
        {
            if (s.skillId == skill.id)
                return s.cooldownLeft == 0;
        }

        return false;
    }

    void setName(std::string n) { name = std::move(n); }

    // Защита, Ярость, Ослабление: прибавка к атаке или защите до конца боя
    void addBattleBonus(BattleStat stat, int32_t amount)
    {
        switch (stat)
        {
        case BattleStat::Attack:   battleAttack += amount; break;
        case BattleStat::Defence:  battleDefence += amount; break;
        case BattleStat::Evasion:  battleEvasion += amount; break;
        case BattleStat::Accuracy: battleAccuracy += amount; break;
        }
    }
    void clearBattleBonuses() noexcept            // конец боя: остаются только яд, слепота и камень
    {
        battleAttack = battleDefence = battleEvasion = battleAccuracy = 0;
        battleWards.clear();
        statuses &= kLastingStatuses;
    }
    [[nodiscard]] int32_t getBattleBonus(BattleStat stat) const noexcept
    {
        switch (stat)
        {
        case BattleStat::Attack:   return battleAttack;
        case BattleStat::Defence:  return battleDefence;
        case BattleStat::Evasion:  return battleEvasion;
        case BattleStat::Accuracy: return battleAccuracy;
        }
        return 0;
    }
    void addWard(Elemental e) { battleWards.push_back(e); }
    void dispelResists() { resists.clear(); battleWards.clear(); }

    [[nodiscard]] const std::vector<Elemental>& getResists() const { return resists; }
    [[nodiscard]] const std::vector<Elemental>& getVulnerabilities() const { return vulnerability; }

protected:
    std::string name;
    uint32_t attack{0}, defence{0}, agility{0};
    bool isAlive{false};
    uint32_t currentHp{0}, maxHp{0};
    uint32_t currentMp{0}, maxMp{0};
    uint32_t experience{0}, gold{0};
    uint8_t level{1};
    std::vector<SkillInstance> skills;
    std::vector<Elemental> resists, immunities, vulnerability;
    std::map<int, uint16_t> currentCooldowns;
    int32_t battleAttack{0}, battleDefence{0}, battleEvasion{0}, battleAccuracy{0};
    std::vector<Elemental> battleWards;
    uint16_t statuses{0};

    [[nodiscard]] static uint32_t withBattleBonus(uint32_t value, int32_t bonus)
    {
        return static_cast<uint32_t>(std::max<int64_t>(0, static_cast<int64_t>(value) + bonus));
    }
};

#endif //PROJECT_NAME_GAMEPERSON_H