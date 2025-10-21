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


void Visualizer::playSound(int &chanel, Mix_Chunk *sound) const
{
    chanel = Mix_PlayChannel(-1, sound, 0);
}

void Visualizer::stopSound(int &chanel) const { Mix_Pause(chanel); }

void Visualizer::playMusic(Mix_Music* music) const
{
    if (!music) return;
    Mix_PlayMusic(music, -1);
}

void Visualizer::stopMusic() const { Mix_HaltMusic(); }