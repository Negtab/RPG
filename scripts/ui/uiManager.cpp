#include "UIManager.h"
#include "../main/game.h"
#include <SDL_log.h>

constexpr int ENV_BUTTON_COUNT = 16;

// ─────────────────────────────────────────
// State
// ─────────────────────────────────────────

void UIManager::setState(const UIChooseState& state) { uiState = state; }
UIChooseState UIManager::getState() const { return uiState; }

// ─────────────────────────────────────────
// Scene lookup
// ─────────────────────────────────────────

Scene* UIManager::findScene(const std::string& id) {
    auto it = scenes.find(id);
    if (it == scenes.end()) {
        SDL_Log("UIManager: scene '%s' not found", id.c_str());
        return nullptr;
    }
    return &it->second;
}

UIObject* UIManager::findUIObject(const std::string& id, const std::string& sceneId) {
    Scene* scene = findScene(sceneId);
    if (!scene) return nullptr;

    UIObject* obj = scene->find(id);   // рекурсивный поиск в Panel
    if (!obj)
        SDL_Log("UIManager: element '%s' not found in scene '%s'",
                id.c_str(), sceneId.c_str());
    return obj;
}

// ─────────────────────────────────────────
// Scene management
// ─────────────────────────────────────────

void UIManager::addScene(const std::string& id) {
    if (!scenes.contains(id)) {
        scenes[id].name = id;
    }
}

// ─────────────────────────────────────────
// Widget добавление — делегируем в Panel
// ─────────────────────────────────────────

void UIManager::addPanel(const std::string& id, const std::string& sceneId,
                          const std::string& panelId)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    Panel* target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target) { SDL_Log("addPanel: panel '%s' not found", panelId.c_str()); return; }
    target->addPanel(id);
}

void UIManager::addButton(const std::string& id, const std::string& sceneId,
                           const SDL_Rect& rect, SDL_Texture* texture,
                           std::function<void()> onClick, std::function<void()> onHover,
                           const std::string& panelId)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    Panel* target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target) { SDL_Log("addButton: panel '%s' not found", panelId.c_str()); return; }
    target->addButton(id, rect, texture, std::move(onClick), std::move(onHover));
}

void UIManager::addEdit(const std::string& id, const std::string& sceneId,
                         const SDL_Rect& rect, TTF_Font* font, SDL_Color color,
                         std::function<void()> onClick, std::function<void()> onHover,
                         const std::string& panelId)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    Panel* target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target) { SDL_Log("addEdit: panel '%s' not found", panelId.c_str()); return; }
    target->addEdit(id, rect, font, color, std::move(onClick), std::move(onHover));
}

void UIManager::addImage(const std::string& id, const std::string& sceneId,
                          const SDL_Rect& rect, SDL_Texture* texture,
                          SDL_Rect srect, const std::string& panelId)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    Panel* target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target) { SDL_Log("addImage: panel '%s' not found", panelId.c_str()); return; }
    target->addImage(id, rect, texture, srect);
}

void UIManager::addLabel(const std::string& id, const std::string& sceneId,
                          const SDL_Rect& bgRect, SDL_Texture* bgTexture,
                          const std::string& text, TTF_Font* font,
                          SDL_Color color, int paddingX, int paddingY,
                          const std::string& panelId)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    Panel* target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target) { SDL_Log("addLabel: panel '%s' not found", panelId.c_str()); return; }
    target->addLabel(id, bgRect, bgTexture, text, font, color, paddingX, paddingY);
}

// ─────────────────────────────────────────
// Audio
// ─────────────────────────────────────────

void UIManager::addMusic(const std::string& musicId, const std::string& sceneId) {
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    Mix_Music* m = ResourceManager::getMusic(musicId);
    if (!m) { SDL_Log("addMusic: resource '%s' not found", musicId.c_str()); return; }
    scene->musicPlayer.music[musicId] = m;
}

void UIManager::addSound(const std::string& soundId, const std::string& sceneId) {
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    Mix_Chunk* c = ResourceManager::getSound(soundId);
    if (!c) { SDL_Log("addSound: resource '%s' not found", soundId.c_str()); return; }
    scene->musicPlayer.sound[soundId] = c;
}

void UIManager::playSound(const std::string& sceneId, const std::string& soundId) {
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    scene->musicPlayer.playSound(audioManager, soundId);
}

// ─────────────────────────────────────────
// Animation
// ─────────────────────────────────────────

