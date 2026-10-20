#include <algorithm>
#include <cmath>

#include "UIManager.h"
#include "../main/game.h"
#include "../gameLogic/gameData.h"
#include "windowSkin.h"
#include <SDL_log.h>

constexpr int ENV_BUTTON_COUNT = 8;                 // список умений и предметов: 2 колонки по 4 строки

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

    if (sceneId == "Battle" && uiState == UIChooseState::ChooseTarget && event.button.button == SDL_BUTTON_LEFT)
    {
        // Клик прямо по живой цели выбирает её
        const SDL_Point m{event.button.x, event.button.y};
        const int max = std::min(getTargetCount(currentType), 4);
        for (int i = 0; i < max; ++i)
        {
            const bool alive = currentType == TargetType::Enemy
                    ? game.getBattle()->getEnemies().at(i).getIsAlive()
                    : player.getHeroes().at(i).getIsAlive();
            const SDL_Rect r = currentType == TargetType::Enemy ? enemySlotRect[i] : heroSlotRect[i];
            if (alive && SDL_PointInRect(&m, &r))
            {
                selectedTargetIndex = i;
                confirmTarget();
                return;
            }
        }
    }

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
    if (sceneId == "Battle")
        drawBattleOverlay();                          // поверх окон: стрелки и числа урона
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
    const int heroCount = static_cast<int>(player.getHeroes().size());

    // Герои напарников
    std::string remoteHeroText;
    int slot = 0;
    for (const auto &[id, remote] : game.getRemotePlayers())
    {
        if (!game.getBattle()->isParticipant(id)) continue;
        const auto &heroes = remote.getHeroes();
        for (int i = 0; i < static_cast<int>(heroes.size()) && slot < 4; ++i, ++slot)
        {
            const auto &hero = heroes[i];
            remoteHeroText += "Remote" + hero.getName() + " " + std::to_string(hero.getCurrentHp()) + "/"
                              + std::to_string(hero.getMaxHp()) + "\n";
            const std::string img = "Image" + std::to_string(heroLook(hero.getClass()))
                                    + (hero.getCurrentHp() <= 0 ? "Dead" : "");
            setTexture("RemoteHero" + std::to_string(slot + 1), "Battle", ResourceManager::getTexture(img));
            trackHp(lastRemoteHp[slot], static_cast<int>(hero.getCurrentHp()), remoteSlotRect[slot]);
        }
    }
    const bool coop = slot > 0;                                     // slot посчитан в блоке «Герои напарников»
    setVisible("RemoteWindow", "Battle", coop);
    setRect("EnemiesWindow", "Battle",
            {scr.w - 400, 452, coop ? 200 : 400, scr.h - 452});
    setText("RemoteHerosStatus", "Battle", remoteHeroText);

    // Свои герои. Тот, кто сейчас выбирает действие, выходит на шаг вперёд
    const bool choosing = game.getBattle()->isPlayerChoosing();
    const int active = choosing ? game.getBattle()->getCurrentHeroIndex() : -1;
    std::string heroText;
    for (int i = 0; i < heroCount && i < 4; i++)
    {
        const auto &hero = player.getHeroes().at(i);
        heroText += hero.getName() + " " + std::to_string(hero.getCurrentHp()) + "/"
                    + std::to_string(hero.getMaxHp()) + "\n";
        const std::string id = "Hero" + std::to_string(i + 1);
        const std::string img = "Image" + std::to_string(heroLook(hero.getClass()))
                                + (hero.getCurrentHp() <= 0 ? "Dead" : "");
        setTexture(id, "Battle", ResourceManager::getTexture(img));
        SDL_Rect r = heroSlotRect[i];
        if (i == active) r.x -= 8 * PIXEL_SCALE;
        setRect(id, "Battle", r);
        trackHp(lastHeroHp[i], static_cast<int>(hero.getCurrentHp()), heroSlotRect[i]);
    }
    setText("HerosStatus", "Battle", heroText);

    // Кто ходит и сколько у него маны: в свободной части командного окна
    std::string actorText;
    if (active >= 0 && active < heroCount)
    {
        const auto &hero = player.getHeroes().at(active);
        actorText = hero.getName() + "\nMP " + std::to_string(hero.getCurrentMp()) + "/"
                    + std::to_string(hero.getMaxMp());
    }
    setText("ActorInfo", "Battle", actorText);

    // Пустые команды серые: открыть пустой список нельзя
    TTF_Font *font = ResourceManager::getFont("RetroByte");
    const SDL_Color white{255, 255, 255, 255}, gray{120, 128, 168, 255};
    setTexture("ItemsButton", "Battle", ResourceManager::getTextTexture("Items", font, hasItems() ? white : gray));
    setTexture("MagicButton", "Battle",
               ResourceManager::getTextTexture("Magic", font, choosing && hasUsableSkill() ? white : gray));

    // Враги
    const auto& enemies = game.getBattle()->getEnemies();
    for (int i = 0; i < static_cast<int>(enemies.size()) && i < 4; ++i)
    {
        trackHp(lastEnemyHp[i], static_cast<int>(enemies[i].getCurrentHp()), enemySlotRect[i]);
        if (!enemies[i].getIsAlive())
            setTexture("Enemy" + std::to_string(i + 1), "Battle", nullptr);
    }

    std::vector<std::pair<std::string, int>> groups;                // имя и сколько таких живых
    for (const auto& enemy : enemies)
    {
        if (!enemy.getIsAlive()) continue;
        auto it = std::find_if(groups.begin(), groups.end(),
                               [&](const auto& g) { return g.first == enemy.getName(); });
        if (it == groups.end()) groups.push_back({enemy.getName(), 1});
        else ++it->second;
    }
    std::string enemyText;
    for (const auto& [name, count] : groups)
        enemyText += name + (count > 1 ? " x" + std::to_string(count) : "") + "\n";
    setText("EnemiesStatus", "Battle", enemyText);
}

