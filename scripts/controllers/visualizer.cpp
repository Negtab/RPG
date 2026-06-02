#include "visualizer.h"

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

void Visualizer::drawRect(const SDL_Rect& rect, SDL_Color color) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}