void UIManager::addAnimation(const std::string& id, const std::string& sceneId,
                              std::vector<SDL_Texture*> frames,
                              std::vector<SDL_Rect> rects,
                              float frameTime, AnimPlayMode mode)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    scene->animPlayer.animations[id] = { frameTime, std::move(frames), std::move(rects) };
}

void UIManager::addAnimation(const std::string& id, const std::string& sceneId,
                              std::vector<SDL_Texture*> frames,
                              SDL_Rect rect, float frameTime, AnimPlayMode mode)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    scene->animPlayer.animations[id] = {
        frameTime, std::move(frames), {}, rect, 0, 0.0f, false, mode
    };
}

void UIManager::playAnimation(const std::string& animId,
                               const std::string& sceneId, bool restart)
{
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    scene->animPlayer.play(animId, restart);
}

bool UIManager::isAnimationFinished(const std::string& sceneId) const {
    auto it = scenes.find(sceneId);
    if (it == scenes.end()) return true;
    const AnimPlayer& p = it->second.animPlayer;
    return p.currentAnimationId.empty() && p.animationQueue.empty();
}

// ─────────────────────────────────────────
// Setters / Getters
// ─────────────────────────────────────────

void UIManager::setVisible(const std::string& id, const std::string& sceneId, bool visible) {
    if (auto* obj = findUIObject(id, sceneId)) {
        obj->isVisible = visible;
        obj->isEnabled = visible;
    }
}

void UIManager::setEnabled(const std::string& id, const std::string& sceneId, bool enabled) {
    if (auto* obj = findUIObject(id, sceneId))
        obj->isEnabled = enabled;
}

void UIManager::setEnVI(const std::string& id, const std::string& sceneId, bool value) {
    if (auto* obj = findUIObject(id, sceneId)) {
        obj->isEnabled = value;
        obj->isVisible = value;
    }
}

void UIManager::setTexture(const std::string& id, const std::string& sceneId,
                            SDL_Texture* texture)
{
    if (auto* obj = findUIObject(id, sceneId))
        obj->texture = texture;
}

void UIManager::setRect(const std::string& id, const std::string& sceneId,
                         const SDL_Rect& rect)
{
    if (auto* obj = findUIObject(id, sceneId))
        obj->rect = rect;
}

SDL_Rect UIManager::getRect(const std::string& id, const std::string& sceneId) {
    if (auto* obj = findUIObject(id, sceneId))
        return obj->rect;
    return {0, 0, 0, 0};
}

void UIManager::setSrcRect(const std::string& id, const std::string& sceneId,
                            const SDL_Rect& srect)
{
    if (auto* obj = findUIObject(id, sceneId))
        obj->setSrcRect(srect);   // виртуальный метод — no-op если не Image
}

void UIManager::setText(const std::string& id, const std::string& sceneId,
                         const std::string& text)
{
    if (auto* obj = findUIObject(id, sceneId))
        obj->setText(text);
}

std::string UIManager::getText(const std::string& id, const std::string& sceneId) {
    if (auto* obj = findUIObject(id, sceneId))
        return obj->getText();
    return "";
}

void UIManager::setFocus(const std::string& id, const std::string& sceneId, bool focus) {
    if (auto* obj = findUIObject(id, sceneId))
        obj->setFocus(focus);
}

bool UIManager::getFocus(const std::string& id, const std::string& sceneId) {
    if (auto* obj = findUIObject(id, sceneId))
        return obj->getFocus();
    return false;
}

void UIManager::setPos(const std::string& id, const std::string& sceneId, int pos) {
    if (auto* obj = findUIObject(id, sceneId))
        obj->setPos(pos);
}

int UIManager::getPos(const std::string& id, const std::string& sceneId) {
    if (auto* obj = findUIObject(id, sceneId))
        return obj->getPos();
    return 0;
}

void UIManager::setOnClick(const std::string& id, const std::string& sceneId,
                            std::function<void()> onClick)
{
    if (auto* obj = findUIObject(id, sceneId))
        obj->setOnClick(std::move(onClick));
}

// ─────────────────────────────────────────
// Events
// ─────────────────────────────────────────

void UIManager::handleClickEvent(const SDL_Event& event, const std::string& sceneId) {
    if (event.type != SDL_MOUSEBUTTONDOWN) return;

    if (sceneId == "Battle" && !isAnimationFinished("Battle"))
        return;

    Scene* scene = findScene(sceneId);
    if (!scene) return;
    scene->handleClick(event.button.x, event.button.y);
}