// ─────────────────────────────────────────
// Battle helpers
// ─────────────────────────────────────────
namespace {
    constexpr double kPi = 3.14159265358979323846;

    SDL_Point textureSize(SDL_Texture* t)
    {
        int w = 0, h = 0;
        if (t) SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
        return {w, h};
    }

    // Рамки анимации атаки: герой делает шаг влево и возвращается; «ноги» остаются на нижнем крае слота
    std::vector<SDL_Rect> attackRects(const std::vector<SDL_Texture*>& frames, const SDL_Rect& slot, int scale)
    {
        std::vector<SDL_Rect> rects;
        const int n = static_cast<int>(frames.size());
        for (int f = 0; f < n; ++f)
        {
            const SDL_Point sz = textureSize(frames[f]);
            const int w = sz.x * scale, h = sz.y * scale;
            const int dx = -static_cast<int>(std::lround(14.0 * scale * std::sin(kPi * f / std::max(1, n - 1))));
            rects.push_back({slot.x + slot.w / 2 - w / 2 + dx, slot.y + slot.h - h, w, h});
        }
        return rects;
    }
}


void UIManager::layoutBattle()
{
    const SDL_Rect scr = game.getScreenRect();
    const int S = PIXEL_SCALE;                 // тот же масштаб, что на карте
    const int fieldH = 452;                    // поле боя над нижней панелью
    const int margin = 16;

    // ── Враги: одна или две колонки слева ──────────────────────────────
    const auto& enemies = game.getBattle()->getEnemies();
    const int n = std::min<int>(static_cast<int>(enemies.size()), 4);

    std::array<SDL_Point, 4> nat{};            // родные размеры спрайтов
    for (int i = 0; i < n; ++i)
        if (const EnemyDef* def = GameData::instance().enemy(enemies[i].getEnemyId()))
            nat[i] = {def->srcRect.w, def->srcRect.h};

    const int columns = n <= 2 ? 1 : 2;
    std::array<std::vector<int>, 2> col;
    for (int i = 0; i < n; ++i) col[columns == 1 ? 0 : i % 2].push_back(i);

    const SDL_Rect area{margin, margin, 560 - margin, fieldH - 2 * margin};

    auto fits = [&](int scale) {
        for (int c = 0; c < columns; ++c)
        {
            int sumH = 0, maxW = 0;
            for (int i : col[c]) { sumH += nat[i].y * scale; maxW = std::max(maxW, nat[i].x * scale); }
            if (sumH > area.h || maxW > area.w / columns) return false;
        }
        return true;
    };
    int es = S;
    while (es > 1 && !fits(es)) --es;          // слишком крупная группа: уменьшаем целым шагом, а не дробью

    for (int c = 0; c < columns; ++c)
    {
        const int cx = columns == 1 ? area.x + area.w / 2 : area.x + area.w * (c == 0 ? 27 : 73) / 100;
        const int m = static_cast<int>(col[c].size());
        for (int k = 0; k < m; ++k)
        {
            const int i = col[c][k];
            const int w = nat[i].x * es, h = nat[i].y * es;
            const int slotH = area.h / std::max(1, m);
            const int bottom = area.y + slotH * (k + 1) - 6;               // «ноги» на нижнем крае слота
            enemySlotRect[i] = {cx - w / 2, bottom - h, w, h};
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        const std::string id = "Enemy" + std::to_string(i + 1);
        if (i < n && nat[i].x > 0) setRect(id, "Battle", enemySlotRect[i]);
        else                       setTexture(id, "Battle", nullptr);      // пустой слот: убираем остатки прошлого боя
    }

    // ── Герои: две колонки справа (свои и напарника) ──────────────────
    const int hw = 16 * S, hh = 24 * S;
    const int slotH = (fieldH - 2 * margin) / 4;
    const int localX = scr.w - margin - hw - 8;
    const int remoteX = localX - hw - 36;
    for (int i = 0; i < 4; ++i)
    {
        const int y = margin + slotH * (i + 1) - 10 - hh;
        heroSlotRect[i]   = {localX,  y, hw, hh};
        remoteSlotRect[i] = {remoteX, y, hw, hh};
        setRect("Hero" + std::to_string(i + 1), "Battle", heroSlotRect[i]);
        setRect("RemoteHero" + std::to_string(i + 1), "Battle", remoteSlotRect[i]);
    }
}

void UIManager::addEnemys()
{
    const auto& enemies = game.getBattle()->getEnemies();
    for (int i = 0; i < 4; i++)
        setTexture("Enemy" + std::to_string(i + 1), "Battle",
                   i < static_cast<int>(enemies.size())
                       ? ResourceManager::getTexture(enemies[i].getTextureName()) : nullptr);
    layoutBattle();                            // сначала враги, потому что addCharacters использует позиции героев
    resetHpTracking();                         // новый бой: первый кадр только запоминает HP
    closeList();
    uiState = UIChooseState::ChooseAction;
}

void UIManager::addCharacters()
{
    const int S = PIXEL_SCALE;
    const auto& mine = player.getHeroes();
    for (int i = 0; i < 4; ++i)
    {
        const std::string id = "Hero" + std::to_string(i + 1);
        if (i >= static_cast<int>(mine.size())) { setTexture(id, "Battle", nullptr); continue; }

        const int look = heroLook(mine[i].getClass());
        setTexture(id, "Battle", ResourceManager::getTexture("Image" + std::to_string(look)));
        const auto& frames = ResourceManager::getAnimation("Hero" + std::to_string(look) + "Animation");
        addAnimation(id + "Animation", "Battle", frames, attackRects(frames, heroSlotRect[i], S), 0.2f);
    }

    int slot = 0;                                                         // герои напарников
    for (const auto& [id, remote] : game.getRemotePlayers())
    {
        if (!game.getBattle()->isParticipant(id)) continue;
        const auto& heroes = remote.getHeroes();
        for (int i = 0; i < static_cast<int>(heroes.size()) && slot < 4; ++i, ++slot)
        {
            const int look = heroLook(heroes[i].getClass());
            const std::string rid = "RemoteHero" + std::to_string(slot + 1);
            setTexture(rid, "Battle", ResourceManager::getTexture("Image" + std::to_string(look)));
            const auto& frames = ResourceManager::getAnimation("Hero" + std::to_string(look) + "Animation");
            addAnimation(rid + "Animation", "Battle", frames, attackRects(frames, remoteSlotRect[slot], S), 0.2f);
        }
    }
    for (; slot < 4; ++slot)
        setTexture("RemoteHero" + std::to_string(slot + 1), "Battle", nullptr);
}

// ─────────────────────────────────────────
// Battle UI — action flow
// ─────────────────────────────────────────

namespace {
    const SDL_Color kWhite{255, 255, 255, 255};
    const SDL_Color kGray {120, 128, 168, 255};             // недоступная строка: синевато-серая, под цвет окна
}

bool UIManager::hasUsableSkill() const
{
    const int i = game.getBattle()->getCurrentHeroIndex();
    if (i < 0 || i >= static_cast<int>(player.getHeroes().size())) return false;
    const Hero& hero = player.getHeroes()[i];
    for (const auto& s : hero.getAvailableSkills())
        if (hero.canCast(game.getSkill(s.skillId))) return true;
    return false;
}

bool UIManager::hasItems() const
{
    for (const auto& [id, count] : player.getItemCounts())
        if (count > 0) return true;
    return false;
}

void UIManager::onActionButton(ActionType type)
{
    closeList();                                           // кнопку другой команды нажали при открытом списке
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
            setEnVI("ActionPanel", "Battle", false);
            break;

        case ActionType::Magic:
            if (!hasUsableSkill()) return;                 // пустой список не открываем, кнопка и так серая
            uiState = UIChooseState::ChooseSkill;
            initMagic();
            setEnVI("EnvironmentPanel", "Battle", true);
            break;

        case ActionType::Item:
            if (!hasItems()) return;
            uiState = UIChooseState::ChooseItem;
            initItems();
            setEnVI("EnvironmentPanel", "Battle", true);
            break;

        case ActionType::Skip:
            game.getBattle()->confirmAction(pendingAction);
            uiState = UIChooseState::ChooseAction;
            setEnVI("ActionPanel", "Battle", true);
            break;
    }
}

