#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include "game.h"

int main(int argc, char* args [])
{
    Game game;
    SDL_Window *window = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Init(SDL_INIT_EVERYTHING);

    if (window == nullptr)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return 1;
    }

    if (renderer == nullptr)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int result = IMG_Init( IMG_INIT_JPG | IMG_INIT_PNG );

    if ( result == 0 ) {
        SDL_Log("Failed to init image: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    result = Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 1024 );

    // Check load
    if ( result == -1 )
    {
        SDL_Log("Failed to init audio: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if ( TTF_Init() < 0 ) {
        SDL_Log("Error initializing SDL_ttf: %s", TTF_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
    }

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
                break;
            }

        }
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    Mix_Quit();
    SDL_Quit();
    return 0;
}
