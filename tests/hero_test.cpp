#include <gtest/gtest.h>

#include "../scripts/gameLogic/gameData.h"
#include "../scripts/gameLogic/hero.h"

namespace {
    // Данные игры грузятся один раз на весь набор тестов
    class HeroTest : public ::testing::Test
    {
    protected:
        static void SetUpTestSuite() { GameData::instance().loadAll(RPG_DATA_DIR); }
    };
}

TEST_F(HeroTest, WarriorStartsWithFullHpAndMp)
{
    const Hero hero(Specialization(SpecializationNames::Warrior));
    EXPECT_EQ(hero.getLevel(), 1);
    EXPECT_GT(hero.getMaxHp(), 0u);
    EXPECT_EQ(hero.getCurrentHp(), hero.getMaxHp());
    EXPECT_EQ(hero.getCurrentMp(), hero.getMaxMp());
    EXPECT_TRUE(hero.getIsAlive());
}

TEST_F(HeroTest, LevelUpRaisesStats)
{
    Hero hero(Specialization(SpecializationNames::Warrior));
    const uint32_t hp = hero.getMaxHp();
    const uint32_t atk = hero.getBaseAttack();

    hero.levelUp();

    EXPECT_EQ(hero.getLevel(), 2);
    EXPECT_GT(hero.getMaxHp(), hp);
    EXPECT_GT(hero.getBaseAttack(), atk);
    EXPECT_EQ(hero.getCurrentHp(), hero.getMaxHp());
}

TEST_F(HeroTest, EnoughXpGivesLevel)
{
    Hero hero(Specialization(SpecializationNames::Thief));
    hero.addXP(hero.getXpToNextLevel());
    EXPECT_EQ(hero.getLevel(), 2);
    EXPECT_EQ(hero.getExperience(), 0u);
}

TEST_F(HeroTest, SetLevelNeverLowers)
{
    Hero hero(Specialization(SpecializationNames::Monk));
    hero.setLevel(5);
    EXPECT_EQ(hero.getLevel(), 5);
    hero.setLevel(2);
    EXPECT_EQ(hero.getLevel(), 5);
}

TEST_F(HeroTest, LearnSkillOnlyOnce)
{
    Hero hero(Specialization(SpecializationNames::WhiteMage));
    hero.learnSkill(999);
    hero.learnSkill(999);
    EXPECT_TRUE(hero.knowsSkill(999));
    EXPECT_EQ(hero.getLearnedSkills().size(), 1u);
}

TEST_F(HeroTest, EquipStartingGear)
{
    Hero hero(Specialization(SpecializationNames::Warrior));
    const uint32_t bare = hero.getAttackPower(nullptr);
    hero.equipStarting();
    EXPECT_GE(hero.getAttackPower(nullptr), bare);
}