void UIManager::handleHoverEvent(const SDL_Event& event, const std::string& sceneId) {
    if (event.type != SDL_MOUSEMOTION) return;
    Scene* scene = findScene(sceneId);
    if (!scene) return;
    scene->handleHover(event.motion.x, event.motion.y);
}

// ─────────────────────────────────────────
// Draw
// ─────────────────────────────────────────

void UIManager::drawScene(const std::string& sceneId) {
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    scene->draw(visualizer, game.getScreenRect());   // Panel::draw рекурсивно + animPlayer

    if (sceneId == "Map")
        drawRemotePlayers(*scene);

    scene->musicPlayer.playMusic(audioManager, sceneId + "Theme");
}

void UIManager::drawRemotePlayers(Scene& scene) {
    Point localPos = player.getPlayerCoords();
    SDL_Rect scr   = game.getScreenRect();

    for (const auto& [id, remote] : game.getRemotePlayers()) {
        Point remotePos = remote.getPlayerCoords();
        int screenX = scr.w / 2 - (remotePos.x - localPos.x);
        int screenY = scr.h / 2 - (remotePos.y - localPos.y);

        const AnimPlayer& anim = remote.animPlayer;
        if (remote.getDirection() == Direction::Idle || anim.currentAnimationId.empty()) {
            visualizer.drawTexture(ResourceManager::getTexture("MovingCharacter"),
                                   screenX, screenY, 45, 45, 0, 0, 16, 25);
        } else {
            const auto& cur = anim.animations.at(anim.currentAnimationId);
            if (!cur.frames.empty())
                visualizer.drawTexture(cur.frames[cur.currentFrame],
                                       screenX, screenY, 45, 45);
        }
    }
}

// ─────────────────────────────────────────
// Update
// ─────────────────────────────────────────

void UIManager::update(float dt) {
    const std::string& sceneId = gameStateString.at(game.getGameState());
    Scene* scene = findScene(sceneId);
    if (!scene) return;

    updateSceneData(sceneId, *scene);

    if (sceneId == "Map")
        updatePlayerAnimation(scene->animPlayer, player);

    scene->animPlayer.update(dt);
}

void UIManager::updateSceneData(const std::string& sceneId, Scene& scene) {
    setTexture("Time", sceneId,
        ResourceManager::getTextTexture(game.getCurrentTime(),
            ResourceManager::getFont("RetroByte"), {0,0,0,0}));
    setTexture("FPS", sceneId,
        ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()),
            ResourceManager::getFont("RetroByte"), {0,0,0,0}));

    if (sceneId == "Map")
        updateMapData(scene);
    else if (sceneId == "Battle")
        updateBattleData(scene);
}

void UIManager::updateMapData(Scene& scene) {
    Point pos = player.getPlayerCoords();
    if (auto* obj = scene.find("MapBackground")) {
        obj->rect.x = pos.x - 900;
        obj->rect.y = pos.y - 500;
    }
}

void UIManager::updateBattleData(Scene& scene) {
    const SDL_Rect scr    = game.getScreenRect();
    const int enemyCount  = static_cast<int>(game.getBattle()->getEnemies().size());
    const int heroCount   = static_cast<int>(player.getHeroes().size());

    // Герои
    std::string heroText;
    for (int i = 0; i < heroCount; i++) {
        const auto& hero = player.getHeroes().at(i);
        heroText += "Hero" + std::to_string(i + 1) + " "
                  + std::to_string(hero.getCurrentHp()) + "/"
                  + std::to_string(hero.getMaxHp()) + "\n";

        bool dead = hero.getCurrentHp() <= 0;
        SDL_Rect drect = dead
            ? SDL_Rect{scr.w - 100, scr.h/2 - 230 + i * 100, 66, 84}
            : SDL_Rect{scr.w - 100, scr.h/2 - 230 + i * 100, 56, 74};

        std::string imgId = "Image" + std::to_string(i + 1) + (dead ? "Dead" : "");
        setRect   ("Hero" + std::to_string(i + 1), "Battle", drect);
        setTexture("Hero" + std::to_string(i + 1), "Battle",
                   ResourceManager::getTexture(imgId));
    }
    setTexture("HerosStatus", "Battle",
        ResourceManager::getTextTexture(heroText,
            ResourceManager::getFont("RetroByte"), {0,0,0,0}, 245));

    // Враги
    std::string enemyText;
    for (int i = 0; i < enemyCount; i++) {
        const auto& enemy = game.getBattle()->getEnemies().at(i);
        if (!enemy.getIsAlive())
            setTexture("Enemy" + std::to_string(i + 1), "Battle", nullptr);
        enemyText += "Enemy" + std::to_string(i + 1) + " "
                   + std::to_string(enemy.getCurrentHp()) + "/"
                   + std::to_string(enemy.getMaxHp()) + "\n";
    }
    SDL_Rect eRect = getRect("EnemiesStatus", "Battle");
    eRect.h = enemyCount * 40;
    setRect   ("EnemiesStatus", "Battle", eRect);
    setTexture("EnemiesStatus", "Battle",
        ResourceManager::getTextTexture(enemyText,
            ResourceManager::getFont("RetroByte"), {0,0,0,0}, 245));
}

