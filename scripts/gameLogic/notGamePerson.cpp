#include "notGamePerson.h"

#include "../main/game.h"

NotGamePerson::NotGamePerson(std::string name, std::string sprite, Point cell, Direction facing, std::vector<std::string> lines)
    : name(std::move(name)), sprite(std::move(sprite)), cell(cell), facing(facing), lines(std::move(lines)),
      homeFacing(facing), stepFrom(cell) {}

namespace {
    constexpr float kStepSeconds = 0.25f;
}

void NotGamePerson::moveTo(Point to)
{
    facing = to.x > cell.x ? Direction::Right : to.x < cell.x ? Direction::Left
           : to.y > cell.y ? Direction::Down : Direction::Up;
    stepFrom = cell;
    cell = to;
    stepLeft = kStepSeconds;
    awayTime = 0.f;
}

void NotGamePerson::stepAside(Point to)
{
    trail.push_back(cell);
    moveTo(to);
}

void NotGamePerson::stepBack()
{
    moveTo(trail.back());
    trail.pop_back();
    if (trail.empty()) facing = homeFacing;
}

void NotGamePerson::goHome()
{
    if (!trail.empty()) cell = trail.front();
    trail.clear();
    stepFrom = cell;
    stepLeft = awayTime = 0.f;
    facing = homeFacing;
}

float NotGamePerson::visualX() const noexcept
{
    return static_cast<float>(cell.x) + (static_cast<float>(stepFrom.x - cell.x) * stepLeft / kStepSeconds);
}

float NotGamePerson::visualY() const noexcept
{
    return static_cast<float>(cell.y) + (static_cast<float>(stepFrom.y - cell.y) * stepLeft / kStepSeconds);
}

void NotGamePerson::talk(Game& game)
{
    if (!script.empty() && game.getStory().start("npc", script)) return;
    if (!lines.empty()) game.showDialog(name, lines);
}
