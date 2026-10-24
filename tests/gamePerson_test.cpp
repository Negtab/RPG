#include <gtest/gtest.h>

#include "../scripts/gameLogic/gamePerson.h"

namespace {
    // GamePerson абстрактный: минимальный персонаж, чтобы проверить общую логику HP/MP и урона
    class Dummy : public GamePerson
    {
    public:
        Dummy(uint32_t hp, uint32_t mp)
        {
            maxHp = currentHp = hp;
            maxMp = currentMp = mp;
            isAlive = true;
        }
        void setResists(std::vector<Elemental> r, std::vector<Elemental> imm, std::vector<Elemental> vul)
        {
            resists = std::move(r);
            immunities = std::move(imm);
            vulnerability = std::move(vul);
        }

        uint32_t getAttackPower(const Skill*) const override { return 0; }
        uint32_t getDefense() const override { return 0; }
        uint32_t getSpeed() const override { return 1; }
        std::vector<SkillInstance> getAvailableSkills() const override { return skills; }
    };
}

TEST(GamePerson, TakeDamageReducesHp)
{
    Dummy p(100, 0);
    p.takeDamage(30);
    EXPECT_EQ(p.getCurrentHp(), 70u);
    EXPECT_TRUE(p.getIsAlive());
}

TEST(GamePerson, LethalDamageKills)
{
    Dummy p(100, 0);
    p.takeDamage(500);
    EXPECT_EQ(p.getCurrentHp(), 0u);
    EXPECT_FALSE(p.getIsAlive());
}

TEST(GamePerson, HealDoesNotExceedMaxHp)
{
    Dummy p(100, 0);
    p.takeDamage(10);
    p.heal(1000);
    EXPECT_EQ(p.getCurrentHp(), 100u);
}

TEST(GamePerson, ElementalDamage)
{
    Dummy p(1000, 0);
    p.setResists({Elemental::Fire}, {Elemental::Ice}, {Elemental::Dark});

    p.takeDamage(100, Elemental::Fire);     // сопротивление: половина
    EXPECT_EQ(p.getCurrentHp(), 950u);
    p.takeDamage(100, Elemental::Ice);      // иммунитет: урона нет
    EXPECT_EQ(p.getCurrentHp(), 950u);
    p.takeDamage(100, Elemental::Dark);     // уязвимость: x1.5
    EXPECT_EQ(p.getCurrentHp(), 800u);
}

TEST(GamePerson, ManaClampedToMax)
{
    Dummy p(10, 50);
    p.spendMana(20);
    EXPECT_EQ(p.getCurrentMp(), 30u);
    p.restoreMana(1000);
    EXPECT_EQ(p.getCurrentMp(), 50u);
}
