#include "audioManager.h"

#include <algorithm>

#include <SDL_log.h>

int AudioManager::musicVolume = 7;
int AudioManager::soundVolume = 8;

namespace {
    int toMixer(int level) { return level * MIX_MAX_VOLUME / AudioManager::kVolumeSteps; }
}

void AudioManager::playSound(int &chanel, Mix_Chunk *sound)
{
    chanel = Mix_PlayChannel(-1, sound, 0);
    if (chanel >= 0) Mix_Volume(chanel, toMixer(soundVolume));     // канал мог быть выделен уже после настройки
}

void AudioManager::stopSound(int &chanel) { Mix_Pause(chanel); }

void AudioManager::playMusic(Mix_Music* music)
{
    if (!music) return;
    Mix_PlayMusic(music, -1);
    Mix_VolumeMusic(toMixer(musicVolume));                          // некоторые версии SDL_mixer сбрасывают её при старте трека
}

void AudioManager::stopMusic() { Mix_HaltMusic(); }

void AudioManager::setMusicVolume(int level)
{
    musicVolume = std::clamp(level, 0, kVolumeSteps);
    Mix_VolumeMusic(toMixer(musicVolume));
}

void AudioManager::setSoundVolume(int level)
{
    soundVolume = std::clamp(level, 0, kVolumeSteps);
    Mix_Volume(-1, toMixer(soundVolume));
}