// ─────────────────────────────────────────
// Battle helpers
// ─────────────────────────────────────────

void UIManager::addEnemys() {
    for (int i = 0; i < (int)game.getBattle()->getEnemies().size(); i++)
        setTexture("Enemy" + std::to_string(i + 1), "Battle",
            ResourceManager::getTexture(game.getBattle()->getEnemies().at(i).getName()));
}

void UIManager::addCharacters() {
    for (int i = 0; i < MAX_HERO_COUNT; i++)
        setTexture("Hero" + std::to_string(i + 1), "Battle",
            ResourceManager::getTexture("Image" + std::to_string(i + 1)));
}

// ─────────────────────────────────────────
// Battle UI — action flow
// ─────────────────────────────────────────

void UIManager::onActionButton(ActionType type) {
    pendingAction            = Action{};
    currentHeroIndex         = game.getBattle()->getCurrentHeroIndex();
    pendingAction.actorIndex = currentHeroIndex;
    pendingAction.type       = type;

    switch (type) {
        case ActionType::Attack:
            pendingAction.targetType = TargetType::Enemy;
            uiState = UIChooseState::ChooseTarget;
            moveSelector();
            setEnVI("ActionPanel",    "Battle", false);  // ← только здесь
            setEnVI("ActorSelector",  "Battle", true);
            setEnVI("TargetSelector", "Battle", true);
            break;

        case ActionType::Magic:
            uiState = UIChooseState::ChooseSkill;
            initMagic();
            setEnVI("EnvironmentPanel", "Battle", true);
            // ActionPanel НЕ скрываем — текст внизу остаётся
            break;

        case ActionType::Item:
            uiState = UIChooseState::ChooseItem;
            initItems();
            setEnVI("EnvironmentPanel", "Battle", true);
            // ActionPanel НЕ скрываем — текст внизу остаётся
            break;

        case ActionType::Skip:
            game.getBattle()->confirmAction(pendingAction);
            uiState = UIChooseState::ChooseAction;
            setEnVI("ActionPanel", "Battle", true);
            break;
    }
}

void UIManager::initMagic() {
    const auto& skills = player.getHeroes().at(currentHeroIndex).getAvailableSkills();
    for (int i = 0; i < (int)skills.size() && i < ENV_BUTTON_COUNT; i++) {
        const std::string btnId = "EnButton" + std::to_string(i + 1);
        const std::string name  = game.getSkill(skills.at(i).skillId).name;
        setTexture(btnId, "Battle",
            ResourceManager::getTextTexture(name,
                ResourceManager::getFont("RetroByte"), {0,0,0,0}));
        setOnClick(btnId, "Battle",
            [this, skillId = skills.at(i).skillId]() {
                onEnvironmentCLick(skillId, true);
            });
        setEnVI(btnId, "Battle", true);
    }
}

void UIManager::initItems() {
    const auto& items = player.getAvailableItems();
    for (int i = 0; i < (int)items.size() && i < ENV_BUTTON_COUNT; i++) {
        const std::string btnId = "EnButton" + std::to_string(i + 1);
        const std::string name  = game.getItem(items.at(i)).name;
        setTexture(btnId, "Battle",
            ResourceManager::getTextTexture(name,
                ResourceManager::getFont("RetroByte"), {0,0,0,0}));
        setOnClick(btnId, "Battle",
            [this, itemId = items.at(i)]() {
                onEnvironmentCLick(itemId, false);
            });
        setEnVI(btnId, "Battle", true);
    }
}

