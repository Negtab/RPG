#ifndef PROJECT_NAME_VISUALIZER_H
#define PROJECT_NAME_VISUALIZER_H

#include "SDL_render.h"
#include "SDL_mixer.h"

class Visualizer {
public:
    explicit Visualizer(SDL_Renderer* renderer) : renderer(renderer) {}

    void drawTexture(SDL_Texture* texture, int dx, int dy, int dw, int dh, int sx = -1, int sy = -1, int sw = -1, int sh = -1) const;
    void playMusic(Mix_Music* music) const;
    void stopMusic() const;
    void playSound(int& channel, Mix_Chunk* sound) const;
    void stopSound(int& channel) const;

private:
    SDL_Renderer* renderer;
};

#endif