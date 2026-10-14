#include <cmath>

#include "UIManager.h"
#include "../main/game.h"
#include <SDL_log.h>

constexpr int ENV_BUTTON_COUNT = 16;

// ─────────────────────────────────────────
// State
// ─────────────────────────────────────────

void UIManager::setState(const UIChooseState &state) { uiState = state; }
UIChooseState UIManager::getState() const { return uiState; }

// ─────────────────────────────────────────
// Scene lookup
// ─────────────────────────────────────────

Scene *UIManager::findScene(const std::string &id)
{
    auto it = scenes.find(id);
    if (it == scenes.end())
    {
        SDL_Log("UIManager: scene '%s' not found", id.c_str());
        return nullptr;
    }
    return &it->second;
}

UIObject *UIManager::findUIObject(const std::string &id, const std::string &sceneId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return nullptr;

    UIObject *obj = scene->find(id); // рекурсивный поиск в Panel
    if (!obj)
        SDL_Log("UIManager: element '%s' not found in scene '%s'",
                id.c_str(), sceneId.c_str());
    return obj;
}

// ─────────────────────────────────────────
// Scene management
// ─────────────────────────────────────────

void UIManager::addScene(const std::string &id) { if (!scenes.contains(id)) { scenes[id].name = id; } }

// ─────────────────────────────────────────
// Widget добавление — делегируем в Panel
// ─────────────────────────────────────────

void UIManager::addPanel(const std::string &id, const std::string &sceneId,
                         const std::string &panelId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    Panel *target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target)
    {
        SDL_Log("addPanel: panel '%s' not found", panelId.c_str());
        return;
    }
    target->addPanel(id);
}

void UIManager::addButton(const std::string &id, const std::string &sceneId,
                          const SDL_Rect &rect, SDL_Texture *texture,
                          std::function<void()> onClick, std::function<void()> onHover,
                          const std::string &panelId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    Panel *target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target)
    {
        SDL_Log("addButton: panel '%s' not found", panelId.c_str());
        return;
    }
    target->addButton(id, rect, texture, std::move(onClick), std::move(onHover));
}

void UIManager::addEdit(const std::string &id, const std::string &sceneId,
                        const SDL_Rect &rect, TTF_Font *font, SDL_Color color,
                        std::function<void()> onClick, std::function<void()> onHover,
                        const std::string &panelId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    Panel *target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target)
    {
        SDL_Log("addEdit: panel '%s' not found", panelId.c_str());
        return;
    }
    target->addEdit(id, rect, font, color, std::move(onClick), std::move(onHover));
}

void UIManager::addImage(const std::string &id, const std::string &sceneId,
                         const SDL_Rect &rect, SDL_Texture *texture,
                         SDL_Rect srect, const std::string &panelId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    Panel *target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target)
    {
        SDL_Log("addImage: panel '%s' not found", panelId.c_str());
        return;
    }
    target->addImage(id, rect, texture, srect);
}

void UIManager::addLabel(const std::string &id, const std::string &sceneId,
                         const SDL_Rect &bgRect, SDL_Texture *bgTexture,
                         const std::string &text, TTF_Font *font,
                         SDL_Color color, int paddingX, int paddingY,
                         const std::string &panelId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    Panel *target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target)
    {
        SDL_Log("addLabel: panel '%s' not found", panelId.c_str());
        return;
    }
    target->addLabel(id, bgRect, bgTexture, text, font, color, paddingX, paddingY);
}
void UIManager::addWindow(const std::string &id, const std::string &sceneId,
                          const SDL_Rect &rect, const std::string &panelId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    Panel *target = panelId.empty() ? scene : scene->findPanel(panelId);
    if (!target)
    {
        SDL_Log("addWindow: panel '%s' not found", panelId.c_str());
        return;
    }
    target->addWindow(id, rect);
}
// ─────────────────────────────────────────
// Audio
// ─────────────────────────────────────────

void UIManager::addMusic(const std::string &musicId, const std::string &sceneId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    Mix_Music *m = ResourceManager::getMusic(musicId);
    if (!m)
    {
        SDL_Log("addMusic: resource '%s' not found", musicId.c_str());
        return;
    }
    scene->musicPlayer.music[musicId] = m;
}

void UIManager::addSound(const std::string &soundId, const std::string &sceneId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    Mix_Chunk *c = ResourceManager::getSound(soundId);
    if (!c)
    {
        SDL_Log("addSound: resource '%s' not found", soundId.c_str());
        return;
    }
    scene->musicPlayer.sound[soundId] = c;
}

void UIManager::playSound(const std::string &sceneId, const std::string &soundId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    scene->musicPlayer.playSound(audioManager, soundId);
}

// ─────────────────────────────────────────
// Animation
// ─────────────────────────────────────────