void UIManager::onEnvironmentCLick(int id, bool isMagic) {
    pendingAction            = Action{};
    pendingAction.actorIndex = game.getBattle()->getCurrentHeroIndex();
    pendingAction.payloadId  = id;

    if (isMagic) {
        pendingAction.type       = ActionType::Magic;
        pendingAction.targetType = game.getSkill(id).targetType;
    } else {
        pendingAction.type       = ActionType::Item;
        pendingAction.targetType = game.getItem(id).targetType;
    }

    if (pendingAction.targetType == TargetType::Self ||
       pendingAction.targetType == TargetType::AllEnemies ||
       pendingAction.targetType == TargetType::AllAllies)
    {
        game.getBattle()->confirmAction(pendingAction);
        setEnVI("EnvironmentPanel", "Battle", false);
        uiState = UIChooseState::ChooseAction;
        setEnVI("ActionPanel", "Battle", true);
        return;
    }

    uiState = UIChooseState::ChooseTarget;
    moveSelector();
    setEnVI("ActionPanel",      "Battle", false);  // ← скрываем только при переходе к цели
    setEnVI("ActorSelector",    "Battle", true);
    setEnVI("TargetSelector",   "Battle", true);
    setEnVI("EnvironmentPanel", "Battle", false);
}

void UIManager::confirmTarget() {
    pendingAction.targetIndex = selectedTargetIndex;

    setEnVI("TargetSelector",   "Battle", false);
    setEnVI("ActorSelector",    "Battle", false);
    setEnVI("EnvironmentPanel", "Battle", false);

    game.getBattle()->confirmAction(pendingAction);

    uiState = UIChooseState::ChooseAction;
    setEnVI("ActionPanel", "Battle", true);
}

// ─────────────────────────────────────────
// Selector
// ─────────────────────────────────────────

int UIManager::getTargetCount(TargetType type) const {
    switch (type) {
        case TargetType::Enemy: return (int)game.getBattle()->getEnemies().size();
        case TargetType::Ally:  return (int)player.getHeroes().size();
        default:                return 0;
    }
}

void UIManager::moveSelector() {
    currentType         = pendingAction.targetType;
    selectedTargetIndex = 0;

    SDL_Rect targetRect{};
    SDL_Rect actorRect = getRect("Hero" + std::to_string(currentHeroIndex + 1), "Battle");

    if (currentType == TargetType::Enemy) {
        for (int i = 0; i < (int)game.getBattle()->getEnemies().size(); i++) {
            if (game.getBattle()->getEnemies().at(i).getIsAlive()) {
                targetRect = getRect("Enemy" + std::to_string(i + 1), "Battle");
                selectedTargetIndex = i;
                break;
            }
        }
    } else if (currentType == TargetType::Ally) {
        for (int i = 0; i < (int)player.getHeroes().size(); i++) {
            if (player.getHeroes().at(i).getIsAlive()) {
                targetRect = getRect("Hero" + std::to_string(i + 1), "Battle");
                selectedTargetIndex = i;
                break;
            }
        }
    } else if (currentType == TargetType::Self) {
        targetRect = actorRect;
    }

    // Защита — если rect нулевой, цель ещё не готова
    if (targetRect.w == 0 || targetRect.h == 0) {
        SDL_Log("moveSelector: target rect is zero, skipping");
        return;
    }
    if (actorRect.w == 0 || actorRect.h == 0) {
        SDL_Log("moveSelector: actor rect is zero, skipping");
        return;
    }

    SDL_Rect tSel = getRect("TargetSelector", "Battle");
    SDL_Rect aSel = getRect("ActorSelector",  "Battle");

    tSel.x = targetRect.x + targetRect.w / 2 - tSel.w / 2;
    tSel.y = targetRect.y - targetRect.h;
    aSel.x = actorRect.x  + actorRect.w  / 2 - aSel.w / 2;
    aSel.y = actorRect.y  - actorRect.h;

    setRect("TargetSelector", "Battle", tSel);
    setRect("ActorSelector",  "Battle", aSel);
}

void UIManager::moveSelectorToNext() {
    auto* sel = findUIObject("TargetSelector", "Battle");
    if (!sel || !sel->isEnabled) return;

    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex + 1) % max;
    snapSelectorToTarget();
}

void UIManager::moveSelectorToPrevious() {
    auto* sel = findUIObject("TargetSelector", "Battle");
    if (!sel || !sel->isEnabled) return;

    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex - 1 + max) % max;
    snapSelectorToTarget();
}

