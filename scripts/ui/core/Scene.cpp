#include "Scene.h"
#include "../audio/MusicPlayer.h"

void Scene::draw(Visualizer& v, const SDL_Rect& screen) const {
    Panel::draw(v);
    animPlayer.draw(v, screen);  // всегда один метод
}

void Scene::playMusic(const AudioManager audio, const std::string &musicId) {
    musicPlayer.playMusic(audio, musicId);
}

void Scene::playSound(const AudioManager audio, const std::string &soundId) {
    musicPlayer.playSound(audio, soundId);
}

void Scene::changePanelOrder(const std::string& panelId, int newOrder) {
    auto it = std::find(order.begin(), order.end(), panelId);
    if (it == order.end()) {
        SDL_Log("Scene::changePanelOrder: '%s' not found", panelId.c_str());
        return;
    }

    int clamped = std::clamp(newOrder, 0, static_cast<int>(order.size()) - 1);
    int oldIndex = static_cast<int>(std::distance(order.begin(), it));
    if (oldIndex == clamped) return;

    std::string val = std::move(*it);
    order.erase(it);
    order.insert(order.begin() + clamped, std::move(val));
}