void UIManager::addAnimation(const std::string &id, const std::string &sceneId,
                             std::vector<SDL_Texture *> frames,
                             std::vector<SDL_Rect> rects,
                             float frameTime, AnimPlayMode mode)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    scene->animPlayer.animations[id] = {frameTime, std::move(frames), std::move(rects)};
}

void UIManager::addAnimation(const std::string &id, const std::string &sceneId,
                             std::vector<SDL_Texture *> frames,
                             SDL_Rect rect, float frameTime, AnimPlayMode mode)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    scene->animPlayer.animations[id] = {
        frameTime, std::move(frames), {}, rect, 0, 0.0f, false, mode
    };
}

void UIManager::playAnimation(const std::string &animId,
                              const std::string &sceneId, bool restart)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    scene->animPlayer.play(animId, restart);
}

bool UIManager::isAnimationFinished(const std::string &sceneId) const
{
    auto it = scenes.find(sceneId);
    if (it == scenes.end()) return true;
    const AnimPlayer &p = it->second.animPlayer;
    return p.currentAnimationId.empty() && p.animationQueue.empty();
}

// ─────────────────────────────────────────
// Setters / Getters
// ─────────────────────────────────────────

void UIManager::setVisible(const std::string &id, const std::string &sceneId, bool visible)
{
    if (auto *obj = findUIObject(id, sceneId))
    {
        obj->isVisible = visible;
        obj->isEnabled = visible;
    }
}

void UIManager::setEnabled(const std::string &id, const std::string &sceneId, bool enabled)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->isEnabled = enabled;
}

void UIManager::setEnVI(const std::string &id, const std::string &sceneId, bool value)
{
    if (auto *obj = findUIObject(id, sceneId))
    {
        obj->isEnabled = value;
        obj->isVisible = value;
    }
}

void UIManager::setTexture(const std::string &id, const std::string &sceneId,
                           SDL_Texture *texture)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->texture = texture;
}

void UIManager::setRect(const std::string &id, const std::string &sceneId,
                        const SDL_Rect &rect)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->rect = rect;
}

SDL_Rect UIManager::getRect(const std::string &id, const std::string &sceneId)
{
    if (auto *obj = findUIObject(id, sceneId))
        return obj->rect;
    return {0, 0, 0, 0};
}

void UIManager::setSrcRect(const std::string &id, const std::string &sceneId,
                           const SDL_Rect &srect)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->setSrcRect(srect); // виртуальный метод — no-op если не Image
}

void UIManager::setText(const std::string &id, const std::string &sceneId,
                        const std::string &text)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->setText(text);
}

std::string UIManager::getText(const std::string &id, const std::string &sceneId)
{
    if (auto *obj = findUIObject(id, sceneId))
        return obj->getText();
    return "";
}

void UIManager::setFocus(const std::string &id, const std::string &sceneId, bool focus)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->setFocus(focus);
}

bool UIManager::getFocus(const std::string &id, const std::string &sceneId)
{
    if (auto *obj = findUIObject(id, sceneId))
        return obj->getFocus();
    return false;
}

void UIManager::setPos(const std::string &id, const std::string &sceneId, int pos)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->setPos(pos);
}

int UIManager::getPos(const std::string &id, const std::string &sceneId)
{
    if (auto *obj = findUIObject(id, sceneId))
        return obj->getPos();
    return 0;
}

void UIManager::setOnClick(const std::string &id, const std::string &sceneId,
                           std::function<void()> onClick)
{
    if (auto *obj = findUIObject(id, sceneId))
        obj->setOnClick(std::move(onClick));
}

// ─────────────────────────────────────────
// Events
// ─────────────────────────────────────────

void UIManager::handleClickEvent(const SDL_Event &event, const std::string &sceneId)
{
    if (event.type != SDL_MOUSEBUTTONDOWN) return;

    if (sceneId == "Battle" && !isAnimationFinished("Battle"))
        return;

    Scene *scene = findScene(sceneId);
    if (!scene) return;
    scene->handleClick(event.button.x, event.button.y);
}

void UIManager::handleHoverEvent(const SDL_Event &event, const std::string &sceneId)
{
    if (event.type != SDL_MOUSEMOTION) return;
    Scene *scene = findScene(sceneId);
    if (!scene) return;
    scene->handleHover(event.motion.x, event.motion.y);
}

// ─────────────────────────────────────────
// Draw
// ─────────────────────────────────────────


