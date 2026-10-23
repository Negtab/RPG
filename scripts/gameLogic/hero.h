//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_HERO_H
#define PROJECT_NAME_HERO_H
#include <array>
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
    [[nodiscard]] uint32_t getDefense() const override;
    [[nodiscard]] uint32_t getSpeed() const override;

    // Статы без экипировки: для экрана Status и сравнения в Equipment
    [[nodiscard]] uint32_t getBaseAttack()  const { return attack; }
    [[nodiscard]] uint32_t getBaseDefense() const { return defence; }
    [[nodiscard]] uint32_t getBaseSpeed()   const { return agility; }
    [[nodiscard]] uint32_t getXpToNextLevel() const { return xpToNextLevel; }

    // Экипировка: id предмета в слоте, 0 — пусто. equip возвращает снятый предмет (0, если слот был пуст)
    [[nodiscard]] int getEquipped(EquipSlot slot) const;
    int equip(EquipSlot slot, int itemId);
    void equipStarting();                    // стартовый набор класса из heroes.json
    [[nodiscard]] const std::array<int, kEquipSlots>& getEquipment() const { return equipment; }
    [[nodiscard]] std::vector<SkillInstance> getAvailableSkills() const override { return skills; }

    [[nodiscard]] SpecializationNames getClass() const { return spec.name; }
    void setLevel(int target);               // повышает до уровня target (понижать нельзя)

    void addXP(const uint32_t& xp);


private:
    [[nodiscard]] int32_t equipBonus(int32_t Item::*stat) const;
    void refreshResists();                   // сопротивления класса плюс надетой брони

    uint32_t xpToNextLevel;
    Specialization spec;
    std::array<int, kEquipSlots> equipment{};
};

#endif //PROJECT_NAME_HERO_H