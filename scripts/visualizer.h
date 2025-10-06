#ifndef PROJECT_NAME_VISUALIZER_H
#define PROJECT_NAME_VISUALIZER_H

#include "SDL_render.h"
#include "SDL_mixer.h"

class Visualizer
{
public:
    Visualizer() = default;
    ~Visualizer() = default;

    void drawTexture(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y, int w, int h) const;
    void playMusic(Mix_Music * music) const;
    void stopMusic() const;

    void playSound(int &chanel, Mix_Chunk *sound) const;
    void stopSound(int &chanel) const;
};

#endif