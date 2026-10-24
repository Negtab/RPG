#ifndef PROJECT_NAME_NOTGAMEPERSON_H
#define PROJECT_NAME_NOTGAMEPERSON_H
#include <string>
#include <vector>

#include "../main/types.h"

class Game;

// Житель на карте: стоит в клетке, загораживает её и говорит, если подойти и нажать Enter
class NotGamePerson
{
public:
    NotGamePerson(std::string name, std::string sprite, Point cell, Direction facing, std::vector<std::string> lines = {});
    void setScript(std::string s) { script = std::move(s); }
    void setVisibility(std::string show, std::string hide) { showIf = std::move(show); hideIf = std::move(hide); }
    [[nodiscard]] const std::string& getShowIf() const noexcept { return showIf; }
    [[nodiscard]] const std::string& getHideIf() const noexcept { return hideIf; }
    virtual ~NotGamePerson() = default;

    [[nodiscard]] const std::string& getName() const noexcept { return name; }
    [[nodiscard]] const std::string& getSprite() const noexcept { return sprite; }
    [[nodiscard]] Point getCell() const noexcept { return cell; }
    [[nodiscard]] Direction getFacing() const noexcept { return facing; }
    void setFacing(Direction d) noexcept { if (d != Direction::Idle) facing = d; }
    [[nodiscard]] const std::vector<std::string>& getLines() const noexcept { return lines; }

    virtual void talk(Game& game);                    // сцена npc.<script>, иначе реплики в окне разговора

protected:
    std::string name;
    std::string sprite;
    Point cell;
    Direction facing;
    std::vector<std::string> lines;
    std::string script;                               // npc.<script> в resources/Story/*.lua
    std::string showIf, hideIf;                       // флаги сюжета: король не всегда на троне, принцесса — тоже
};

#endif //PROJECT_NAME_NOTGAMEPERSON_H