namespace {
int px(float v) { return static_cast<int>(std::lround(v)); }

SDL_Texture* idleFrame(Direction d)     // стоячий кадр = первый кадр анимации ходьбы в этом направлении
{
    static const std::unordered_map<Direction, const char*> names = {
        {Direction::Up, "MoveUpAnimation"},   {Direction::Down, "MoveDownAnimation"},
        {Direction::Left, "MoveLeftAnimation"}, {Direction::Right, "MoveRightAnimation"} };
    auto it = names.find(d);
    if (it == names.end()) return nullptr;
    const auto& frames = ResourceManager::getAnimation(it->second);
    return frames.empty() ? nullptr : frames.front();
}
}

void UIManager::drawActor(const Player& p, int cellScreenX, int cellScreenY)
{
    SDL_Texture* tex = nullptr;

    const AnimPlayer& ap = p.animPlayer;
    if (p.getDirection() != Direction::Idle && !ap.currentAnimationId.empty())
    {
        auto it = ap.animations.find(ap.currentAnimationId);
        if (it != ap.animations.end() && it->second.currentFrame < it->second.frames.size())
            tex = it->second.frames[it->second.currentFrame];
    }
    if (!tex) tex = idleFrame(p.getMove().dir);

    // ноги в центре нижнего края клетки; герой 16x24 при scale 3 выходит на 72 px, то есть на 24 px выше клетки
    visualizer.drawSprite(tex, cellScreenX + CELL_PX / 2, cellScreenY + CELL_PX, PIXEL_SCALE);
}

void UIManager::drawMapWorld()
{
    const SDL_Rect scr = game.getScreenRect();
    const MapData& map = game.getMap();
    const MoveState& localMove = player.getMove();
    const Vec2f lp = visualCell(localMove);

    auto camAxis = [](int cam, int mapPx, int view) {
        if (mapPx <= view) return -(view - mapPx) / 2;
        return std::clamp(cam, 0, mapPx - view);
    };

    const int camX = camAxis(px(lp.x * CELL_PX) - (scr.w - CELL_PX) / 2, map.w * CELL_PX, scr.w);
    const int camY = camAxis(px(lp.y * CELL_PX) - (scr.h - CELL_PX) / 2, map.h * CELL_PX, scr.h);

    if (MapRenderer* r = rendererFor(map))
        r->draw(visualizer.getRenderer(), camX, camY, scr.w, scr.h, PIXEL_SCALE);

    struct Actor { const Player* p; Vec2f pos; };
    std::vector<Actor> actors;
    actors.push_back({ &player, lp });
    for (const auto& [id, remote] : game.getRemotePlayers())
        if (remote.getMove().mapId == localMove.mapId)           // игроки с другой карты не видны
            actors.push_back({ &remote, visualCell(remote.getMove()) });

    std::sort(actors.begin(), actors.end(), [](const Actor& a, const Actor& b) { return a.pos.y < b.pos.y; });

    for (const Actor& a : actors)
        drawActor(*a.p, px(a.pos.x * CELL_PX) - camX, px(a.pos.y * CELL_PX) - camY);
}

void UIManager::drawScene(const std::string &sceneId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    if (sceneId == "Map")
        drawMapWorld();                               // под интерфейсом

    scene->draw(visualizer, game.getScreenRect());
    scene->musicPlayer.playMusic(audioManager, sceneId + "Theme");
}
// ─────────────────────────────────────────
// Update
// ─────────────────────────────────────────

void UIManager::update(float dt)
{
    const std::string &sceneId = gameStateString.at(game.getGameState());
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    updateSceneData(sceneId, *scene);          // без этого вызова время и FPS замирают
    scene->animPlayer.update(dt);
}

void UIManager::updateSceneData(const std::string &sceneId, Scene &scene)
{
    setTexture("Time", sceneId,
               ResourceManager::getTextTexture(Game::getCurrentTime(),
                                               ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    setTexture("FPS", sceneId,
               ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()),
                                               ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));

    if (sceneId == "Battle")                   // ветка для Map больше не нужна
        updateBattleData(scene);
}

