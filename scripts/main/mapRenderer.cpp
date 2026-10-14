#include "mapRenderer.h"

#include <algorithm>

#include "SDL_image.h"
#include "SDL_log.h"

bool MapRenderer::load(SDL_Renderer* renderer, SDL_Surface* image, int chunkSize)
{
    clear();
    if (!renderer || !image) return false;

    SDL_Surface* src = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_RGBA32, 0);
    if (!src)
    {
        SDL_Log("MapRenderer: convert failed: %s", SDL_GetError());
        return false;
    }
    SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);      // копируем пиксели как есть, вместе с альфой

    imgW = src->w;
    imgH = src->h;

    for (int y = 0; y < imgH; y += chunkSize)
        for (int x = 0; x < imgW; x += chunkSize)
        {
            const SDL_Rect r{ x, y, std::min(chunkSize, imgW - x), std::min(chunkSize, imgH - y) };

            SDL_Surface* part = SDL_CreateRGBSurfaceWithFormat(0, r.w, r.h, 32, SDL_PIXELFORMAT_RGBA32);
            SDL_Texture* tex = nullptr;
            if (part)
            {
                SDL_BlitSurface(src, &r, part, nullptr);
                tex = SDL_CreateTextureFromSurface(renderer, part);
                SDL_FreeSurface(part);
            }
            if (!tex)
            {
                SDL_Log("MapRenderer: chunk (%d,%d) failed: %s", x, y, SDL_GetError());
                SDL_FreeSurface(src);
                clear();
                return false;
            }
            chunks.push_back({ tex, r.x, r.y, r.w, r.h });
        }

    SDL_FreeSurface(src);
    return true;
}

bool MapRenderer::loadFile(SDL_Renderer* renderer, const std::string& path, int chunkSize)
{
    SDL_Surface* s = IMG_Load(path.c_str());
    if (!s)
    {
        SDL_Log("MapRenderer: cannot load %s: %s", path.c_str(), IMG_GetError());
        return false;
    }
    const bool ok = load(renderer, s, chunkSize);
    SDL_FreeSurface(s);
    return ok;
}

void MapRenderer::draw(SDL_Renderer* renderer, int camX, int camY, int viewW, int viewH, int scale) const
{
    for (const Chunk& c : chunks)
    {
        const SDL_Rect dst{ c.x * scale - camX, c.y * scale - camY, c.w * scale, c.h * scale };
        if (dst.x >= viewW || dst.y >= viewH || dst.x + dst.w <= 0 || dst.y + dst.h <= 0)
            continue;                                      // за пределами экрана
        SDL_RenderCopy(renderer, c.tex, nullptr, &dst);
    }
}

void MapRenderer::clear()
{
    for (Chunk& c : chunks) SDL_DestroyTexture(c.tex);
    chunks.clear();
    imgW = imgH = 0;
}