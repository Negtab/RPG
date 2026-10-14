// windowSkin.cpp
#include "windowSkin.h"

#include <algorithm>

namespace {
bool insideRounded(int x, int y, int w, int h, int r)
{
    if (x < 0 || y < 0 || x >= w || y >= h) return false;
    const int cx = x < r ? r - x : (x >= w - r ? x - (w - r - 1) : 0);
    const int cy = y < r ? r - y : (y >= h - r ? y - (h - r - 1) : 0);
    return cx * cx + cy * cy <= r * r;
}

Uint8 lerp8(Uint8 a, Uint8 b, float t) { return static_cast<Uint8>(a + (b - a) * t + 0.5f); }
}

SDL_Texture* makeWindowTexture(SDL_Renderer* renderer, int w, int h, const WindowStyle& s)
{
    if (!renderer || w < 8 || h < 8) return nullptr;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return nullptr;

    const int b = s.unit;                          // толщина рамки
    const int r = s.radius * s.unit;               // радиус скругления
    auto* pixels = static_cast<Uint8*>(surf->pixels);

    for (int y = 0; y < h; ++y)
    {
        const float t = static_cast<float>(y) / static_cast<float>(h - 1);
        const SDL_Color fill{ lerp8(s.top.r, s.bottom.r, t), lerp8(s.top.g, s.bottom.g, t),
                              lerp8(s.top.b, s.bottom.b, t), 255 };

        for (int x = 0; x < w; ++x)
        {
            SDL_Color c{0, 0, 0, 0};                                  // за скруглённым углом: прозрачно

            if (insideRounded(x, y, w, h, r))
            {
                const bool inner1 = insideRounded(x - b,     y - b,     w - 2 * b, h - 2 * b, std::max(0, r - b));
                const bool inner2 = insideRounded(x - 2 * b, y - 2 * b, w - 4 * b, h - 4 * b, std::max(0, r - 2 * b));
                c = !inner1 ? s.border : (!inner2 ? s.shadow : fill);
            }

            Uint8* p = pixels + y * surf->pitch + x * 4;
            p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;           // RGBA32: байты по порядку R, G, B, A
        }
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

SDL_Texture* getWindowSkin(SDL_Renderer* renderer)
{
    static SDL_Texture* skin = nullptr;                 // умрёт вместе с рендерером при выходе
    if (!skin)
        skin = makeWindowTexture(renderer, kWindowSkinSize, kWindowSkinSize);
    return skin;
}