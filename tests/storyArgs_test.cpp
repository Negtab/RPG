#include <gtest/gtest.h>

#include "../scripts/gameLogic/notGamePerson.h"
#include "../scripts/story/scriptArgs.h"

using D = Direction;

TEST(StoryArgs, DirectionWordsAndLetters)
{
    EXPECT_EQ(parseDirection("up"), D::Up);
    EXPECT_EQ(parseDirection("d"), D::Down);
    EXPECT_EQ(parseDirection("left"), D::Left);
    EXPECT_EQ(parseDirection("r"), D::Right);
    EXPECT_FALSE(parseDirection("north"));
    EXPECT_FALSE(parseDirection(""));
}

TEST(StoryArgs, WalkPathCountsSteps)
{
    const auto path = parseWalkPath("l3 u, R");
    ASSERT_TRUE(path);
    EXPECT_EQ(*path, (std::vector<D>{D::Left, D::Left, D::Left, D::Up, D::Right}));
}

TEST(StoryArgs, WalkPathRejectsTyposAndHugeCounts)
{
    EXPECT_FALSE(parseWalkPath("l2 x"));
    EXPECT_FALSE(parseWalkPath("u1000"));
    ASSERT_TRUE(parseWalkPath(""));
    EXPECT_TRUE(parseWalkPath("")->empty());
}

TEST(StoryArgs, ClassNamesForScripts)
{
    EXPECT_STREQ(classScriptName(SpecializationNames::WhiteMage), "white_mage");
    EXPECT_STREQ(classScriptName(SpecializationNames::Warrior), "warrior");    EXPECT_STREQ(classScriptName(SpecializationNames::None), "");
}

TEST(StoryArgs, ScriptedWalkBecomesNewHome)
{
    NotGamePerson guard("Guard", "guard", {5, 5}, D::Down);
    guard.stepAside({6, 5});                                 // уступил дорогу игроку
    guard.walk({6, 4});                                      // а сцена повела его дальше
    EXPECT_FALSE(guard.isAway());
    EXPECT_EQ(guard.getFacing(), D::Up);
    guard.stepAside({7, 4});                                 // уступает дорогу уже от нового места
    guard.stepBack();
    EXPECT_EQ(guard.getCell().x, 6);
    EXPECT_EQ(guard.getCell().y, 4);
    EXPECT_EQ(guard.getFacing(), D::Up);
}

TEST(StoryArgs, LeavingTheMapUndoesScriptedWalk)
{
    NotGamePerson guard("Guard", "guard", {5, 5}, D::Down);
    guard.walk({5, 4});
    guard.turn(D::Left);
    guard.goHome();                                          // сменили карту или загрузили сохранение
    EXPECT_EQ(guard.getCell().x, 5);
    EXPECT_EQ(guard.getCell().y, 5);
    EXPECT_EQ(guard.getFacing(), D::Down);
}
