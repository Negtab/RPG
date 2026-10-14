#include "visualizer.h"

#include <algorithm>

void Visualizer::drawTexture(SDL_Texture* texture, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh) const
{
    SDL_Rect dst = { dx, dy, dw, dh };

    if (sx >= 0 && sy >= 0 && sw > 0 && sh > 0)
    {
        SDL_Rect src = { sx, sy, sw, sh };
        SDL_RenderCopy(renderer, texture, &src, &dst);
    }
    else
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
}

void Visualizer::drawSprite(SDL_Texture* texture, int footX, int footY, int scale, const SDL_Rect* src) const
{
    if (!texture) return;

    int w = 0, h = 0;
    if (src) { w = src->w; h = src->h; }
    else     SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    const SDL_Rect dst{ footX - (w * scale) / 2, footY - h * scale, w * scale, h * scale };
    SDL_RenderCopy(renderer, texture, src, &dst);
}

void Visualizer::drawRect(const SDL_Rect& rect, SDL_Color color) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void Visualizer::drawNinePatch(SDL_Texture* skin, const SDL_Rect& dst, int slice, int scale) const
{
    if (!skin) return;

    int tw = 0, th = 0;
    SDL_QueryTexture(skin, nullptr, nullptr, &tw, &th);

    const int s  = slice;
    const int ds = std::min({ s * scale, dst.w / 2, dst.h / 2 });   // граница на экране

    const int sx[3] = { 0, s, tw - s },  sw[3] = { s, tw - 2 * s, s };
    const int sy[3] = { 0, s, th - s },  sh[3] = { s, th - 2 * s, s };
    const int dx[3] = { dst.x, dst.x + ds, dst.x + dst.w - ds },  dw[3] = { ds, dst.w - 2 * ds, ds };
    const int dy[3] = { dst.y, dst.y + ds, dst.y + dst.h - ds },  dh[3] = { ds, dst.h - 2 * ds, ds };

    for (int j = 0; j < 3; ++j)
        for (int i = 0; i < 3; ++i)
        {
            if (dw[i] <= 0 || dh[j] <= 0) continue;
            const SDL_Rect src{ sx[i], sy[j], sw[i], sh[j] };
            const SDL_Rect d{ dx[i], dy[j], dw[i], dh[j] };
            SDL_RenderCopy(renderer, skin, &src, &d);
        }
}