#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include <string>

#include "SDL_events.h"
#include "visualizer.h"
#include "resourceManager.h"

class UIManager {
public:
    explicit UIManager(Visualizer& visualizer, ResourceManager& resourceManager, Game& game, Player& player) : visualizer(visualizer), resourceManager(resourceManager), game(game), player(player) {}

    void addScene(const std::string& id);
    void addButton(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick);
    void addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture);
    void addMusic(const std::string& id, const std::string& sceneId);
    void playMusic(const std::string& sceneId, const std::string& musicId);
    void handleEvent(const SDL_Event& event, const std::string& sceneId);
    void drawScene(const std::string& sceneId);

    void initialize();
    SDL_Rect screen;

private:
    struct Button
    {
        SDL_Rect rect;
        SDL_Texture* texture;
        std::function<void()> onClick;
    };
    struct Image
    {
        SDL_Rect rect;
        SDL_Texture* texture;
    };
    struct MusicPlayer
    {
        int chanel = 0;
        std::string currentMusicId;
        std::map<std::string, Mix_Music*> music;
        std::map<std::string, Mix_Chunk*> sound;
    };
    struct Scene
    {
        std::map<std::string, Button> buttons;
        std::map<std::string, Image> images;
        MusicPlayer musicPlayer;
    };

    std::map<std::string, Scene> scenes;
    Visualizer& visualizer;
    ResourceManager& resourceManager;
    Game& game;
    Player& player;
};

#endif