#ifndef PROJECT_NAME_MOVEMENT_H
#define PROJECT_NAME_MOVEMENT_H

#include <array>
#include <functional>
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
    std::string background;                   // задник боя в зоне (BB<имя>.png); пусто — по местности
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
    std::string showIf, hideIf;               // флаги сюжета: мост Корнелии появляется после спасения принцессы
};

// Есть ли у объекта карты право быть видимым при таких флагах сюжета
inline bool shownByFlags(const std::string& showIf, const std::string& hideIf,
                         const std::function<bool(const std::string&)>& flag)
{
    return (showIf.empty() || flag(showIf)) && (hideIf.empty() || !flag(hideIf));
}

// Сюжетная зона (слой "events" в Tiled): шаг в неё запускает сцену event.<script>
struct MapEvent {
    int x = 0, y = 0, w = 1, h = 1;
    std::string script;
};

// Комната под крышей (слой "rooms" в Tiled): пока герой внутри, карта рисуется картинкой roomView —
// крыши сняты, всё вокруг затемнено, как в Pixel Remaster
struct MapRoom {
    int x = 0, y = 0, w = 1, h = 1;
};

struct Portal {
    std::string name;                         // как объект назван в Tiled: город или пещера, ориентир для встреч
    int x = 0, y = 0;
    int w = 1, h = 1;                         // прямоугольник в клетках: вход шириной в несколько клеток
    std::string targetMap;                    // пусто — карта, у которой свойство entrance = name
    int targetX = 0, targetY = 0;
    bool hasTargetCell = false;               // нет targetX/targetY — на spawn целевой карты
};

// NPC, как он стоит в Tiled на слое "npcs": тайл из npcs.tsx или точка со свойством sprite
struct NpcSpawn {
    std::string name;                         // имя объекта в Tiled: показывается в окне разговора
    std::string sprite;                       // Npc<Имя>: лист resources/Images/Npcs/Npc<Имя>.png
    Point cell{};
    Direction facing = Direction::Down;
    std::string shop;                         // id из shops.json: NPC — продавец
    std::string text;                         // реплика; строки разделяются символом |
    std::string script;                       // сцена npc.<script> из resources/Story/*.lua
    std::string showIf, hideIf;               // виден только при флаге / пропадает при флаге
};

