#include "mapRenderer.h"
#include "terrain.h"

#include <algorithm>
#include <cmath>

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

    findWater(src);
    buildMinimap(renderer, src);
    SDL_FreeSurface(src);
    return true;
}

namespace {
    Uint32 hash(Uint32 a, Uint32 b, Uint32 c = 0)          // одинаковый для клетки при каждом запуске
    {
        Uint32 h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u ^ (c + 0x165667B1u) * 0xC2B2AE3Du;
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        return h;
    }
}

void MapRenderer::findWater(const SDL_Surface* src)
{
    const auto coverage = waterCoverage(src, kTile, waterW, waterH);
    water.resize(coverage.size());
    for (size_t i = 0; i < coverage.size(); ++i)
        water[i] = coverage[i] >= kTile * kTile - 4;                      // пара чужих пикселей — ещё вода, берег уже нет
}

void MapRenderer::buildMinimap(SDL_Renderer* renderer, const SDL_Surface* src)
{
    const int cw = imgW / kTile, ch = imgH / kTile;
    if (cw <= 0 || ch <= 0) return;
    SDL_Surface* small = SDL_CreateRGBSurfaceWithFormat(0, cw, ch, 32, SDL_PIXELFORMAT_RGBA32);
    if (!small) return;

    const auto* in = static_cast<const Uint8*>(src->pixels);
    auto* out = static_cast<Uint8*>(small->pixels);
    for (int cy = 0; cy < ch; ++cy)
        for (int cx = 0; cx < cw; ++cx)
        {
            unsigned r = 0, g = 0, b = 0;
            for (int y = 0; y < kTile; ++y)
            {
                const Uint8* row = in + (cy * kTile + y) * src->pitch + cx * kTile * 4;
                for (int x = 0; x < kTile; ++x) { r += row[x * 4]; g += row[x * 4 + 1]; b += row[x * 4 + 2]; }
            }
            Uint8* p = out + cy * small->pitch + cx * 4;
            constexpr unsigned n = kTile * kTile;
            p[0] = static_cast<Uint8>(r / n); p[1] = static_cast<Uint8>(g / n); p[2] = static_cast<Uint8>(b / n); p[3] = 255;
        }

    minimapTex = SDL_CreateTextureFromSurface(renderer, small);
    SDL_FreeSurface(small);
}

void MapRenderer::drawWater(SDL_Renderer* renderer, int camX, int camY, int viewW, int viewH, int scale, Uint32 ticks) const
{
    if (water.empty()) return;
    const int cell = kTile * scale;
    const int x0 = std::max(0, camX / cell), x1 = std::min(waterW - 1, (camX + viewW) / cell);
    const int y0 = std::max(0, camY / cell), y1 = std::min(waterH - 1, (camY + viewH) / cell);
    const float t = static_cast<float>(ticks) / 1000.0f;
    constexpr float kPi = 3.14159265f;

    // Сложение цветов: блик только высветляет воду и не бывает темнее её
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    auto line = [&](int sx, int sy, int px, int py, int len, int h, SDL_Color c) {
        const int a = std::max(0, px), b = std::min(kTile, px + len);           // не вылезаем из клетки
        if (b <= a) return;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        const SDL_Rect r{sx + a * scale, sy + py * scale, (b - a) * scale, h * scale};
        SDL_RenderFillRect(renderer, &r);
    };

    for (int cy = y0; cy <= y1; ++cy)
        for (int cx = x0; cx <= x1; ++cx)
        {
            if (!water[cy * waterW + cx]) continue;
            const Uint32 h = hash(cx, cy);
            const int sx = cx * cell - camX, sy = cy * cell - camY;

            // Гребень волны: короткая светлая черта, медленно плывёт вправо и «дышит»
            if (h & 1)
            {
                const int len = 4 + static_cast<int>((h >> 4) % 4);
                const float speed = 2.0f + static_cast<float>((h >> 8) % 3);           // пикселей карты в секунду
                const float travel = static_cast<float>(kTile + len);
                const int px = static_cast<int>(std::fmod(t * speed + static_cast<float>((h >> 12) % 16), travel)) - len;
                const int py = 2 + static_cast<int>((h >> 16) % 12);
                const float k = 0.5f + 0.5f * std::sin(t * 1.3f + static_cast<float>(h % 628) / 100.0f);
                line(sx, sy, px, py, len, 1, {70, 80, 120, static_cast<Uint8>(40 + 110 * k)});
            }

            // Искра: короткая вспышка в случайной точке, раз в несколько секунд и не у всех клеток
            const float period = 2.5f + static_cast<float>(h % 200) / 100.0f;
            const float phase = static_cast<float>((h >> 3) % 1000) / 1000.0f * period;
            const Uint32 cycle = static_cast<Uint32>((t + phase) / period);
            const float local = std::fmod(t + phase, period);
            constexpr float kLife = 0.6f;
            const Uint32 hc = hash(cx, cy, cycle);
            if (local < kLife && hc % 4 == 0)
            {
                const float k = std::sin(local / kLife * kPi);                         // 0 → 1 → 0
                const int px = 3 + static_cast<int>(hc % 10), py = 3 + static_cast<int>((hc >> 8) % 10);
                const int half = static_cast<int>(std::lround(k * 2.0f));               // 0..2: черта растёт и гаснет
                const SDL_Color white{255, 255, 255, static_cast<Uint8>(230 * k)};
                line(sx, sy, px - half, py, 2 * half + 1, 1, white);
                if (k > 0.75f) line(sx, sy, px, py - 1, 1, 3, white);                // на пике — крестик
            }
        }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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
    water.clear();
    waterW = waterH = 0;
    if (minimapTex) { SDL_DestroyTexture(minimapTex); minimapTex = nullptr; }
}