void UIManager::setListEntry(int i, const std::string& name, const std::string& info,
                             bool usable, std::function<void()> onClick)
{
    TTF_Font* font = ResourceManager::getFont("RetroByte");
    const SDL_Color c = usable ? kWhite : kGray;
    const std::string n = std::to_string(i + 1);

    const SDL_Rect cell = listRect[i];
    SDL_Texture* nameTex = ResourceManager::getTextTexture(name, font, c);
    SDL_Texture* infoTex = info.empty() ? nullptr : ResourceManager::getTextTexture(info, font, c);
    int nw = 0, nh = 0, iw = 0, ih = 0;
    if (nameTex) SDL_QueryTexture(nameTex, nullptr, nullptr, &nw, &nh);
    if (infoTex) SDL_QueryTexture(infoTex, nullptr, nullptr, &iw, &ih);

    // Текст в родном размере: раньше он растягивался в 60x25, и длинные названия сплющивались
    setRect("EnButton" + n, "Battle", {cell.x, cell.y + (cell.h - nh) / 2, nw, nh});
    setRect("EnInfo" + n, "Battle", {cell.x + cell.w - iw, cell.y + (cell.h - ih) / 2, iw, ih});
    setTexture("EnButton" + n, "Battle", nameTex);
    setTexture("EnInfo" + n, "Battle", infoTex);
    setOnClick("EnButton" + n, "Battle", usable ? onClick : nullptr);
    setOnClick("EnInfo" + n, "Battle", usable ? std::move(onClick) : nullptr);
    setEnVI("EnButton" + n, "Battle", true);
    setEnVI("EnInfo" + n, "Battle", true);
}

