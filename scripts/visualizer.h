#ifndef PROJECT_NAME_VISUALIZER_H
#define PROJECT_NAME_VISUALIZER_H

#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_mixer.h"

class Visualizer
{
public:
    Visualizer() = default;
    ~Visualizer() = default;

    void drawImage(SDL_Renderer renderer, SDL_Surface* surface, int x, int y) const;
    //void displayText() const;
    void playMusic(Mix_Music * music) const;
    void stopMusic() const;

    void playSound(int &chanel, Mix_Chunk *sound) const;
    void stopSound(int &chanel) const;
    private:
};
    #endif