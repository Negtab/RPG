#ifndef PROJECT_NAME_MOVEMENT_H
#define PROJECT_NAME_MOVEMENT_H

#include <cstdint>
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
};

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

inline void advance(MoveState& m, float dt) {
    if (!m.moving) return;
    m.progress += dt * CELLS_PER_SEC;
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

inline MoveResult updateLocal(MoveState& m, const MapData& map, Direction want, float dt) {
    MoveResult r;
    const bool was = m.moving;
    advance(m, dt);
    r.stepFinished = was && !m.moving;
    if (!m.moving && want != Direction::Idle) {
        m.dir = want;                                  // поворот даже при упоре в стену
        const Point t = stepTo(m.cell, want);
        if (map.isWalkable(t.x, t.y)) { m.target = t; m.moving = true; m.progress = 0.f; r.stepStarted = true; }
    }
    r.stopped = r.stepFinished && !m.moving;
    return r;
}

#endif