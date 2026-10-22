#ifndef PROJECT_NAME_UTILS_H
#define PROJECT_NAME_UTILS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

struct SDLContext
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
};

bool initSDL(SDLContext &ctx, int width = 800, int height = 600)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return false;
    }

    ctx.window = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!ctx.window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(ctx.window, width / 2, height / 2);

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx.renderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(ctx.window);
        SDL_Quit();
        return false;
    }
    // Игра всегда рисует в width x height, SDL растягивает это на окно с полосами по краям
    SDL_RenderSetLogicalSize(ctx.renderer, width, height);

    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG)))
    {
        SDL_Log("Failed to init SDL_image: %s", SDL_GetError());
        SDL_DestroyRenderer(ctx.renderer);
        SDL_DestroyWindow(ctx.window);
        SDL_Quit();
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
    {
        SDL_Log("Failed to init SDL_mixer: %s", SDL_GetError());
        IMG_Quit();
        SDL_DestroyRenderer(ctx.renderer);
        SDL_DestroyWindow(ctx.window);
        SDL_Quit();
        return false;
    }

    if (TTF_Init() < 0)
    {
        SDL_Log("Failed to init SDL_ttf: %s", TTF_GetError());
        Mix_Quit();
        IMG_Quit();
        SDL_DestroyRenderer(ctx.renderer);
        SDL_DestroyWindow(ctx.window);
        SDL_Quit();
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    return true;
}

void cleanupSDL(SDLContext &ctx)
{
    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.window);
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}
#endif
