//
// Created by Artem on 12.10.2026.
//

#ifndef PROJECT_NAME_MAPRENDERER_H
#define PROJECT_NAME_MAPRENDERER_H
#include <string>
#include <vector>

#include "SDL_render.h"

class MapRenderer {
public:
    MapRenderer() = default;
    ~MapRenderer() { clear(); }
    MapRenderer(const MapRenderer&) = delete;
    MapRenderer& operator=(const MapRenderer&) = delete;

    // Режет картинку на куски-текстуры. Поверхность остаётся у вызывающего
    bool load(SDL_Renderer* renderer, SDL_Surface* image, int chunkSize = 1024);
    bool loadFile(SDL_Renderer* renderer, const std::string& path, int chunkSize = 1024);

    // camX/camY: левый верхний угол камеры в мировых экранных пикселях
    void draw(SDL_Renderer* renderer, int camX, int camY, int viewW, int viewH, int scale) const;

    // Блики на воде поверх draw(): гребни волн и искры. ticks — SDL_GetTicks()
    void drawWater(SDL_Renderer* renderer, int camX, int camY, int viewW, int viewH, int scale, Uint32 ticks) const;

    void clear();
    [[nodiscard]] int width()  const { return imgW; }
    [[nodiscard]] int height() const { return imgH; }

private:
    void findWater(const SDL_Surface* rgba);

    struct Chunk { SDL_Texture* tex; int x, y, w, h; };   // x, y, w, h в пикселях картинки
    std::vector<Chunk> chunks;
    int imgW = 0, imgH = 0;

    // Клетки 16x16, целиком залитые водой. Берег не отмечаем, чтобы блики не попадали на сушу
    static constexpr int kTile = 16;
    std::vector<uint8_t> water;
    int waterW = 0, waterH = 0;
};


#endif //PROJECT_NAME_MAP_RENDERER_H