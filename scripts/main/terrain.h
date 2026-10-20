#ifndef PROJECT_NAME_TERRAIN_H
#define PROJECT_NAME_TERRAIN_H

#include <cstdint>
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

#endif
