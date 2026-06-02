#include "audioManager.h"

void AudioManager::playSound(int &chanel, Mix_Chunk *sound)
{
    chanel = Mix_PlayChannel(-1, sound, 0);
}

void AudioManager::stopSound(int &chanel) { Mix_Pause(chanel); }

void AudioManager::playMusic(Mix_Music* music)
{
    if (!music) return;
    Mix_PlayMusic(music, -1);
}

void AudioManager::stopMusic() { Mix_HaltMusic(); }