void UIManager::updateBattleData(Scene &scene)
{
    const SDL_Rect scr = game.getScreenRect();
    const int enemyCount = static_cast<int>(game.getBattle()->getEnemies().size());
    const int heroCount = static_cast<int>(player.getHeroes().size());

    //Remote Hero
    if (game.getRemotePlayers().size() > 0)
    {
        // Герои удалённых игроков (раньше: цикл по числу героев ЛОКАЛЬНОГО игрока и [i] у последнего удалённого, выход за границу)
        std::string remoteHeroText;
        int slot = 0;
        for (const auto &[id, remote] : game.getRemotePlayers())
        {
            if (!game.getBattle()->isParticipant(id)) continue;

            const auto &heroes = remote.getHeroes();
            for (int i = 0; i < static_cast<int>(heroes.size()) && slot < MAX_HERO_COUNT; ++i, ++slot)
            {
                const auto &hero = heroes[i];
                remoteHeroText += "Remote" + hero.getName() + " "
                        + std::to_string(hero.getCurrentHp()) + "/"
                        + std::to_string(hero.getMaxHp()) + "\n";

                const bool dead = hero.getCurrentHp() <= 0;
                const SDL_Rect drect = dead
                    ? SDL_Rect{scr.w - 200, scr.h / 2 - 230 + slot * 100, 66, 84}
                : SDL_Rect{scr.w - 200, scr.h / 2 - 230 + slot * 100, 56, 74};

                const std::string name = "RemoteHero" + std::to_string(slot + 1);
                setRect(name, "Battle", drect);
                setTexture(name, "Battle",
                           ResourceManager::getTexture("Image" + std::to_string(i + 1) + (dead ? "Dead" : "")));
            }
        }
        if (slot > 0)
            setText("RemoteHerosStatus", "Battle", remoteHeroText);
    }


    // Герои
    std::string heroText;
    for (int i = 0; i < heroCount; i++)
    {
        const auto &hero = player.getHeroes().at(i);
        heroText += hero.getName() + " "
                + std::to_string(hero.getCurrentHp()) + "/"
                + std::to_string(hero.getMaxHp()) + "\n";

        bool dead = hero.getCurrentHp() <= 0;
        SDL_Rect drect = dead
                             ? SDL_Rect{scr.w - 100, scr.h / 2 - 230 + i * 100, 66, 84}
                             : SDL_Rect{scr.w - 100, scr.h / 2 - 230 + i * 100, 56, 74};

        std::string imgId = "Image" + std::to_string(i + 1) + (dead ? "Dead" : "");
        setRect("Hero" + std::to_string(i + 1), "Battle", drect);
        setTexture("Hero" + std::to_string(i + 1), "Battle",
                   ResourceManager::getTexture(imgId));
    }
    setText("HerosStatus", "Battle", heroText);

    // Враги
    std::string enemyText;
    for (int i = 0; i < enemyCount; i++)
    {
        const auto &enemy = game.getBattle()->getEnemies().at(i);
        if (!enemy.getIsAlive())
            setTexture("Enemy" + std::to_string(i + 1), "Battle", nullptr);
        enemyText += enemy.getName() + " "
                + std::to_string(enemy.getCurrentHp()) + "/"
                + std::to_string(enemy.getMaxHp()) + "\n";
    }
    SDL_Rect eRect = getRect("EnemiesStatus", "Battle");
    eRect.h = enemyCount * 22;
    setRect("EnemiesStatus", "Battle", eRect);
    setText("EnemiesStatus", "Battle", enemyText);
}

// ─────────────────────────────────────────
// Battle helpers
// ─────────────────────────────────────────

void UIManager::addEnemys()
{
    for (int i = 0; i < (int) game.getBattle()->getEnemies().size(); i++)
        setTexture("Enemy" + std::to_string(i + 1), "Battle",
                   ResourceManager::getTexture(game.getBattle()->getEnemies().at(i).getTextureName()));
}

void UIManager::addCharacters()
{
    for (int i = 0; i < MAX_HERO_COUNT; i++)
        setTexture("Hero" + std::to_string(i + 1), "Battle",
                   ResourceManager::getTexture("Image" + std::to_string(i + 1)));
}

// ─────────────────────────────────────────
// Battle UI — action flow
// ─────────────────────────────────────────

