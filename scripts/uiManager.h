#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include <string>

#include "SDL_events.h"
#include "controllers/visualizer.h"
#include "controllers/resourceManager.h"

class UIManager {
public:
    explicit UIManager(Visualizer& visualizer, ResourceManager& resourceManager, Game& game, Player& player) : visualizer(visualizer), resourceManager(resourceManager), game(game), player(player) {}

    void addScene(const std::string& id);
    void addPanel(const std::string& id, const std::string& sceneId);
    void addButton(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover);
    void addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0});
    void addLabel(const std::string& id, const std::string& sceneId, const SDL_Rect& textRect, const SDL_Rect& imageRect, SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect = {0, 0,0,0});
    void addMusic(const std::string& id, const std::string& sceneId);
    void addSound(const std::string& id, const std::string& sceneId);
    void playMusic(const std::string& sceneId, const std::string& musicId);
    void playSound(const std::string& sceneId, const std::string& soundId);
    void handleClickEvent(const SDL_Event& event, const std::string& sceneId);
    void handleHoverEvent(const SDL_Event& event, const std::string& sceneId);
    void drawScene(const std::string& sceneId);
    void addEnemys(const Game& game);
    void initialize();
    SDL_Rect screen;

private:
    struct UIObject
    {
        SDL_Rect rect;
        SDL_Texture* texture;
    };
    struct Button : UIObject
    {
        std::function<void()> onClick;
        std::function<void()> onHover;
    };
    struct Image : UIObject
    {
        SDL_Rect srect;
    };
    struct Label : UIObject
    {
        Image image;
    };
    struct MusicPlayer
    {
        int chanel = 0;
        std::string currentMusicId, currentSoundId;
        std::map<std::string, Mix_Music*> music;
        std::map<std::string, Mix_Chunk*> sound;
    };
    struct Panel
    {
        std::map<std::string, Label> labels;
        std::map<std::string, Button> buttons;
        std::map<std::string, Image> images;
        std::vector<std::string> order;
    };
    struct Scene
    {
        std::map<std::string, Label> labels;
        std::map<std::string, Button> buttons;
        std::map<std::string, Image> images;
        std::vector<std::string> order;
        std::map<std::string, Panel> panels;
        MusicPlayer musicPlayer;
    };

    void drawPanel(const Panel& panel);
    void changePanelOrder(const std::string& sceneId, const std::string& panelId, const int& newOrder);

    std::map<std::string, Scene> scenes;
    Visualizer& visualizer;
    ResourceManager& resourceManager;
    Game& game;
    Player& player;
};

#endif