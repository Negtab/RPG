#ifndef PROJECT_NAME_MOVEMENT_H
#define PROJECT_NAME_MOVEMENT_H

#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include "../controllers/visualizer.h"
#include "types.h"

constexpr float CELLS_PER_SEC = 4.0f;
constexpr int   TILE_PX       = 16;
constexpr int   CELL_PX       = TILE_PX * PIXEL_SCALE; // клетка на экране


struct Zone {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;      // клетки, включительно
    std::string name;
    bool safe = false;
    int encounterRate = 8;
    std::string encounter;// % на шаг
    std::vector<int> enemies;
    bool river = false;                       // по воде в этой зоне ходит каноэ, а не корабль
};

// Объект поверх картинки карты: замок, башня, мост. Ставится в Tiled тайлом из props.tsx
enum class PropCollision { Solid, Walk, None };   // стена (кроме клеток-порталов), проходимо поверх воды, не трогать
struct Prop {
    std::string sprite;                       // имя PNG в resources/Images/Props без расширения
    float px = 0.f, py = 0.f;                 // левый нижний угол в пикселях карты, как в Tiled: рисуем точно там
    int pw = 16, ph = 16;                     // размер картинки в пикселях карты
    int x0 = 0, bottom = 0;                   // клетки, покрытые больше чем наполовину: левая и нижний ряд
    int w = 1, h = 1;                         // их число по ширине и высоте
    PropCollision collision = PropCollision::Solid;
};

struct Portal {
    int x = 0, y = 0;
    std::string targetMap;
    int targetX = 0, targetY = 0;
};

struct MapData {
    std::string name;
    int mapId = 0;
    std::string backgroundPath;
    int w = 0, h = 0;
    Point spawn{0, 0};
    std::vector<uint8_t> solid;                        // w*h, 1 = стена (позже грузим из Tiled)
    std::vector<Zone> zones;
    std::vector<Portal> portals;
    std::vector<uint8_t> water;                        // w*h, 1 = клетка хотя бы наполовину вода (считается по картинке)
    std::vector<Prop> props;
    std::array<std::optional<Point>, kVehicleKinds> vehicleMarkers;   // маркеры canoe, ship, airship из Tiled

    // Временная карта без стен. 256x277 клеток = 4096x4432 px, как твоя картинка
    static MapData openField(int w, int h) {
        MapData m; m.w = w; m.h = h; m.spawn = {w / 2, h / 2};
        m.solid.assign(static_cast<size_t>(w) * h, 0);
        return m;
    }
    [[nodiscard]] bool isWalkable(int x, int y) const {
        return x >= 0 && y >= 0 && x < w && y < h && !solid[static_cast<size_t>(y) * w + x];
    }

    [[nodiscard]] const Zone* zoneAt(int x, int y) const {
        for (auto it = zones.rbegin(); it != zones.rend(); ++it)      // последняя нарисованная важнее
            if (x >= it->x0 && x <= it->x1 && y >= it->y0 && y <= it->y1) return &*it;
        return nullptr;
    }
    [[nodiscard]] const Portal* portalAt(int x, int y) const {
        for (const auto& p : portals) if (p.x == x && p.y == y) return &p;
        return nullptr;
    }
    [[nodiscard]] bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    [[nodiscard]] bool isWater(int x, int y) const {
        return inside(x, y) && !water.empty() && water[static_cast<size_t>(y) * w + x];
    }
    [[nodiscard]] bool isRiver(int x, int y) const {
        for (const auto& z : zones)
            if (z.river && x >= z.x0 && x <= z.x1 && y >= z.y0 && y <= z.y1) return true;
        return false;
    }
    [[nodiscard]] bool hasRivers() const {
        for (const auto& z : zones) if (z.river) return true;
        return false;
    }