void UIManager::snapSelectorToTarget() {
    const int max = getTargetCount(currentType);
    SDL_Rect targetRect{};

    if (currentType == TargetType::Enemy || currentType == TargetType::AllEnemies) {
        for (int guard = 0; guard < max; guard++) {
            if (game.getBattle()->getEnemies().at(selectedTargetIndex).getIsAlive()) break;
            selectedTargetIndex = (selectedTargetIndex + 1) % max;
        }
        targetRect = getRect("Enemy" + std::to_string(selectedTargetIndex + 1), "Battle");
    } else if (currentType == TargetType::Ally || currentType == TargetType::AllAllies) {
        for (int guard = 0; guard < max; guard++) {
            if (player.getHeroes().at(selectedTargetIndex).getIsAlive()) break;
            selectedTargetIndex = (selectedTargetIndex + 1) % max;
        }
        targetRect = getRect("Hero" + std::to_string(selectedTargetIndex + 1), "Battle");
    }

    if (targetRect.w == 0 || targetRect.h == 0) {
        SDL_Log("snapSelectorToTarget: target rect is zero, skipping");
        return;
    }

    SDL_Rect sel = getRect("TargetSelector", "Battle");
    sel.x = targetRect.x + targetRect.w / 2 - sel.w / 2;
    sel.y = targetRect.y - targetRect.h;
    setRect("TargetSelector", "Battle", sel);
}

// ─────────────────────────────────────────
// Player animation
// ─────────────────────────────────────────

void UIManager::updatePlayerAnimation(AnimPlayer& animPlayer, const Player& player) {
    if (player.getDirection() == Direction::Idle) {
        animPlayer.stopLoop();
        currentPlayerAnim.clear();
        setEnVI("Character", "Map", true);
        return;
    }

    setEnVI("Character", "Map", false);

    static const std::unordered_map<Direction, std::string> dirToAnim = {
        { Direction::Up,    "MoveUpAnimation"    },
        { Direction::Down,  "MoveDownAnimation"  },
        { Direction::Left,  "MoveLeftAnimation"  },
        { Direction::Right, "MoveRightAnimation" },
    };

    auto it = dirToAnim.find(player.getDirection());
    if (it == dirToAnim.end()) return;

    if (currentPlayerAnim != it->second) {
        currentPlayerAnim = it->second;
        playAnimation(currentPlayerAnim, "Map", true);
    }
}

void UIManager::updateRemoteAnimation(AnimPlayer& animPlayer, const Player& player) {
    if (player.getDirection() == Direction::Idle) {
        animPlayer.stopLoop();
        return;
    }

    static const std::unordered_map<Direction, std::string> dirToAnim = {
        { Direction::Up,    "MoveUpAnimation"    },
        { Direction::Down,  "MoveDownAnimation"  },
        { Direction::Left,  "MoveLeftAnimation"  },
        { Direction::Right, "MoveRightAnimation" },
    };

    auto it = dirToAnim.find(player.getDirection());
    if (it != dirToAnim.end())
        animPlayer.play(it->second, false);
}

// ─────────────────────────────────────────
// Initialize — по одному методу на сцену
// ─────────────────────────────────────────

void UIManager::initialize() {
    screen = game.getScreenRect();
    addScene(gameStateString.at(GameState::Battle));
    addScene(gameStateString.at(GameState::Options));
    addScene(gameStateString.at(GameState::Menu));
    addScene(gameStateString.at(GameState::CreatePlayer));
    addScene(gameStateString.at(GameState::Map));
    addScene(gameStateString.at(GameState::Inventory));
    addScene(gameStateString.at(GameState::Online));

    initMapScene();
    initMenuScene();
    initOptionsScene();
    initOnlineScene();
    initBattleScene();
}

void UIManager::initMapScene() {
    TTF_Font* font = ResourceManager::getFont("RetroByte");
    addImage("MapBackground", "Map",
             {-900, -500, 18000, 18000}, ResourceManager::getTexture("MapBackground"));
    addImage("Character", "Map",
             {screen.w/2 - 25, screen.h/2 - 25, 45, 45},
             ResourceManager::getTexture("MovingCharacter"), {0,0,16,25});
    addImage("Press E", "Map",
             {screen.w/2 - 25, screen.h/2 - 50, 60, 20},
             ResourceManager::getTextTexture("Press E", font, {0,0,0,255}));
    addImage("FPS",  "Map", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0,0,0,0}));
    addImage("Time", "Map", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0,0,0,0}));

    addAnimation("MoveLeftAnimation",  "Map",
                 ResourceManager::getAnimation("MoveLeftAnimation"),  {0,0,45,45}, 0.2f);
    addAnimation("MoveRightAnimation", "Map",
                 ResourceManager::getAnimation("MoveRightAnimation"), {0,0,45,45}, 0.2f);
    addAnimation("MoveUpAnimation",    "Map",
                 ResourceManager::getAnimation("MoveUpAnimation"),    {0,0,45,45}, 0.2f);
    addAnimation("MoveDownAnimation",  "Map",
                 ResourceManager::getAnimation("MoveDownAnimation"),  {0,0,45,45}, 0.2f);

    setVisible("Press E", "Map", false);
    addMusic("MapTheme", "Map");
}

