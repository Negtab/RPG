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
    [[nodiscard]] const std::string& getScript() const noexcept { return script; }

    // Сцена ведёт жителя (npcWalk, npcFace): новое место и взгляд — его дом, пока игрок не ушёл с карты
    void walk(Point to);
    void turn(Direction d) noexcept { if (d != Direction::Idle) facing = homeFacing = d; }

    // Горожанин уступает дорогу: шагает в сторону, а через несколько секунд возвращается той же дорогой
    void setYields(bool on) noexcept { yieldsWay = on; }
    [[nodiscard]] bool yields() const noexcept { return yieldsWay; }
    [[nodiscard]] bool isStepping() const noexcept { return stepLeft > 0.f; }
    [[nodiscard]] bool isAway() const noexcept { return !trail.empty(); }
    [[nodiscard]] bool canStepAway() const noexcept { return trail.size() < kMaxTrail; }   // обмен местами не ограничен
    [[nodiscard]] Point stepBackCell() const { return trail.back(); }
    void stepAside(Point to);
    void stepBack();
    void updateStep(float dt) noexcept { stepLeft = stepLeft > dt ? stepLeft - dt : 0.f; }
    [[nodiscard]] float addAwayTime(float dt) noexcept { return awayTime += dt; }
    void goHome();                                    // без анимации: игрок ушёл с карты — на место из Tiled
    [[nodiscard]] float visualX() const noexcept;     // дробная клетка для рисования
    [[nodiscard]] float visualY() const noexcept;

    virtual void talk(Game& game);                    // сцена npc.<script>, иначе реплики в окне разговора

protected:
    std::string name;
    std::string sprite;
    Point cell;
    Direction facing;
    std::vector<std::string> lines;
    std::string script;                               // npc.<script> в resources/Story/*.lua
    std::string showIf, hideIf;                       // флаги сюжета: король не всегда на троне, принцесса — тоже

private:
    static constexpr size_t kMaxTrail = 3;            // дальше трёх шагов от своего места не уходит
    void moveTo(Point to);

    bool yieldsWay = false;
    std::vector<Point> trail;                         // откуда пришёл: от дома до текущей клетки
    Direction homeFacing;
    Point spawnCell;                                  // как поставили в Tiled: сюда возвращает goHome
    Direction spawnFacing;
    Point stepFrom{};
    float stepLeft = 0.f;
    float awayTime = 0.f;
};

#endif //PROJECT_NAME_NOTGAMEPERSON_H
