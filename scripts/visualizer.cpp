#include "visualizer.h"


void Visualizer::playSound(int &chanel, Mix_Chunk *sound) const
{
    chanel = Mix_PlayChannel(-1, sound, 0);
}

void Visualizer::stopSound(int &chanel) const
{
    Mix_Pause(chanel);
}

void Visualizer::playMusic(Mix_Music* music) const
{
    Mix_PlayMusic(music, -1);
}

void Visualizer::stopMusic() const
{
    Mix_HaltMusic();
}