void UIManager::initMenuScene() {
    TTF_Font* font = ResourceManager::getFont("RetroByte");
    addImage("MenuBackground", "Menu", screen, ResourceManager::getTexture("MenuBackground"));
    addImage("Time", "Menu", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0,0,0,0}));
    addMusic("MenuTheme", "Menu");
    addSound("ButtonHover", "Menu");
    addPanel("ButtonPanel", "Menu");

    auto hover = [this]() { playSound("Menu", "ButtonHover"); };
    addButton("Online",   "Menu", {190, screen.h/2 + 30,  170, 50},
              ResourceManager::getTextTexture("Co-op",    font, {0,0,0,0}),
              [this]() { game.openOnlineMenu(); }, hover, "ButtonPanel");
    addButton("Continue", "Menu", {190, screen.h/2 + 80,  170, 50},
              ResourceManager::getTextTexture("Continue", font, {0,0,0,0}),
              [this]() { game.startGame(); },      hover, "ButtonPanel");
    addButton("New game", "Menu", {190, screen.h/2 + 130, 170, 50},
              ResourceManager::getTextTexture("New game", font, {0,0,0,0}),
              [this]() { game.startGame(); },      hover, "ButtonPanel");
    addButton("Options",  "Menu", {190, screen.h/2 + 180, 150, 50},
              ResourceManager::getTextTexture("Options",  font, {0,0,0,0}),
              [this]() { game.openOptions(GameState::Menu); }, hover, "ButtonPanel");
    addButton("Quit",     "Menu", {190, screen.h/2 + 230, 100, 50},
              ResourceManager::getTextTexture("Quit",     font, {0,0,0,0}),
              [this]() { game.endGame(); },        hover, "ButtonPanel");
}

void UIManager::initOptionsScene() {
    TTF_Font* font = ResourceManager::getFont("RetroByte");
    const std::string text =
        "Options:\nOn Map:\nFor moving use WASD or arrows\n"
        "In Battle:\nTo select target use WASD or arrows\n"
        "To confirm the target use SPACE or ENTER(RETURN)";

    addImage("OptionsBackground", "Options", screen,
             ResourceManager::getTexture("OptionsBackground"), {0,0,1920,1000});
    addImage("InstructionText", "Options",
             {screen.w/2 - 350, screen.h/2 - 150, 700, 240},
             ResourceManager::getTextTexture(text, font, {0,0,0,0}, screen.w - 200));
    addImage("Time", "Options", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0,0,0,0}));
    addButton("BackButton", "Options",
              {screen.w/2 - 110, screen.h - 100, 220, 90},
              ResourceManager::getTextTexture("Back to menu", font, {0,0,0,0}),
              [this]() { game.closeOptions(); },
              [this]() { playSound("Menu", "ButtonHover"); });
    addMusic("OptionsTheme", "Options");
}

void UIManager::initOnlineScene() {
    TTF_Font* font  = ResourceManager::getFont("RetroByte");
    auto hover      = [this]() { playSound("Menu", "ButtonHover"); };

    addMusic("OnlineTheme", "Online");
    addImage("OnlineBackground", "Online", screen,
             ResourceManager::getTexture("OptionsBackground"), {0,0,1920,1000});
    addImage("Time", "Online", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0,0,0,0}));

    addImage("IPImage", "Online",
             {screen.w - 360, screen.h/2 - 25, 170, 50},
             ResourceManager::getTextTexture("Your IP: " + game.getIP(), font, {0,0,0,0}));
    addButton("StartServerButton", "Online",
              {screen.w - 360, screen.h/2 + 30, 170, 50},
              ResourceManager::getTextTexture("Start server", font, {0,0,0,0}),
              [this]() { game.startServer(); }, hover);

    addImage("ConnectImage", "Online",
             {190, screen.h/2 - 25, 170, 50},
             ResourceManager::getTextTexture("Enter friend IP", font, {0,0,0,0}));
    addEdit("IPEdit", "Online", {190, screen.h/2 + 30, 170, 50},
            font, {0,0,0,255},
            [this]() { setFocus("IPEdit", "Online", true); }, hover);
    addButton("ConnectButton", "Online",
              {190, screen.h/2 + 85, 170, 50},
              ResourceManager::getTextTexture("Connect", font, {0,0,0,0}),
              [this]() { game.connectToServer(getText("IPEdit", "Online")); }, hover);
}

