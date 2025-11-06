#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "SDL_events.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_mixer.h"

#include "controllers/visualizer.h"
#include "controllers/resourceManager.h"

class UIManager
{
public:
    explicit UIManager(Visualizer& visualizer, ResourceManager& resourceManager, Game& game, Player& player)
        : visualizer(visualizer), resourceManager(resourceManager), game(game), player(player) {}

    void addScene(const std::string& id);
    void handleClickEvent(const SDL_Event& event, const std::string& sceneId);
    void handleHoverEvent(const SDL_Event& event, const std::string& sceneId);
    void drawScene(const std::string& sceneId);
    void addEnemys();
    void initialize();

    void addButton(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover, std::string panelId = "");
    void addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0}, std::string panelId = "");
    void addLabel(const std::string& id, const std::string& sceneId, const SDL_Rect& textRect, const SDL_Rect& imageRect, SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0}, std::string panelId = "");
    void addPanel(const std::string& id, const std::string& sceneId, std::string panelId = "");
    void addMusic(const std::string& musicId, const std::string& sceneId);
    void addSound(const std::string& soundId, const std::string& sceneId);
    void playSound(const std::string& sceneId, const std::string& soundId);

    void setVisible(const std::string& id, const std::string& sceneId, const bool& visible);
    void setEnabled(const std::string& id, const std::string& sceneId, const bool& enabled);
    void setTexture(const std::string &id, const std::string &sceneId, SDL_Texture *texture);
    void setRect(const std::string& id, const std::string& sceneId, const SDL_Rect& rect);
private:
    class UIObject
    {
    public:
        SDL_Rect rect{0,0,0,0};
        SDL_Texture* texture = nullptr;
        bool isVisible {true};
        bool isEnabled {true};
    };

    class Button : public UIObject
    {
    public:
        std::function<void()> onClick;
        std::function<void()> onHover;
    };

    class Image : public UIObject
    {
    public:
        SDL_Rect srect{0,0,0,0};
    };

    class Label : public UIObject
    {
    public:
        Image image;
    };

    class MusicPlayer
    {
    public:
        int chanel = 0;
        std::string currentMusicId;
        std::string currentSoundId;
        std::map<std::string, Mix_Music*> music;
        std::map<std::string, Mix_Chunk*> sound;
    };

    class Panel : public UIObject
    {
    public:
        std::map<std::string, Label> labels;
        std::map<std::string, Button> buttons;
        std::map<std::string, Image> images;
        std::vector<std::string> order;
        std::map<std::string, Panel> panels;

        void addPanelLocal(const std::string& id);
        void addButtonLocal(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture,
                            std::function<void()> onClick, std::function<void()> onHover);
        void addImageLocal(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0});
        void addLabelLocal(const std::string& id, const SDL_Rect& textRect, const SDL_Rect& imageRect,
                           SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0});

        virtual void handleClickLocal(int x, int y);
        virtual void handleHoverLocal(int x, int y);
    };

    class Scene : public Panel
    {
    public:
        std::string name;
        MusicPlayer musicPlayer;

        void drawPanel(const Panel& panel, Visualizer& visualizer);
        void changePanelOrder(const std::string& panelId, int newOrder);
        void playMusicLocal(const Visualizer& visualizer, const std::string& musicId);
        void playSoundLocal(const Visualizer& visualizer, const std::string& soundId);

        void handleClickLocal(int x, int y) override;
        void handleHoverLocal(int x, int y) override;
    };

    UIObject* findUIObject(const std::string &id, const std::string &sceneId);
    Scene* findScene(const std::string &id);

    std::map<std::string, Scene> scenes;
    Visualizer& visualizer;
    ResourceManager& resourceManager;
    Game& game;
    Player& player;
};

#endif
