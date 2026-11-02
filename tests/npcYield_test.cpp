#include <gtest/gtest.h>

#include <set>

#include "../scripts/gameLogic/notGamePerson.h"
#include "../scripts/main/movement.h"

namespace {
    const Point kNpc{10, 10};
    auto freeCells(std::set<std::pair<int, int>> cells)
    {
        return [cells = std::move(cells)](Point p) { return cells.contains({p.x, p.y}); };
    }
}

TEST(NpcYield, StepsSidewaysFirst)
{
    const auto plan = planNpcYield(kNpc, Direction::Up, 1, true, true, freeCells({{11, 10}, {9, 10}, {10, 9}}));
    EXPECT_EQ(plan.kind, NpcYield::Step);
    EXPECT_EQ(plan.cell.x, 11);
    EXPECT_EQ(plan.cell.y, 10);
}

TEST(NpcYield, UsesTheOtherSideWhenOneIsBlocked)
{
    const auto plan = planNpcYield(kNpc, Direction::Right, 1, true, true, freeCells({{10, 9}}));
    EXPECT_EQ(plan.kind, NpcYield::Step);
    EXPECT_EQ(plan.cell.x, 10);
    EXPECT_EQ(plan.cell.y, 9);
}

TEST(NpcYield, StepsForwardWhenSidesAreBlocked)
{
    const auto plan = planNpcYield(kNpc, Direction::Up, 1, true, true, freeCells({{10, 9}}));
    EXPECT_EQ(plan.kind, NpcYield::Step);
    EXPECT_EQ(plan.cell.x, 10);
    EXPECT_EQ(plan.cell.y, 9);
}

TEST(NpcYield, SwapsInADeadEnd)
{
    const auto plan = planNpcYield(kNpc, Direction::Up, 1, true, true, freeCells({}));
    EXPECT_EQ(plan.kind, NpcYield::Swap);
}

TEST(NpcYield, SwapsWhenTooFarFromHome)
{
    const auto plan = planNpcYield(kNpc, Direction::Up, 1, false, true, freeCells({{11, 10}}));
    EXPECT_EQ(plan.kind, NpcYield::Swap);
}

TEST(NpcYield, StaysWhenSwapIsNotAllowed)
{
    const auto plan = planNpcYield(kNpc, Direction::Up, 1, true, false, freeCells({}));
    EXPECT_EQ(plan.kind, NpcYield::Stay);
}

TEST(NpcYield, WalksBackHomeAlongTheTrail)
{
    NotGamePerson npc("Dancer", "NpcDancer", {5, 5}, Direction::Down);
    npc.stepAside({6, 5});
    npc.stepAside({6, 6});
    EXPECT_TRUE(npc.isAway());

    EXPECT_EQ(npc.stepBackCell().x, 6);
    EXPECT_EQ(npc.stepBackCell().y, 5);
    npc.stepBack();
    npc.stepBack();
    EXPECT_FALSE(npc.isAway());
    EXPECT_EQ(npc.getCell().x, 5);
    EXPECT_EQ(npc.getCell().y, 5);
    EXPECT_EQ(npc.getFacing(), Direction::Down);
}

TEST(NpcYield, GoesHomeInstantly)
{
    NotGamePerson npc("Dancer", "NpcDancer", {5, 5}, Direction::Left);
    npc.stepAside({6, 5});
    npc.stepAside({7, 5});
    npc.goHome();
    EXPECT_FALSE(npc.isAway());
    EXPECT_FALSE(npc.isStepping());
    EXPECT_EQ(npc.getCell().x, 5);
    EXPECT_EQ(npc.getFacing(), Direction::Left);
}
