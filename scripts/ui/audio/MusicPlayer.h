#ifndef PROJECT_NAME_MUSICPLAYER_H
#define PROJECT_NAME_MUSICPLAYER_H

#include <map>
#include <string>
#include "SDL_mixer.h"
#include "SDL_log.h"
#include "../../controllers/audioManager.h"

class Visualizer;

class MusicPlayer {
public:
    std::map<std::string, Mix_Music*>  music;
    std::map<std::string, Mix_Chunk*>  sound;

    void playMusic(const AudioManager& v, const std::string& musicId) {
        auto it = music.find(musicId);
        if (it == music.end()) {
            SDL_Log("MusicPlayer::playMusic: '%s' not found", musicId.c_str());
            return;
        }
        // Не перезапускаем если уже играет тот же трек
        if (currentMusicId == musicId && Mix_PlayingMusic())
            return;

        if (Mix_PlayingMusic())
            v.stopMusic();

        v.playMusic(it->second);
        currentMusicId = musicId;
    }

    void playSound(const AudioManager& v, const std::string& soundId) {
        auto it = sound.find(soundId);
        if (it == sound.end()) {
            SDL_Log("MusicPlayer::playSound: '%s' not found", soundId.c_str());
            return;
        }
        // Не перезапускаем если уже играет тот же звук на том же канале
        if (currentSoundId == soundId && Mix_Playing(channel))
            return;

        if (Mix_Playing(channel))
            v.stopSound(channel);

        v.playSound(channel, it->second);
        currentSoundId = soundId;
    }

    void stopMusic(const Visualizer& v) {
        if (Mix_PlayingMusic())
            v.stopMusic();
        currentMusicId.clear();
    }

    void stopSound(const Visualizer& v) {
        if (Mix_Playing(channel))
            v.stopSound(channel);
        currentSoundId.clear();
    }

    [[nodiscard]] bool isMusicPlaying() const { return Mix_PlayingMusic() != 0; }
    [[nodiscard]] bool isSoundPlaying() const { return Mix_Playing(channel) != 0; }

private:
    int         channel{0};
    std::string currentMusicId;
    std::string currentSoundId;
};

#endif