void UIManager::closeList()
{
    setEnVI("EnvironmentPanel", "Battle", false);
    for (int i = 0; i < ENV_BUTTON_COUNT; ++i)                // строки прошлого списка не должны пережить смену списка
    {
        const std::string n = std::to_string(i + 1);
        setTexture("EnButton" + n, "Battle", nullptr);
        setTexture("EnInfo" + n, "Battle", nullptr);
        setOnClick("EnButton" + n, "Battle", nullptr);
        setOnClick("EnInfo" + n, "Battle", nullptr);
    }
}

void UIManager::initMagic()
{
    closeList();
    const Hero& hero = player.getHeroes().at(currentHeroIndex);
    const auto skills = hero.getAvailableSkills();
    for (int i = 0; i < static_cast<int>(skills.size()) && i < ENV_BUTTON_COUNT; i++)
    {
        const Skill& skill = game.getSkill(skills[i].skillId);
        std::string info = std::to_string(skill.manaCost) + " MP";
        if (skills[i].cooldownLeft > 0) info = "CD " + std::to_string(skills[i].cooldownLeft);
        setListEntry(i, skill.name, info, hero.canCast(skill),
                     [this, skillId = skill.id]() { onEnvironmentCLick(skillId, true); });
    }
}

void UIManager::initItems()
{
    closeList();
    int i = 0;
    for (const auto& [itemId, count] : player.getItemCounts())
    {
        if (count == 0) continue;
        if (i >= ENV_BUTTON_COUNT) break;
        setListEntry(i++, game.getItem(itemId).name, "x" + std::to_string(count), true,
                     [this, itemId]() { onEnvironmentCLick(itemId, false); });
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
        closeList();
        uiState = UIChooseState::ChooseAction;
        setEnVI("ActionPanel", "Battle", true);
        return;
    }

    uiState = UIChooseState::ChooseTarget;
    moveSelector();
    setEnVI("ActionPanel", "Battle", false);
    setEnVI("EnvironmentPanel", "Battle", false);           // строки не трогаем: Esc вернёт к этому же списку
}

void UIManager::confirmTarget()
{
    pendingAction.targetIndex = selectedTargetIndex;
    closeList();

    game.getBattle()->confirmAction(pendingAction);

    uiState = UIChooseState::ChooseAction;
    setEnVI("ActionPanel", "Battle", true);
}