void UIManager::initBattleScene() {
    TTF_Font* font = ResourceManager::getFont("RetroByte");
    auto hover     = [this]() { playSound("Battle", "ButtonHover"); };

    addImage("BattleBackground",  "Battle",
             {0, 0, screen.w, 452},          ResourceManager::getTexture("BattleImage0"));
    addImage("DownMenuBackground","Battle",
             {0, 452, screen.w, screen.h-452}, ResourceManager::getTexture("BattleTile"));
    addImage("FPS",  "Battle", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0,0,0,0}));
    addImage("Time", "Battle", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0,0,0,0}));

    addMusic("BattleTheme", "Battle");

    addPanel("ActionPanel",      "Battle");
    addPanel("EnvironmentPanel", "Battle");
    addPanel("EnemyPanel",       "Battle");
    addPanel("HeroPanel",        "Battle");

    addButton("AttackButton", "Battle", {0, 453, 60, 25},
              ResourceManager::getTextTexture("Attack", font, {0,0,0,0}),
              [this]() { onActionButton(ActionType::Attack); }, hover, "ActionPanel");
    addButton("ItemsButton",  "Battle", {0, 478, 60, 25},
              ResourceManager::getTextTexture("Items",  font, {0,0,0,0}),
              [this]() { onActionButton(ActionType::Item); },   hover, "ActionPanel");
    addButton("MagicButton",  "Battle", {0, 503, 60, 25},
              ResourceManager::getTextTexture("Magic",  font, {0,0,0,0}),
              [this]() { onActionButton(ActionType::Magic); },  hover, "ActionPanel");
    addButton("Run",          "Battle", {0, 528, 60, 20},
              ResourceManager::getTextTexture("Run",    font, {0,0,0,0}),
              [this]() { game.endRandomBattle(); },             hover, "ActionPanel");

    addImage("TargetSelector", "Battle", {0,0,100,100},
             ResourceManager::getTexture("TargetSelector"), {0,0,100,100});
    addImage("ActorSelector",  "Battle", {0,0,50,50},
             ResourceManager::getTexture("ActorSelector"),  {0,0,100,100});
    setVisible("TargetSelector", "Battle", false);
    setVisible("ActorSelector",  "Battle", false);

    for (int i = 0; i < 4; i++) {
        addImage("Enemy" + std::to_string(i+1), "Battle",
                 {screen.w/2-100, screen.h/2-230 + i*100, 76, 94},
                 nullptr, {0,0,108,144}, "EnemyPanel");
        addImage("Hero"  + std::to_string(i+1), "Battle",
                 {screen.w-100,   screen.h/2-230 + i*100, 56, 74},
                 nullptr, {0,0,16,24},  "HeroPanel");
    }

    // EnButtons — равномерно, без магических шагов
    for (int i = 0; i < ENV_BUTTON_COUNT; i++) {
        int row = i < ENV_BUTTON_COUNT / 2 ? 452 : 480;
        int col = (i % (ENV_BUTTON_COUNT / 2)) * 60 + 60;
        addButton("EnButton" + std::to_string(i+1), "Battle",
                  {col, row, 60, 25}, nullptr, nullptr, nullptr, "EnvironmentPanel");
    }
    setEnVI("EnvironmentPanel", "Battle", false);

    addImage("HerosStatus",   "Battle",
             {screen.w-600, 452, 300, screen.h-452}, nullptr);
    addImage("EnemiesStatus", "Battle",
             {screen.w-300, 452, 300, screen.h-452}, nullptr);

    for (int i = 0; i < (int)player.getHeroes().size(); i++) {
        addAnimation("Hero" + std::to_string(i+1) + "Animation", "Battle",
            ResourceManager::getAnimation("Hero" + std::to_string(i+1) + "Animation"),
            { {screen.w-100, screen.h/2-230+i*100, 56, 94},
              {screen.w-150, screen.h/2-230+i*100, 56, 94},
              {screen.w-100, screen.h/2-230+i*100, 56, 94} }, 0.2f);
    }
}