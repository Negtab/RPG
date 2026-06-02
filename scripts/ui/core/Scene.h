#ifndef PROJECT_NAME_SCENE_H
#define PROJECT_NAME_SCENE_H

#include <string>
#include "Panel.h"
#include "../audio/MusicPlayer.h"
#include "../animation/animPlayer.h"

class Visualizer;
class AudioManager;

class Scene : public Panel {
public:
    std::string name;
    MusicPlayer musicPlayer;
    AnimPlayer  animPlayer;

    void draw(Visualizer& v, const SDL_Rect& screen) const;

    void playMusic(AudioManager audio, const std::string &musicId);
    void playSound(AudioManager audio, const std::string &soundId);

    void changePanelOrder(const std::string& panelId, int newOrder);

    void handleClickLocal(int x, int y) const;
    void handleHoverLocal(int x, int y) const;
};

#endif