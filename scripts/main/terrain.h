#ifndef PROJECT_NAME_TERRAIN_H
#define PROJECT_NAME_TERRAIN_H

#include <cstdint>
#include <iterator>
#include <vector>

#include "SDL_surface.h"

// Вода на картинке карты: океан FF1 — синий #5522ff и близкие оттенки.
// Общая для логики (где плавает корабль) и отрисовки (где блестит вода)
inline bool isWaterPixel(const Uint8* rgba) { return rgba[2] >= 0xC0 && rgba[0] <= 0x80 && rgba[1] <= 0x50; }

// Сколько пикселей воды в каждой клетке tile x tile (0..tile*tile). Поверхность должна быть RGBA32
inline std::vector<uint16_t> waterCoverage(const SDL_Surface* rgba, int tile, int& cellsW, int& cellsH)
{
    cellsW = rgba->w / tile;
    cellsH = rgba->h / tile;
    std::vector<uint16_t> out(static_cast<size_t>(cellsW) * cellsH, 0);
    const auto* pixels = static_cast<const Uint8*>(rgba->pixels);

    for (int cy = 0; cy < cellsH; ++cy)
        for (int cx = 0; cx < cellsW; ++cx)
        {
            uint16_t count = 0;
            for (int y = 0; y < tile; ++y)
            {
                const Uint8* row = pixels + (cy * tile + y) * rgba->pitch + cx * tile * 4;
                for (int x = 0; x < tile; ++x)
                    count += isWaterPixel(row + x * 4);
            }
            out[static_cast<size_t>(cy) * cellsW + cx] = count;
        }
    return out;
}

// Местность клетки по преобладающему цвету карты FF1: от неё зависит задник боя
enum class Terrain : uint8_t { Plains, Forest, Swamp, Desert, Mountain, Water };

inline std::vector<uint8_t> terrainCells(const SDL_Surface* rgba, int tile, int cellsW, int cellsH)
{
    struct Ref { Uint8 r, g, b; Terrain t; };
    static const Ref refs[] = {
        {0x44, 0x99, 0x22, Terrain::Plains},  {0x88, 0xCC, 0x33, Terrain::Plains},  {0xAA, 0xDD, 0x55, Terrain::Plains},
        {0x22, 0x44, 0x22, Terrain::Forest},  {0x22, 0x99, 0x44, Terrain::Swamp},   {0xDD, 0xBB, 0x66, Terrain::Desert},
        {0x44, 0x44, 0x22, Terrain::Mountain},{0x55, 0x55, 0x33, Terrain::Mountain},{0x55, 0x22, 0xFF, Terrain::Water},
    };
    std::vector<uint8_t> out(static_cast<size_t>(cellsW) * cellsH, 0);
    const auto* pixels = static_cast<const Uint8*>(rgba->pixels);
    for (int cy = 0; cy < cellsH; ++cy)
        for (int cx = 0; cx < cellsW; ++cx)
        {
            int votes[6] = {};
            for (int y = 0; y < tile; y += 2)                      // каждый второй пиксель: для большинства хватает
                for (int x = 0; x < tile; x += 2)
                {
                    const Uint8* p = pixels + (cy * tile + y) * rgba->pitch + (cx * tile + x) * 4;
                    int best = 0, bestD = 1 << 30;
                    for (int i = 0; i < static_cast<int>(std::size(refs)); ++i)
                    {
                        const int dr = p[0] - refs[i].r, dg = p[1] - refs[i].g, db = p[2] - refs[i].b;
                        const int d = dr * dr + dg * dg + db * db;
                        if (d < bestD) { bestD = d; best = i; }
                    }
                    ++votes[static_cast<int>(refs[best].t)];
                }
            int t = 0;
            for (int i = 1; i < 6; ++i) if (votes[i] > votes[t]) t = i;
            out[static_cast<size_t>(cy) * cellsW + cx] = static_cast<uint8_t>(t);
        }
    return out;
}

#endif
