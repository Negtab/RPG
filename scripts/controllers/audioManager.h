//
// Created by User on 31.05.2026.
//

#ifndef PROJECT_NAME_AUDIOMANAGER_H
#define PROJECT_NAME_AUDIOMANAGER_H

#include <string>

#include "SDL_mixer.h"

class AudioManager {
public:

    AudioManager() = default;
    ~AudioManager() = default;


    static void playMusic(Mix_Music* music) ;
    static void stopMusic() ;
    static void playSound(int& channel, Mix_Chunk* sound) ;
    static void stopSound(int& channel) ;

    // Громкость в шагах 0..kVolumeSteps: так её удобно крутить стрелками и рисовать полоской
    static constexpr int kVolumeSteps = 10;
    static void setMusicVolume(int level);
    static void setSoundVolume(int level);
    [[nodiscard]] static int getMusicVolume() { return musicVolume; }
    [[nodiscard]] static int getSoundVolume() { return soundVolume; }

    static void loadSettings(const std::string& path);   // нет файла: громкость по умолчанию
    static bool saveSettings(const std::string& path);

private:
    static int musicVolume;
    static int soundVolume;
};

#endif //PROJECT_NAME_AUDIOMANAGER_H