void UIManager::cancelChoice()
{
    switch (uiState)
    {
        case UIChooseState::ChooseTarget:
            if (pendingAction.type == ActionType::Magic || pendingAction.type == ActionType::Item)
            {
                uiState = pendingAction.type == ActionType::Magic ? UIChooseState::ChooseSkill
                                                                  : UIChooseState::ChooseItem;
                setEnVI("ActionPanel", "Battle", true);
                setEnVI("EnvironmentPanel", "Battle", true);
                return;
            }
            break;
        case UIChooseState::ChooseSkill:
        case UIChooseState::ChooseItem:
            break;
        default:
            return;
    }
    closeList();
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

SDL_Rect UIManager::selectedTargetRect() const
{
    const int i = selectedTargetIndex;
    if (i < 0 || i > 3) return {};
    if (currentType == TargetType::Enemy) return enemySlotRect[i];
    if (currentType != TargetType::Ally) return {};
    SDL_Rect r = heroSlotRect[i];
    if (game.getBattle()->isPlayerChoosing() && i == game.getBattle()->getCurrentHeroIndex())
        r.x -= 8 * PIXEL_SCALE;                              // ходящий герой стоит на шаг впереди
    return r;
}

void UIManager::moveSelector()
{
    currentType = pendingAction.targetType;
    selectedTargetIndex = 0;
    snapSelectorToTarget();                                  // первая живая цель
}

void UIManager::moveSelectorToNext()
{
    if (uiState != UIChooseState::ChooseTarget) return;
    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex + 1) % max;
    snapSelectorToTarget();
}

void UIManager::moveSelectorToPrevious()
{
    if (uiState != UIChooseState::ChooseTarget) return;
    const int max = getTargetCount(currentType);
    if (max == 0) return;

    // Назад тоже пропускаем мёртвых, иначе snap перепрыгнет вперёд через ту же цель
    for (int guard = 0; guard < max; guard++)
    {
        selectedTargetIndex = (selectedTargetIndex - 1 + max) % max;
        const bool alive = currentType == TargetType::Enemy
                ? game.getBattle()->getEnemies().at(selectedTargetIndex).getIsAlive()
                : player.getHeroes().at(selectedTargetIndex).getIsAlive();
        if (alive) break;
    }
}

void UIManager::snapSelectorToTarget()
{
    const int max = getTargetCount(currentType);
    for (int guard = 0; guard < max; guard++)
    {
        const bool alive = currentType == TargetType::Enemy
                ? game.getBattle()->getEnemies().at(selectedTargetIndex).getIsAlive()
                : player.getHeroes().at(selectedTargetIndex).getIsAlive();
        if (alive) break;
        selectedTargetIndex = (selectedTargetIndex + 1) % max;
    }
}

// ─────────────────────────────────────────
// Battle overlay: указатели, имя цели, урон
// ─────────────────────────────────────────

void UIManager::drawPointer(int x, int y, int scale) const
{
    // Стрелка 6x9 «пикселей», острие справа по центру. '#' — чёрный контур, 'o' — белая заливка
    static const char* kArrow[] = {
        "##....",
        "#o#...",
        "#oo#..",
        "#ooo#.",
        "#oooo#",
        "#ooo#.",
        "#oo#..",
        "#o#...",
        "##....",
    };
    const int top = y - 9 * scale / 2;
    const int left = x - 6 * scale;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 6; ++c)
        {
            const char ch = kArrow[r][c];
            if (ch == '.') continue;
            visualizer.fillRect({left + c * scale, top + r * scale, scale, scale},
                                ch == '#' ? SDL_Color{0, 0, 0, 255} : SDL_Color{255, 255, 255, 255});
        }
}

void UIManager::drawOutlinedText(const std::string& text, int x, int y, SDL_Color color, Uint8 alpha) const
{
    // Белая текстура красится через color mod: одна текстура на число, любой цвет и чёрная обводка
    SDL_Texture* t = ResourceManager::getTextTexture(text, ResourceManager::getFont("RetroByte"), kWhite);
    if (!t) return;
    int w = 0, h = 0;
    SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
    x -= w / 2;                                              // x — центр надписи

    SDL_SetTextureAlphaMod(t, alpha);
    SDL_SetTextureColorMod(t, 0, 0, 0);
    for (const auto& [dx, dy] : {std::pair{-2, 0}, std::pair{2, 0}, std::pair{0, -2}, std::pair{0, 2}})
        visualizer.drawTexture(t, x + dx, y + dy, w, h);
    SDL_SetTextureColorMod(t, color.r, color.g, color.b);
    visualizer.drawTexture(t, x, y, w, h);
    SDL_SetTextureColorMod(t, 255, 255, 255);                // текстура общая: возвращаем как было
    SDL_SetTextureAlphaMod(t, 255);
}

