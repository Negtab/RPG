#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include "../uiControlls/animPlayer.h"
#include <vector>

#include "game.h"
#include "SDL_events.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_mixer.h"

#include "../controllers/visualizer.h"
#include "../controllers/resourceManager.h"


class UIManager
{
public:
    using AnimPlayer = ::AnimPlayer;

    explicit UIManager(Visualizer& visualizer, ResourceManager& resourceManager, Game& game, Player& player)
        : visualizer(visualizer), resourceManager(resourceManager), game(game), player(player), screen({}) {}

    void addScene(const std::string& id);
    void handleClickEvent(const SDL_Event& event, const std::string& sceneId);
    void handleHoverEvent(const SDL_Event& event, const std::string& sceneId);
    void drawScene(const std::string& sceneId);

    void addEnemys();
    void addCharacters();

    void initialize();
    void update(float dt);

    [[nodiscard]] bool isAnimationFinished(const std::string& sceneId) const;
    void playAnimation(const std::string& animId, const std::string& sceneId, bool restart);
    void addAnimation(const std::string &id, const std::string &sceneId, std::vector<SDL_Texture*> frames, std::vector<SDL_Rect> rect, float time = -1.0f, AnimPlayMode mode = AnimPlayMode::OneShot);
    void addAnimation(const std::string& id, const std::string& sceneId, std::vector<SDL_Texture*> frames, SDL_Rect rect, float frameTime = -1.0f, AnimPlayMode mode = AnimPlayMode::Loop);
    void addButton(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover, std::string panelId = "");
    void addEdit(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, const SDL_Rect& srect, SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover, std::string panelId = "");
    void addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0}, std::string panelId = "");
    void addLabel(const std::string& id, const std::string& sceneId, const SDL_Rect& textRect, const SDL_Rect& imageRect, SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect = {0,0,0,0}, std::string panelId = "");
    void addPanel(const std::string& id, const std::string& sceneId, const std::string &panelId = "");
    void addMusic(const std::string& musicId, const std::string& sceneId);
    void addSound(const std::string& soundId, const std::string& sceneId);

    void playSound(const std::string& sceneId, const std::string& soundId);

    void setVisible(const std::string& id, const std::string& sceneId, const bool& visible);
    void setEnabled(const std::string& id, const std::string& sceneId, const bool& enabled);
    void setEnVI(const std::string& id, const std::string& sceneId, const bool& enabled);
    void setTexture(const std::string &id, const std::string &sceneId, SDL_Texture *texture);
    void setRect(const std::string& id, const std::string& sceneId, const SDL_Rect& rect);
    void setSrcRect(const std::string& id, const std::string& sceneId, const SDL_Rect& srect);
    void setOnClick(const std::string &id, const std::string &sceneId, const std::function<void()>& onClick);

    void setText(const std::string& id, const std::string& sceneId, const std::string& text);
    [[nodiscard]] std::string getText(const std::string& id, const std::string& sceneId);

    void setFocus(const std::string& id, const std::string& sceneId, const bool& isFocus);
    [[nodiscard]] bool getFocus(const std::string& id, const std::string& sceneId);

    void setPos(const std::string& id, const std::string& sceneId, const int& pos);
    [[nodiscard]] int getPos(const std::string& id, const std::string& sceneId);

    [[nodiscard]] SDL_Rect getRect(const std::string& id, const std::string& sceneId);

    void moveSelectorToNext();
    void moveSelectorToPrevious();
    void moveSelector();

    void confirmTarget();

    void setState(const UIChooseState& state);
    [[nodiscard]] UIChooseState getState() const;


    void updatePlayerAnimation(AnimPlayer &animPlayer, const Player& player);
    void updateRemoteAnimation(AnimPlayer& animPlayer, const Player& player);

private:

    class UIObject
    {
    public:
        virtual ~UIObject() = default;
        SDL_Rect rect{0,0,0,0};
        mutable SDL_Texture* texture = nullptr;
        bool isVisible {true};
        bool isEnabled {true};
    };

    class Edit : public UIObject
    {
    public:
        bool isFocused {false};
        int pos {0};
        std::string text;
        SDL_Rect srect{0,0,0,0};
        std::function<void()> onClick;
        std::function<void()> onHover;
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
        int chanel {0};
        std::string currentMusicId;
        std::string currentSoundId;
        std::map<std::string, Mix_Music*> music;
        std::map<std::string, Mix_Chunk*> sound;
    };

    class Panel : public UIObject
    {
    public:
        std::map<std::string, Edit> edits;
        std::map<std::string, Label> labels;
        std::map<std::string, Button> buttons;
        std::map<std::string, Image> images;
        std::vector<std::string> order;
        std::map<std::string, Panel> panels;

        void addPanelLocal(const std::string& id);
        void addButtonLocal(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture,
                            std::function<void()> onClick, std::function<void()> onHover);
        void addEditLocal(const std::string& id, const SDL_Rect& rect, const SDL_Rect& srect, SDL_Texture* texture,
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
        AnimPlayer animPlayer;
        //std::stack<std::string> animationsStack;
        //std::stack<std::string> soundsStack;
        //std::stack<std::string> musicStack;

        void drawPanel(const Panel& panel, Visualizer& visualizer);
        void changePanelOrder(const std::string& panelId, int newOrder);
        void playMusicLocal(const Visualizer& visualizer, const std::string& musicId);
        void playSoundLocal(const Visualizer& visualizer, const std::string& soundId);

        void handleClickLocal(int x, int y) override;
        void handleHoverLocal(int x, int y) override;
    };

    UIObject* findUIObject(const std::string &id, const std::string &sceneId);
    UIObject* findInPanel(const std::string& id, Panel* panel);

    Scene* findScene(const std::string &id);

    void onActionButton(ActionType type);
    void onSkillSelected(int skillId);
    void onItemSelected(int itemId);
    void onTargetSelected(int index);
    void onEnvironmentCLick(const int &id = -1, const bool &isMagic = false);

    void initMagic();
    void initItems();

    [[nodiscard]] int getTargetCount(TargetType type) const;



    std::string currentPlayerAnim;

    Action pendingAction;
    UIChooseState uiState { UIChooseState::ChooseAction };
    UIChooseState prevUIState { UIChooseState::ChooseAction };

    int currentHeroIndex = 0;      // КОГО сейчас выбираем
    int selectedTargetIndex = 0;  // КАКУЮ цель выбираем
    TargetType currentType { TargetType::Enemy };

    SDL_Rect screen;
    std::map<std::string, Scene> scenes;
    Visualizer& visualizer;
    ResourceManager& resourceManager;
    Game& game;
    Player& player;
};

#endif