void UIManager::onActionButton(ActionType type)
{
    pendingAction = Action{};
    currentHeroIndex = game.getBattle()->getCurrentHeroIndex();
    if (currentHeroIndex > 3)
        return;
    pendingAction.actorIndex = currentHeroIndex;
    pendingAction.type = type;

    switch (type)
    {
        case ActionType::Attack:
            pendingAction.targetType = TargetType::Enemy;
            uiState = UIChooseState::ChooseTarget;
            moveSelector();
            setEnVI("ActionPanel", "Battle", false); // ← только здесь
            setEnVI("ActorSelector", "Battle", true);
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

void UIManager::initMagic()
{
    const auto &skills = player.getHeroes().at(currentHeroIndex).getAvailableSkills();
    for (int i = 0; i < (int) skills.size() && i < ENV_BUTTON_COUNT; i++)
    {
        const std::string btnId = "EnButton" + std::to_string(i + 1);
        const std::string name = game.getSkill(skills.at(i).skillId).name;
        setTexture(btnId, "Battle",
                   ResourceManager::getTextTexture(name,
                                                   ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
        setOnClick(btnId, "Battle",
                   [this, skillId = skills.at(i).skillId]() { onEnvironmentCLick(skillId, true); });
        setEnVI(btnId, "Battle", true);
    }
}

void UIManager::initItems()
{
    const auto &items = player.getAvailableItems();
    for (int i = 0; i < (int) items.size() && i < ENV_BUTTON_COUNT; i++)
    {
        const std::string btnId = "EnButton" + std::to_string(i + 1);
        const std::string name = game.getItem(items.at(i)).name;
        setTexture(btnId, "Battle",
                   ResourceManager::getTextTexture(name,
                                                   ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
        setOnClick(btnId, "Battle",
                   [this, itemId = items.at(i)]() { onEnvironmentCLick(itemId, false); });
        setEnVI(btnId, "Battle", true);
    }
}

void UIManager::onEnvironmentCLick(int id, bool isMagic)
{
    pendingAction = Action{};
    pendingAction.actorIndex = game.getBattle()->getCurrentHeroIndex();
    pendingAction.payloadId = id;

    if (isMagic)
    {
        pendingAction.type = ActionType::Magic;
        pendingAction.targetType = game.getSkill(id).targetType;
    } else
    {
        pendingAction.type = ActionType::Item;
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
    setEnVI("ActionPanel", "Battle", false); // ← скрываем только при переходе к цели
    setEnVI("ActorSelector", "Battle", true);
    setEnVI("TargetSelector", "Battle", true);
    setEnVI("EnvironmentPanel", "Battle", false);
}

void UIManager::confirmTarget()
{
    pendingAction.targetIndex = selectedTargetIndex;

    setEnVI("TargetSelector", "Battle", false);
    setEnVI("ActorSelector", "Battle", false);
    setEnVI("EnvironmentPanel", "Battle", false);

    game.getBattle()->confirmAction(pendingAction);

    uiState = UIChooseState::ChooseAction;
    setEnVI("ActionPanel", "Battle", true);
}

// ─────────────────────────────────────────
// Selector
// ─────────────────────────────────────────

int UIManager::getTargetCount(TargetType type) const
{
    switch (type)
    {
        case TargetType::Enemy: return (int) game.getBattle()->getEnemies().size();
        case TargetType::Ally: return (int) player.getHeroes().size();
        default: return 0;
    }
}

void UIManager::moveSelector()
{
    currentType = pendingAction.targetType;
    selectedTargetIndex = 0;

    SDL_Rect targetRect{};
    SDL_Log("moveSelector: currentHeroIndex=%d", currentHeroIndex);
    SDL_Rect actorRect = getRect("Hero" + std::to_string(currentHeroIndex + 1), "Battle");
    SDL_Log("moveSelector: actorRect={%d,%d,%d,%d}", actorRect.x, actorRect.y, actorRect.w, actorRect.h);

    if (currentType == TargetType::Enemy)
    {
        for (int i = 0; i < (int) game.getBattle()->getEnemies().size(); i++)
        {
            if (game.getBattle()->getEnemies().at(i).getIsAlive())
            {
                targetRect = getRect("Enemy" + std::to_string(i + 1), "Battle");
                selectedTargetIndex = i;
                break;
            }
        }
    } else if (currentType == TargetType::Ally)
    {
        for (int i = 0; i < (int) player.getHeroes().size(); i++)
        {
            if (player.getHeroes().at(i).getIsAlive())
            {
                targetRect = getRect("Hero" + std::to_string(i + 1), "Battle");
                selectedTargetIndex = i;
                break;
            }
        }
    } else if (currentType == TargetType::Self) { targetRect = actorRect; }

    // Защита — если rect нулевой, цель ещё не готова
    if (targetRect.w == 0 || targetRect.h == 0)
    {
        SDL_Log("moveSelector: target rect is zero, skipping");
        return;
    }
    if (actorRect.w == 0 || actorRect.h == 0)
    {
        SDL_Log("moveSelector: actor rect is zero, skipping");
        return;
    }

    SDL_Rect tSel = getRect("TargetSelector", "Battle");
    SDL_Rect aSel = getRect("ActorSelector", "Battle");

    tSel.x = targetRect.x + targetRect.w / 2 - tSel.w / 2;
    tSel.y = targetRect.y - targetRect.h;
    aSel.x = actorRect.x + actorRect.w / 2 - aSel.w / 2;
    aSel.y = actorRect.y - actorRect.h;

    setRect("TargetSelector", "Battle", tSel);
    setRect("ActorSelector", "Battle", aSel);
}

void UIManager::moveSelectorToNext()
{
    auto *sel = findUIObject("TargetSelector", "Battle");
    if (!sel || !sel->isEnabled) return;

    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex + 1) % max;
    snapSelectorToTarget();
}

void UIManager::moveSelectorToPrevious()
{
    auto *sel = findUIObject("TargetSelector", "Battle");
    if (!sel || !sel->isEnabled) return;

    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex - 1 + max) % max;
    snapSelectorToTarget();
}

void UIManager::snapSelectorToTarget()
{
    const int max = getTargetCount(currentType);
    SDL_Rect targetRect{};

    if (currentType == TargetType::Enemy || currentType == TargetType::AllEnemies)
    {
        for (int guard = 0; guard < max; guard++)
        {
            if (game.getBattle()->getEnemies().at(selectedTargetIndex).getIsAlive()) break;
            selectedTargetIndex = (selectedTargetIndex + 1) % max;
        }
        targetRect = getRect("Enemy" + std::to_string(selectedTargetIndex + 1), "Battle");
    } else if (currentType == TargetType::Ally || currentType == TargetType::AllAllies)
    {
        for (int guard = 0; guard < max; guard++)
        {
            if (player.getHeroes().at(selectedTargetIndex).getIsAlive()) break;
            selectedTargetIndex = (selectedTargetIndex + 1) % max;
        }
        targetRect = getRect("Hero" + std::to_string(selectedTargetIndex + 1), "Battle");
    }

    if (targetRect.w == 0 || targetRect.h == 0)
    {
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


void UIManager::updateRemoteAnimation(AnimPlayer &animPlayer, const Player &player)
{
    if (player.getDirection() == Direction::Idle)
    {
        animPlayer.stopLoop();
        return;
    }

    static const std::unordered_map<Direction, std::string> dirToAnim = {
        {Direction::Up, "MoveUpAnimation"},
        {Direction::Down, "MoveDownAnimation"},
        {Direction::Left, "MoveLeftAnimation"},
        {Direction::Right, "MoveRightAnimation"},
    };

    auto it = dirToAnim.find(player.getDirection());
    if (it != dirToAnim.end())
        animPlayer.play(it->second, false);
}

MapRenderer* UIManager::rendererFor(const MapData& map)
{
    auto it = mapRenderers.find(map.mapId);
    if (it != mapRenderers.end())
        return it->second->width() > 0 ? it->second.get() : nullptr;      // загрузка не удалась: повторять не будем

    auto r = std::make_unique<MapRenderer>();
    if (r->loadFile(visualizer.getRenderer(), map.backgroundPath) &&
        (r->width() != map.w * TILE_PX || r->height() != map.h * TILE_PX))
        SDL_Log("Map '%s': image is %dx%d, but the grid is %dx%d cells (%dx%d px)",
                map.name.c_str(), r->width(), r->height(), map.w, map.h, map.w * TILE_PX, map.h * TILE_PX);

    MapRenderer* raw = r.get();
    mapRenderers.emplace(map.mapId, std::move(r));
    return raw->width() > 0 ? raw : nullptr;
}
void UIManager::initialize()
{
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

void UIManager::initMapScene()
{
    TTF_Font *font = ResourceManager::getFont("RetroByte");

    rendererFor(game.getMap());     // грузим оверворлд сразу, чтобы не было паузы на первом кадре

    addImage("Press E", "Map",
             {screen.w / 2 - 25, screen.h / 2 - 50, 60, 20},
             ResourceManager::getTextTexture("Press E", font, {0, 0, 0, 255}));
    addImage("FPS", "Map", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
    addImage("Time", "Map", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));

    setVisible("Press E", "Map", false);
    addMusic("MapTheme", "Map");
}

void UIManager::initMenuScene()
{
    TTF_Font *font = ResourceManager::getFont("RetroByte");
    addImage("MenuBackground", "Menu", screen, ResourceManager::getTexture("MenuBackground"));
    addImage("Time", "Menu", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));
    addMusic("MenuTheme", "Menu");
    addSound("ButtonHover", "Menu");
    addPanel("ButtonPanel", "Menu");

    auto hover = [this]() { playSound("Menu", "ButtonHover"); };
    addButton("Online", "Menu", {190, screen.h / 2 + 30, 170, 50},
              ResourceManager::getTextTexture("Co-op", font, {0, 0, 0, 0}),
              [this]() { game.openOnlineMenu(); }, hover, "ButtonPanel");
    addButton("Continue", "Menu", {190, screen.h / 2 + 80, 170, 50},
              ResourceManager::getTextTexture("Continue", font, {0, 0, 0, 0}),
              [this]() { game.startGame(); }, hover, "ButtonPanel");
    addButton("New game", "Menu", {190, screen.h / 2 + 130, 170, 50},
              ResourceManager::getTextTexture("New game", font, {0, 0, 0, 0}),
              [this]() { game.startGame(); }, hover, "ButtonPanel");
    addButton("Options", "Menu", {190, screen.h / 2 + 180, 150, 50},
              ResourceManager::getTextTexture("Options", font, {0, 0, 0, 0}),
              [this]() { game.openOptions(GameState::Menu); }, hover, "ButtonPanel");
    addButton("Quit", "Menu", {190, screen.h / 2 + 230, 100, 50},
              ResourceManager::getTextTexture("Quit", font, {0, 0, 0, 0}),
              [this]() { game.endGame(); }, hover, "ButtonPanel");
    addImage("FPS", "Menu", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
}

void UIManager::initOptionsScene()
{
    TTF_Font *font = ResourceManager::getFont("RetroByte");
    const std::string text =
            "Options:\nOn Map:\nFor moving use WASD or arrows\n"
            "In Battle:\nTo select target use WASD or arrows\n"
            "To confirm the target use SPACE or ENTER(RETURN)";

    addImage("OptionsBackground", "Options", screen,
             ResourceManager::getTexture("OptionsBackground"), {0, 0, 1920, 1000});
    addImage("InstructionText", "Options",
             {screen.w / 2 - 350, screen.h / 2 - 150, 700, 240},
             ResourceManager::getTextTexture(text, font, {0, 0, 0, 0}, screen.w - 200));
    addImage("Time", "Options", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));
    addButton("BackButton", "Options",
              {screen.w / 2 - 110, screen.h - 100, 220, 90},
              ResourceManager::getTextTexture("Back to menu", font, {0, 0, 0, 0}),
              [this]() { game.closeOptions(); },
              [this]() { playSound("Menu", "ButtonHover"); });
    addMusic("OptionsTheme", "Options");
}

void UIManager::initOnlineScene()
{
    TTF_Font *font = ResourceManager::getFont("RetroByte");
    auto hover = [this]() { playSound("Menu", "ButtonHover"); };

    addMusic("OnlineTheme", "Online");
    addImage("OnlineBackground", "Online", screen,
             ResourceManager::getTexture("OptionsBackground"), {0, 0, 1920, 1000});
    addImage("Time", "Online", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));

    addLabel("IPImage", "Online", {screen.w - 360, screen.h / 2 - 25, 170, 50},
             nullptr, "Your IP: " + game.getIP(), font, {0, 0, 0, 0});
    addButton("StartServerButton", "Online",
              {screen.w - 360, screen.h / 2 + 30, 170, 50},
              ResourceManager::getTextTexture("Start server", font, {0, 0, 0, 0}),
              [this]() { game.startServer(); }, hover);

    addLabel("ConnectImage", "Online", {190, screen.h / 2 - 25, 170, 50},
             nullptr, "Enter friend ip", font, {0, 0, 0, 0});
    addLabel("NotifyLabel", "Online", {screen.w / 2 - 60, screen.h / 2 - 70, 170, 50},
             nullptr, "", font, {0, 0, 0, 0});
    addLabel("PlayerLabel", "Online", {screen.w - 170, 20, 170, 50},
             nullptr, "", font, {0, 0, 0, 0});

    addEdit("IPEdit", "Online", {190, screen.h / 2 + 30, 170, 50},
            font, {0, 0, 0, 255},
            [this]() { setFocus("IPEdit", "Online", true); }, hover);
    addButton("ConnectButton", "Online",
              {190, screen.h / 2 + 85, 170, 50},
              ResourceManager::getTextTexture("Connect", font, {0, 0, 0, 0}),
              [this]() { game.connectToServer(getText("IPEdit", "Online")); }, hover);

    addImage("FPS", "Online", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
}

void UIManager::initBattleScene()
{
    TTF_Font *font = ResourceManager::getFont("RetroByte");
    auto hover = [this]() { playSound("Battle", "ButtonHover"); };
    const SDL_Color white{255, 255, 255, 255};

    const int barY = 452;                     // верх нижней панели
    const int barH = screen.h - barY;
    const int colW = 200;                     // ширина окон со статусом

    // ── Всё, что лежит ПОД панелями, добавляется до addPanel ─────────────
    addImage("BattleBackground", "Battle",
             {0, 0, screen.w, barY}, ResourceManager::getTexture("BattleImage0"));

    addWindow("CommandWindow", "Battle", {0, barY, screen.w - 3 * colW, barH});
    addWindow("HerosWindow",   "Battle", {screen.w - 3 * colW, barY, colW, barH});
    addWindow("EnemiesWindow", "Battle", {screen.w - 2 * colW, barY, colW, barH});
    addWindow("RemoteWindow",  "Battle", {screen.w - colW,     barY, colW, barH});

    // Тексты статусов тоже под панелями: когда открыт список умений, он их закрывает.
    // Метка ставит текст в (x + paddingX, y + paddingY), а ширину и высоту берёт по тексту.
    addLabel("HerosStatus",       "Battle", {screen.w - 3 * colW, barY, 0, 0}, nullptr, "", font, white, 12, 10);
    addLabel("EnemiesStatus",     "Battle", {screen.w - 2 * colW, barY, 0, 0}, nullptr, "", font, white, 12, 10);
    addLabel("RemoteHerosStatus", "Battle", {screen.w - colW,     barY, 0, 0}, nullptr, "", font, white, 12, 10);

    addImage("FPS", "Battle", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
    addImage("Time", "Battle", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));

    addMusic("BattleTheme", "Battle");

    // ── Панели (порядок = порядок отрисовки) ─────────────────────────────
    addPanel("ActionPanel", "Battle");
    addPanel("EnvironmentPanel", "Battle");
    addPanel("EnemyPanel", "Battle");
    addPanel("HeroPanel", "Battle");

    addButton("AttackButton", "Battle", {12, barY + 10, 60, 25},
              ResourceManager::getTextTexture("Attack", font, white),
              [this]() { onActionButton(ActionType::Attack); }, hover, "ActionPanel");
    addButton("ItemsButton", "Battle", {12, barY + 36, 60, 25},
              ResourceManager::getTextTexture("Items", font, white),
              [this]() { onActionButton(ActionType::Item); }, hover, "ActionPanel");
    addButton("MagicButton", "Battle", {12, barY + 62, 60, 25},
              ResourceManager::getTextTexture("Magic", font, white),
              [this]() { onActionButton(ActionType::Magic); }, hover, "ActionPanel");
    addButton("Run", "Battle", {12, barY + 88, 60, 25},
              ResourceManager::getTextTexture("Run", font, white),
              [this]() { game.endRandomBattle(); }, hover, "ActionPanel");

    // Окно списка умений и предметов: первым в своей панели, значит, под её кнопками.
    // Скрывается и показывается вместе с панелью.
    addWindow("EnvironmentWindow", "Battle", {84, barY, 496, 66}, "EnvironmentPanel");
    for (int i = 0; i < ENV_BUTTON_COUNT; i++)
    {
        const int row = barY + 8 + (i < ENV_BUTTON_COUNT / 2 ? 0 : 28);
        const int col = 92 + (i % (ENV_BUTTON_COUNT / 2)) * 60;
        addButton("EnButton" + std::to_string(i + 1), "Battle",
                  {col, row, 60, 25}, nullptr, nullptr, nullptr, "EnvironmentPanel");
    }
    setEnVI("EnvironmentPanel", "Battle", false);

    // Селекторы выбора цели: после панелей, чтобы рисовались поверх
    addImage("TargetSelector", "Battle", {0, 0, 100, 100},
             ResourceManager::getTexture("TargetSelector"), {0, 0, 100, 100});
    addImage("ActorSelector", "Battle", {0, 0, 50, 50},
             ResourceManager::getTexture("ActorSelector"), {0, 0, 100, 100});
    setVisible("TargetSelector", "Battle", false);
    setVisible("ActorSelector", "Battle", false);

    // ── Герои и враги: размеры пока без изменений (займёмся при отрисовке боя) ──
    for (int i = 0; i < 4; i++)
    {
        addImage("Enemy" + std::to_string(i + 1), "Battle",
                 {screen.w / 2 - 100, screen.h / 2 - 230 + i * 100, 76, 94},
                 nullptr, {0, 0, 108, 144}, "EnemyPanel");
        addImage("Hero" + std::to_string(i + 1), "Battle",
                 {screen.w - 100, screen.h / 2 - 230 + i * 100, 56, 74},
                 nullptr, {0, 0, 16, 24}, "HeroPanel");
        addImage("RemoteHero" + std::to_string(i + 1), "Battle",
                 {screen.w - 100, screen.h / 2 - 230 + i * 100, 56, 74},
                 nullptr, {0, 0, 16, 24}, "HeroPanel");
    }

    for (int i = 0; i < (int) player.getHeroes().size(); i++)
    {
        addAnimation("Hero" + std::to_string(i + 1) + "Animation", "Battle",
                     ResourceManager::getAnimation("Hero" + std::to_string(i + 1) + "Animation"),
                     {
                         {screen.w - 100, screen.h / 2 - 230 + i * 100, 56, 94},
                         {screen.w - 150, screen.h / 2 - 230 + i * 100, 56, 94},
                         {screen.w - 100, screen.h / 2 - 230 + i * 100, 56, 94}
                     }, 0.2f);
        addAnimation("RemoteHero" + std::to_string(i + 1) + "Animation", "Battle",
                     ResourceManager::getAnimation("Hero" + std::to_string(i + 1) + "Animation"),
                     {
                         {screen.w - 200, screen.h / 2 - 230 + i * 100, 56, 94},
                         {screen.w - 250, screen.h / 2 - 230 + i * 100, 56, 94},
                         {screen.w - 200, screen.h / 2 - 230 + i * 100, 56, 94}
                     }, 0.2f);
    }
}