#include "notGamePerson.h"

#include "../main/game.h"

NotGamePerson::NotGamePerson(std::string name, std::string sprite, Point cell, Direction facing, std::vector<std::string> lines)
    : name(std::move(name)), sprite(std::move(sprite)), cell(cell), facing(facing), lines(std::move(lines)) {}

void NotGamePerson::talk(Game& game)
{
    if (!script.empty() && game.getStory().start("npc", script)) return;
    if (!lines.empty()) game.showDialog(name, lines);
}
