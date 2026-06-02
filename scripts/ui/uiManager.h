#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "SDL_events.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_ttf.h"

#include "../controllers/visualizer.h"
#include "../controllers/resourceManager.h"
#include "core/Scene.h"
#include "animation/animPlayer.h"

class Game;
class Player;
class UIObject;

class UIManager {
public:
    using AnimPlayer = ::AnimPlayer;

    explicit UIManager(Visualizer& visualizer, AudioManager& audioManager, ResourceManager& resourceManager, Game& game, Player& player)
        : visualizer(visualizer), audioManager(audioManager), resourceManager(resourceManager),
          game(game), player(player), screen({}) {}

    // ── Сцены ──────────────────────────────────────────
    void initialize();
    void update(float dt);
    void drawScene(const std::string& sceneId);

    void handleClickEvent(const SDL_Event& event, const std::string& sceneId);
    void handleHoverEvent(const SDL_Event& event, const std::string& sceneId);

    // ── Добавление виджетов ────────────────────────────
    void addScene(const std::string& id);
    void addPanel(const std::string& id, const std::string& sceneId,
                  const std::string& panelId = "");
    void addButton(const std::string& id, const std::string& sceneId,
                   const SDL_Rect& rect, SDL_Texture* texture,
                   std::function<void()> onClick, std::function<void()> onHover,
                   const std::string& panelId = "");
    void addEdit(const std::string& id, const std::string& sceneId,
                 const SDL_Rect& rect, TTF_Font* font, SDL_Color color,
                 std::function<void()> onClick, std::function<void()> onHover,
                 const std::string& panelId = "");
    void addImage(const std::string& id, const std::string& sceneId,
                  const SDL_Rect& rect, SDL_Texture* texture,
                  SDL_Rect srect = {0,0,0,0}, const std::string& panelId = "");
    void addLabel(const std::string& id, const std::string& sceneId,
                  const SDL_Rect& bgRect, SDL_Texture* bgTexture,
                  const std::string& text, TTF_Font* font,
                  SDL_Color color = {0,0,0,255},
                  int paddingX = 8, int paddingY = 4,
                  const std::string& panelId = "");

    // ── Аудио ──────────────────────────────────────────
    void addMusic(const std::string& musicId, const std::string& sceneId);
    void addSound(const std::string& soundId, const std::string& sceneId);
    void playSound(const std::string& sceneId, const std::string& soundId);

    // ── Анимация ───────────────────────────────────────
    void addAnimation(const std::string& id, const std::string& sceneId,
                      std::vector<SDL_Texture*> frames, std::vector<SDL_Rect> rects,
                      float frameTime = -1.0f,
                      AnimPlayMode mode = AnimPlayMode::OneShot);
    void addAnimation(const std::string& id, const std::string& sceneId,
                      std::vector<SDL_Texture*> frames, SDL_Rect rect,
                      float frameTime = -1.0f,
                      AnimPlayMode mode = AnimPlayMode::Loop);
    void playAnimation(const std::string& animId, const std::string& sceneId,
                       bool restart);
    [[nodiscard]] bool isAnimationFinished(const std::string& sceneId) const;

    void updatePlayerAnimation(AnimPlayer& animPlayer, const Player& player);
    void updateRemoteAnimation(AnimPlayer& animPlayer, const Player& player);

    // ── Setters ────────────────────────────────────────
    void setVisible (const std::string& id, const std::string& sceneId, bool visible);
    void setEnabled (const std::string& id, const std::string& sceneId, bool enabled);
    void setEnVI    (const std::string& id, const std::string& sceneId, bool value);
    void setTexture (const std::string& id, const std::string& sceneId, SDL_Texture* texture);
    void setRect    (const std::string& id, const std::string& sceneId, const SDL_Rect& rect);
    void setSrcRect (const std::string& id, const std::string& sceneId, const SDL_Rect& srect);
    void setOnClick (const std::string& id, const std::string& sceneId,
                     std::function<void()> onClick);
    void setText    (const std::string& id, const std::string& sceneId,
                     const std::string& text);
    void setFocus   (const std::string& id, const std::string& sceneId, bool focus);
    void setPos     (const std::string& id, const std::string& sceneId, int pos);

    // ── Getters ────────────────────────────────────────
    [[nodiscard]] std::string  getText  (const std::string& id, const std::string& sceneId);
    [[nodiscard]] bool         getFocus (const std::string& id, const std::string& sceneId);
    [[nodiscard]] int          getPos   (const std::string& id, const std::string& sceneId);
    [[nodiscard]] SDL_Rect     getRect  (const std::string& id, const std::string& sceneId);

    // ── Battle ─────────────────────────────────────────
    void addEnemys();
    void addCharacters();

    void moveSelectorToNext();
    void moveSelectorToPrevious();
    void moveSelector();
    void confirmTarget();

    void setState(const UIChooseState& state);
    [[nodiscard]] UIChooseState getState() const;

private:
    // ── Поиск ─────────────────────────────────────────
    Scene*     findScene    (const std::string& id);
    UIObject*  findUIObject (const std::string& id, const std::string& sceneId);

    // ── Отрисовка ──────────────────────────────────────
    void drawRemotePlayers(Scene& scene);

    // ── Обновление данных ──────────────────────────────
    void updateSceneData (const std::string& sceneId, Scene& scene);
    void updateMapData   (Scene& scene);
    void updateBattleData(Scene& scene);

    // ── Инициализация сцен ─────────────────────────────
    void initMapScene    ();
    void initMenuScene   ();
    void initOptionsScene();
    void initOnlineScene ();
    void initBattleScene ();

    // ── Battle logic ───────────────────────────────────
    void onActionButton    (ActionType type);
    void onEnvironmentCLick(int id, bool isMagic);

    void initMagic();
    void initItems();

    void snapSelectorToTarget();
    [[nodiscard]] int getTargetCount(TargetType type) const;

    // ── Состояние ──────────────────────────────────────
    std::string   currentPlayerAnim;
    Action        pendingAction;
    UIChooseState uiState    { UIChooseState::ChooseAction };
    UIChooseState prevUIState{ UIChooseState::ChooseAction };
    int           currentHeroIndex    = 0;
    int           selectedTargetIndex = 0;
    TargetType    currentType         { TargetType::Enemy };

    SDL_Rect screen;
    std::map<std::string, Scene> scenes;

    Visualizer&      visualizer;
    AudioManager& audioManager;
    ResourceManager& resourceManager;
    Game&            game;
    Player&          player;
};

#endif