void UIManager::resetHpTracking()
{
    lastEnemyHp.fill(-1);
    lastHeroHp.fill(-1);
    lastRemoteHp.fill(-1);
    popups.clear();
}

void UIManager::trackHp(int& last, int hp, const SDL_Rect& slot)
{
    // Числа берём из разницы HP между кадрами: так они работают и у клиента, которому HP приходят снапшотом
    if (last >= 0 && hp != last && slot.w > 0)
    {
        const int delta = hp - last;
        const SDL_Color c = delta < 0 ? SDL_Color{255, 255, 255, 255} : SDL_Color{96, 255, 120, 255};
        const int x = slot.x + slot.w / 2;
        int y = slot.y + slot.h / 3;
        for (const auto& p : popups)                         // два удара подряд по одной цели: второе число выше
            if (p.x == x && SDL_GetTicks() - p.start < 400) y = std::min(y, p.y - 22);
        popups.push_back({x, y, std::to_string(std::abs(delta)), c, SDL_GetTicks()});
    }
    last = hp;
}

void UIManager::drawBattleOverlay()
{
    const Uint32 now = SDL_GetTicks();
    const int bob = (now / 220) % 2 * 3;                     // стрелка «дышит» на 3 px

    // Указатель на цель и окно с её именем
    if (uiState == UIChooseState::ChooseTarget)
    {
        const SDL_Rect t = selectedTargetRect();
        if (t.w > 0)
        {
            drawPointer(t.x - 4 - bob, t.y + t.h / 2, PIXEL_SCALE);

            const std::string name = currentType == TargetType::Enemy
                    ? game.getBattle()->getEnemies().at(selectedTargetIndex).getName()
                    : player.getHeroes().at(selectedTargetIndex).getName();
            SDL_Texture* tex = ResourceManager::getTextTexture(name, ResourceManager::getFont("RetroByte"), kWhite);
            int w = 0, h = 0;
            if (tex) SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
            const SDL_Rect win{screen.w / 2 - (w + 32) / 2, 10, w + 32, h + 16};
            visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), win, kWindowSkinSlice, 1);
            if (tex) visualizer.drawTexture(tex, win.x + 16, win.y + 8, w, h);
        }
    }

    // Стрелка у строки меню под мышью
    int mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    auto pointAt = [&](const std::string& id) {
        const UIObject* obj = findUIObject(id, "Battle");
        if (!obj || !obj->isVisible || !obj->texture) return;
        const SDL_Rect r = obj->rect;
        if (my >= r.y - 2 && my <= r.y + r.h + 2 && mx >= r.x - 16 && mx <= r.x + std::max(r.w, 60))
            drawPointer(r.x - 4, r.y + r.h / 2, 2);
    };
    if (uiState == UIChooseState::ChooseSkill || uiState == UIChooseState::ChooseItem)
        for (int i = 0; i < ENV_BUTTON_COUNT; ++i) pointAt("EnButton" + std::to_string(i + 1));
    else if (uiState == UIChooseState::ChooseAction && game.getBattle()->isPlayerChoosing())
        for (const char* id : {"AttackButton", "ItemsButton", "MagicButton", "Run"}) pointAt(id);

    // Всплывающие числа: подскок вверх, пауза, затухание
    constexpr Uint32 kRise = 250, kLife = 1100, kFade = 300;
    std::erase_if(popups, [&](const Popup& p) { return now - p.start >= kLife; });
    for (const auto& p : popups)
    {
        const Uint32 age = now - p.start;
        const float rise = std::min(1.0f, static_cast<float>(age) / kRise);
        const int y = p.y - static_cast<int>(18 * std::sin(rise * kPi / 2));
        const Uint8 alpha = age < kLife - kFade ? 255
                : static_cast<Uint8>(255 * (kLife - age) / kFade);
        drawOutlinedText(p.text, p.x, y, p.color, alpha);
    }
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

namespace {
constexpr SpecializationNames kPickable[4] = {
    SpecializationNames::Warrior, SpecializationNames::Thief,
    SpecializationNames::Archer,  SpecializationNames::Magician };
}