    // Проходимость под объектами. Вызывать после того, как посчитана вода: мост убирает её под собой
    void applyProps() {
        for (const Prop& p : props)
            for (int y = p.bottom - p.h + 1; y <= p.bottom; ++y)
                for (int x = p.x0; x < p.x0 + p.w; ++x)
                {
                    if (!inside(x, y)) continue;
                    const size_t i = static_cast<size_t>(y) * w + x;
                    if (p.collision == PropCollision::Solid && !portalAt(x, y)) solid[i] = 1;   // вход в замок открыт
                    if (p.collision == PropCollision::Walk)
                    {
                        solid[i] = 0;
                        if (!water.empty()) water[i] = 0;          // под мостом корабль не проплывёт
                    }
                }
    }
};

// ── Транспорт ────────────────────────────────────────

inline float vehicleSpeed(Vehicle v)                   // множитель к CELLS_PER_SEC
{
    switch (v)
    {
    case Vehicle::Ship:    return 1.25f;
    case Vehicle::Airship: return 2.25f;
    default:               return 1.0f;
    }
}

// Плывёт ли транспорт по клетке (без высадки): корабль — море, каноэ — реки, дирижабль — где угодно
inline bool vehicleCanMove(const MapData& map, Vehicle v, int x, int y)
{
    switch (v)
    {
    case Vehicle::Ship:    return map.isWater(x, y) && !map.isRiver(x, y);
    case Vehicle::Canoe:   return map.isWater(x, y) && map.isRiver(x, y);
    case Vehicle::Airship: return map.inside(x, y);
    default:               return false;
    }
}

// Можно ли шагнуть в клетку. Корабль и каноэ высаживают на сушу; пешком можно зайти на стоящий транспорт
inline bool canEnter(const MapData& map, Vehicle riding, int x, int y,
                     const std::array<VehicleState, kVehicleKinds>& parked, uint16_t mapId)
{
    if (riding != Vehicle::None)
        return vehicleCanMove(map, riding, x, y) || (riding != Vehicle::Airship && map.isWalkable(x, y));
    if (map.isWalkable(x, y)) return true;
    for (const auto& v : parked)
        if (v.available && v.mapId == mapId && v.cell.x == x && v.cell.y == y) return true;
    return false;
}

struct MoveState {
    Point     cell{};
    Point     target{};
    Direction dir{Direction::Down};   // куда смотрит; Idle сюда не пишем
    bool      moving{false};
    float     progress{0.f};          // 0..1 внутри шага
    uint16_t  mapId{0};
};

struct Vec2f { float x, y; };

struct MoveResult { bool stepStarted = false, stepFinished = false, stopped = false; };

inline Point stepTo(Point c, Direction d) {
    switch (d) {
        case Direction::Up:    return {c.x, c.y - 1};
        case Direction::Down:  return {c.x, c.y + 1};
        case Direction::Left:  return {c.x - 1, c.y};
        case Direction::Right: return {c.x + 1, c.y};
        default:               return c;
    }
}

inline void advance(MoveState& m, float dt, float speed = 1.0f) {
    if (!m.moving) return;
    m.progress += dt * CELLS_PER_SEC * speed;
    if (m.progress >= 1.f) { m.cell = m.target; m.moving = false; m.progress = 0.f; }
}

inline void settle(MoveState& m) {                     // мгновенно завершить шаг (начало боя)
    if (m.moving) m.cell = m.target;
    m.moving = false; m.progress = 0.f;
}

inline Vec2f visualCell(const MoveState& m) {          // дробная позиция для камеры и рисования
    const float t = m.moving ? m.progress : 0.f;
    return { m.cell.x + (m.target.x - m.cell.x) * t, m.cell.y + (m.target.y - m.cell.y) * t };
}

// passable(Point) решает, можно ли шагнуть в клетку: пешком, на корабле и в воздухе правила разные
template <class Passable>
inline MoveResult updateLocal(MoveState& m, Passable&& passable, Direction want, float dt, float speed = 1.0f) {
    MoveResult r;
    const bool was = m.moving;
    advance(m, dt, speed);
    r.stepFinished = was && !m.moving;
    if (!m.moving && want != Direction::Idle) {
        m.dir = want;                                  // поворот даже при упоре в стену
        const Point t = stepTo(m.cell, want);
        if (passable(t)) { m.target = t; m.moving = true; m.progress = 0.f; r.stepStarted = true; }
    }
    r.stopped = r.stepFinished && !m.moving;
    return r;
}

#endif