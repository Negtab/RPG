//
// Created by User on 31.05.2026.
//

#ifndef PROJECT_NAME_AUDIOMANAGER_H
#define PROJECT_NAME_AUDIOMANAGER_H

#include "SDL_mixer.h"

class AudioManager {
public:

    AudioManager() = default;
    ~AudioManager() = default;


    static void playMusic(Mix_Music* music) ;
    static void stopMusic() ;
    static void playSound(int& channel, Mix_Chunk* sound) ;
    static void stopSound(int& channel) ;

};

#endif //PROJECT_NAME_AUDIOMANAGER_H