void UIManager::initCreateScene()
{
    TTF_Font* font = ResourceManager::getFont("RetroByte");
    const SDL_Color white{255, 255, 255, 255};
    const SDL_Color black{0, 0, 0, 255};
    auto hover = [this]() { playSound("Menu", "ButtonHover"); };

    // drawScene берёт музыку сцены по имени «<сцена>Theme»: подставляем музыку меню
    if (Scene* s = findScene("CreatePlayer"))
        s->musicPlayer.music["CreatePlayerTheme"] = ResourceManager::getMusic("MenuTheme");

    addImage("CreateBackground", "CreatePlayer", screen,
             ResourceManager::getTexture("OptionsBackground"), {0, 0, 1920, 1000});
    addLabel("CreateTitle", "CreatePlayer", {screen.w / 2 - 140, 24, 0, 0}, nullptr,
             "Choose your party", font, black, 8, 4);

    const int slotW = 200, gap = 16, top = 80, slotH = 400;
    const int startX = (screen.w - (4 * slotW + 3 * gap)) / 2;

    for (int i = 0; i < 4; ++i)
    {
        const std::string n = std::to_string(i + 1);
        const int x = startX + i * (slotW + gap);

        addWindow("Slot" + n, "CreatePlayer", {x, top, slotW, slotH});
        addImage("Portrait" + n, "CreatePlayer", {x + 60, top + 24, 80, 120}, nullptr, {0, 0, 16, 24});
        addButton("Prev" + n, "CreatePlayer", {x + 10, top + 160, 30, 30},
                  ResourceManager::getTextTexture("<", font, white), [this, i]() { cycleClass(i, -1); }, hover);
        addButton("Next" + n, "CreatePlayer", {x + slotW - 40, top + 160, 30, 30},
                  ResourceManager::getTextTexture(">", font, white), [this, i]() { cycleClass(i, +1); }, hover);
        addLabel("Class" + n, "CreatePlayer", {x + 48, top + 162, 0, 0}, nullptr, "", font, white, 4, 4);
        addLabel("Stats" + n, "CreatePlayer", {x + 14, top + 205, 0, 0}, nullptr, "", font, white, 4, 4);
        addEdit("Name" + n, "CreatePlayer", {x + 14, top + slotH - 56, slotW - 28, 38}, font, white, nullptr, hover);
        styleEdit("Name" + n, "CreatePlayer", {10, 20, 90, 255}, {232, 232, 240, 255}, 12);
    }

    addButton("StartButton", "CreatePlayer", {screen.w / 2 - 90, screen.h - 90, 180, 50},
              ResourceManager::getTextTexture("Start", font, black), [this]() { confirmParty(); }, hover);
    addButton("BackToMenu", "CreatePlayer", {20, screen.h - 90, 140, 50},
              ResourceManager::getTextTexture("Back", font, black), [this]() { game.closePartyCreation(); }, hover);

    resetPartyCreation();
}

void UIManager::resetPartyCreation()
{
    pickedClass = {0, 1, 2, 3};
    autoName.fill("");
    for (int i = 0; i < 4; ++i)
    {
        const std::string n = std::to_string(i + 1);
        setText("Name" + n, "CreatePlayer", "");
        setFocus("Name" + n, "CreatePlayer", false);
        refreshPartySlot(i);
    }
}

void UIManager::refreshPartySlot(int slot)
{
    const std::string n = std::to_string(slot + 1);
    const SpecializationNames cls = kPickable[pickedClass[slot]];
    const HeroClassDef* def = GameData::instance().heroClass(cls);
    if (!def) return;

    setTexture("Portrait" + n, "CreatePlayer", ResourceManager::getTexture("Image" + std::to_string(heroLook(cls))));
    setText("Class" + n, "CreatePlayer", def->name);
    setText("Stats" + n, "CreatePlayer",
            "HP  " + std::to_string(def->hp) + "\nMP  " + std::to_string(def->mp) +
            "\nATK " + std::to_string(def->attack) + "\nDEF " + std::to_string(def->defence) +
            "\nAGI " + std::to_string(def->agility));

    const std::string current = getText("Name" + n, "CreatePlayer");
    if (current.empty() || current == autoName[slot])        // имя по умолчанию следует за классом, своё не трогаем
        setText("Name" + n, "CreatePlayer", def->name);
    autoName[slot] = def->name;
}

void UIManager::cycleClass(int slot, int dir)
{
    pickedClass[slot] = (pickedClass[slot] + dir + 4) % 4;
    refreshPartySlot(slot);
}

bool UIManager::handleTextEvent(const SDL_Event& event, const std::string& sceneId)
{
    if (event.type != SDL_TEXTINPUT && event.type != SDL_KEYDOWN) return false;

    Scene* scene = findScene(sceneId);
    Edit* edit = scene ? scene->focusedEdit() : nullptr;
    if (!edit) return false;

    if (event.type == SDL_TEXTINPUT)
        return edit->handleTextInput(event.text.text);
    return edit->handleKey(event.key.keysym.scancode);
}