struct MapData {
    std::string name;
    int mapId = 0;
    std::string backgroundPath;
    bool waterEffects = true;                          // блики на воде; false — синий фон замка, а не море
    std::string roomViewPath;                          // вид изнутри комнат (слой-картинка "roomView")
    int w = 0, h = 0;
    Point spawn{0, 0};
    std::vector<uint8_t> solid;                        // w*h, 1 = стена (позже грузим из Tiled)
    std::vector<Zone> zones;
    std::vector<Portal> portals;
    std::vector<uint8_t> water;                        // w*h, 1 = клетка хотя бы наполовину вода (считается по картинке)
    std::vector<uint8_t> terrain;                      // w*h, Terrain из terrain.h — по цвету картинки
    std::vector<uint8_t> river;                        // w*h, 1 = узкая вода далеко от моря: по ней ходит каноэ
    std::vector<uint8_t> port;                         // w*h, 1 = причал: только здесь садятся на корабль и сходят с него
    std::vector<Point> portMarkers;                    // маркеры "port" из Tiled — свои причалы сверх найденных
    std::string exitMode;                              // "back": шаг за край карты — обратно, откуда пришли
    std::string entrance;                              // имя портала карты мира, который ведёт на эту карту
    std::vector<Prop> props;
    std::vector<NpcSpawn> npcs;
    std::vector<MapEvent> events;
    std::vector<MapRoom> rooms;
    std::vector<uint8_t> baseSolid, baseWater;         // стены и вода без объектов: для пересчёта по флагам
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
        for (const auto& p : portals)
            if (x >= p.x && x < p.x + p.w && y >= p.y && y < p.y + p.h) return &p;
        return nullptr;
    }
    // Номер комнаты под клеткой или -1: снаружи
    [[nodiscard]] int roomAt(int x, int y) const {
        for (size_t i = 0; i < rooms.size(); ++i)
            if (x >= rooms[i].x && x < rooms[i].x + rooms[i].w && y >= rooms[i].y && y < rooms[i].y + rooms[i].h)
                return static_cast<int>(i);
        return -1;
    }
    [[nodiscard]] const MapEvent* eventAt(int x, int y) const {
        for (const auto& e : events)
            if (x >= e.x && x < e.x + e.w && y >= e.y && y < e.y + e.h) return &e;
        return nullptr;
    }
    [[nodiscard]] const Portal* portalNamed(const std::string& n) const {
        for (const auto& p : portals) if (p.name == n) return &p;
        return nullptr;
    }
    [[nodiscard]] bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    [[nodiscard]] bool isWater(int x, int y) const {
        return inside(x, y) && !water.empty() && water[static_cast<size_t>(y) * w + x];
    }
    [[nodiscard]] bool isRiver(int x, int y) const {
        if (inside(x, y) && !river.empty() && river[static_cast<size_t>(y) * w + x]) return true;
        for (const auto& z : zones)
            if (z.river && x >= z.x0 && x <= z.x1 && y >= z.y0 && y <= z.y1) return true;
        return false;
    }
    [[nodiscard]] bool hasRivers() const {
        for (uint8_t r : river) if (r) return true;
        for (const auto& z : zones) if (z.river) return true;
        return false;
    }
    [[nodiscard]] bool isPort(int x, int y) const {
        return inside(x, y) && !port.empty() && port[static_cast<size_t>(y) * w + x];
    }
    [[nodiscard]] uint8_t terrainAt(int x, int y) const {
        return inside(x, y) && !terrain.empty() ? terrain[static_cast<size_t>(y) * w + x] : 0;
    }

    // Проходимость под объектами. Вызывать после того, как посчитана вода: мост убирает её под собой
    // Объекты меняют проходимость: замок — стена, мост — дорога над водой. Объект с условием (showIf/hideIf)
    // действует только при нужных флагах, поэтому при смене флагов карту пересчитывают от сохранённой основы
    void applyProps(const std::function<bool(const std::string&)>& flag = [](const std::string&) { return false; }) {
        if (baseSolid.empty()) { baseSolid = solid; baseWater = water; }
        solid = baseSolid;
        water = baseWater;
        for (const Prop& p : props)
            if (shownByFlags(p.showIf, p.hideIf, flag))
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

// Можно ли шагнуть из from в (x, y). Как в FF1: на корабль садятся и сходят только у причала,
// а каноэ не стоит на карте — на реке герои садятся в него сами, если оно у них есть
inline bool canEnter(const MapData& map, Vehicle riding, Point from, int x, int y,
                     const std::array<VehicleState, kVehicleKinds>& parked, uint16_t mapId)
{
    switch (riding)
    {
    case Vehicle::Airship: return map.inside(x, y);
    case Vehicle::Ship:    return vehicleCanMove(map, Vehicle::Ship, x, y) || (map.isWalkable(x, y) && map.isPort(x, y));
    case Vehicle::Canoe:   return vehicleCanMove(map, Vehicle::Canoe, x, y) || map.isWalkable(x, y);
    default: break;
    }
    if (map.isWalkable(x, y)) return true;
    if (parked[vehicleIndex(Vehicle::Canoe)].available && map.isRiver(x, y)) return true;
    for (int i = 0; i < kVehicleKinds; ++i)
    {
        const VehicleState& v = parked[i];
        if (static_cast<Vehicle>(i + 1) == Vehicle::Canoe) continue;           // каноэ не стоит, его не «заходят»
        if (!v.available || v.mapId != mapId || v.cell.x != x || v.cell.y != y) continue;
        return static_cast<Vehicle>(i + 1) != Vehicle::Ship || map.isPort(from.x, from.y);
    }
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