void UIManager::styleEdit(const std::string& id, const std::string& sceneId,
                          SDL_Color background, SDL_Color border, int maxLength)
{
    if (auto* edit = dynamic_cast<Edit*>(findUIObject(id, sceneId)))
    {
        edit->background = background;
        edit->border = border;
        edit->maxLength = maxLength;
    }
}

void UIManager::focusNameField(int slot)
{
    for (int i = 0; i < 4; ++i)
        setFocus("Name" + std::to_string(i + 1), "CreatePlayer", i == slot);
}

void UIManager::confirmParty()
{
    std::vector<SpecializationNames> classes;
    std::vector<std::string> names;
    for (int i = 0; i < 4; ++i)
    {
        classes.push_back(kPickable[pickedClass[i]]);
        names.push_back(sanitizeHeroName(getText("Name" + std::to_string(i + 1), "CreatePlayer")));
    }
    game.createParty(classes, names);
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
    initCreateScene();
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
              [this]() { game.openPartyCreation(); }, hover, "ButtonPanel");
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

void UIManager::showBattleResults(const BattleReport& r)
{
    std::string text = "Victory!\n\n";
    text += "Obtained " + std::to_string(r.gold) + " Gil\n";
    text += "Each hero earned " + std::to_string(r.expEach) + " EXP\n";
    for (const auto& up : r.levelUps)
        text += "\n" + up.name + " reached level " + std::to_string(up.newLevel) + "!";

    setText("ResultText", "Battle", text);
    setEnVI("ActionPanel", "Battle", false);
    closeList();
    uiState = UIChooseState::ChooseAction;
    setEnVI("ResultPanel", "Battle", true);
}

void UIManager::hideBattleResults()
{
    setEnVI("ResultPanel", "Battle", false);
    setEnVI("ActionPanel", "Battle", true);                      // для следующего боя
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
    addLabel("ActorInfo",         "Battle", {96,                  barY, 0, 0}, nullptr, "", font, white, 12, 10);

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

    // Окно списка умений и предметов закрывает всю нижнюю панель правее команд.
    // Две колонки по четыре строки: слева название, справа стоимость или количество.
    const SDL_Rect listWin{84, barY, screen.w - 84, barH};
    addWindow("EnvironmentWindow", "Battle", listWin, "EnvironmentPanel");
    const int rows = ENV_BUTTON_COUNT / 2;
    const int cellW = (listWin.w - 3 * 28) / 2;
    const int cellH = (barH - 20) / rows;
    for (int i = 0; i < ENV_BUTTON_COUNT; i++)
    {
        const int col = i / rows, row = i % rows;              // сначала вниз по колонке, потом вправо
        listRect[i] = {listWin.x + 28 + col * (cellW + 28), barY + 10 + row * cellH, cellW, cellH};
        addButton("EnButton" + std::to_string(i + 1), "Battle", listRect[i], nullptr, nullptr, nullptr, "EnvironmentPanel");
        addButton("EnInfo" + std::to_string(i + 1), "Battle", listRect[i], nullptr, nullptr, nullptr, "EnvironmentPanel");
    }
    setEnVI("EnvironmentPanel", "Battle", false);

    // Слоты врагов и героев: положение и картинки задаёт layoutBattle() при старте боя
    for (int i = 0; i < 4; i++)
    {
        const std::string n = std::to_string(i + 1);
        addImage("Enemy" + n,      "Battle", {0, 0, 0, 0}, nullptr, {0, 0, 0, 0},   "EnemyPanel");
        addImage("Hero" + n,       "Battle", {0, 0, 0, 0}, nullptr, {0, 0, 16, 24}, "HeroPanel");
        addImage("RemoteHero" + n, "Battle", {0, 0, 0, 0}, nullptr, {0, 0, 16, 24}, "HeroPanel");
    }

    addPanel("ResultPanel", "Battle");
    addWindow("ResultWindow", "Battle", {screen.w / 2 - 230, 110, 460, 230}, "ResultPanel");
    addLabel("ResultText", "Battle", {screen.w / 2 - 210, 125, 0, 0}, nullptr, "", font,
             {255, 255, 255, 255}, 8, 4, "ResultPanel");
    addLabel("ResultHint", "Battle", {screen.w / 2 - 210, 300, 0, 0}, nullptr, "Press Enter", font,
             {255, 255, 255, 255}, 8, 4, "ResultPanel");
    setEnVI("ResultPanel", "Battle", false);
}