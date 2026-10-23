#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

#include "UIManager.h"
#include "../main/game.h"
#include "../gameLogic/gameData.h"
#include "windowSkin.h"
#include "widgets/Window.h"
#include "../main/lang.h"
#include "../main/utf8.h"
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
    const bool effectsDone = sceneId != "Battle" || fx.empty();      // ход ждёт, пока доиграют удары и заклинания
    return p.currentAnimationId.empty() && p.animationQueue.empty() && effectsDone;
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
        // Клик прямо по живой цели выбирает её; «на всех» — клик по любой из них, «на себя» — по ходящему герою
        const SDL_Point m{event.button.x, event.button.y};
        const bool enemiesSide = currentType == TargetType::Enemy || currentType == TargetType::AllEnemies;
        const int count = enemiesSide ? static_cast<int>(game.getBattle()->getEnemies().size())
                                      : static_cast<int>(player.getHeroes().size());
        for (int i = 0; i < std::min(count, enemiesSide ? MAX_ENEMY_COUNT : 4); ++i)
        {
            if (currentType == TargetType::Self && i != currentHeroIndex) continue;
            const bool alive = enemiesSide ? game.getBattle()->getEnemies().at(i).getIsAlive()
                                           : player.getHeroes().at(i).getIsAlive();
            const SDL_Rect r = enemiesSide ? enemySlotRect[i] : heroSlotRect[i];
            if (alive && SDL_PointInRect(&m, &r))
            {
                if (currentType == TargetType::Enemy || currentType == TargetType::Ally) selectedTargetIndex = i;
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

void UIManager::drawProp(const Prop& p, int camX, int camY)
{
    SDL_Texture* tex = ResourceManager::getTexture(p.sprite);
    if (!tex) return;
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
    const int footX = px(p.px * PIXEL_SCALE) + tw * PIXEL_SCALE / 2 - camX;   // ровно там, где объект стоит в Tiled
    const int footY = px(p.py * PIXEL_SCALE) - camY;
    const SDL_Rect scr = game.getScreenRect();
    if (footX + tw * PIXEL_SCALE < 0 || footX - tw * PIXEL_SCALE > scr.w || footY < 0 || footY - th * PIXEL_SCALE > scr.h)
        return;                                                        // за экраном
    visualizer.drawSprite(tex, footX, footY, PIXEL_SCALE);
}

void UIManager::showMapNotice(const std::string& text)
{
    mapNotice = text;
    mapNoticeTicks = SDL_GetTicks();
}

void UIManager::drawMinimap()
{
    const MapData& map = game.getMap();
    MapRenderer* r = rendererFor(map);
    if (!r || !r->minimap() || map.w <= 0 || map.h <= 0) return;

    // Окно — карта целиком, вписанная в 180x180 с сохранением пропорций; в правом верхнем углу под часами.
    // При приближении окно то же, а в нём видна часть карты вокруг игрока
    constexpr int kBox = 180, kPad = 6, kMargin = 16, kTop = 46;
    const int zoom = game.getMinimapZoom();
    const float fit = std::min(static_cast<float>(kBox) / map.w, static_cast<float>(kBox) / map.h);
    const int w = std::max(1, px(map.w * fit)), h = std::max(1, px(map.h * fit));
    const SDL_Rect frame{screen.w - kMargin - w - 2 * kPad, kTop, w + 2 * kPad, h + 2 * kPad};
    const SDL_Rect inner{frame.x + kPad, frame.y + kPad, w, h};
    visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), frame, kWindowSkinSlice, 1);

    const Vec2f me = visualCell(player.getMove());
    const int srcW = std::max(1, map.w / zoom), srcH = std::max(1, map.h / zoom);   // клеток в окне
    const int srcX = std::clamp(px(me.x + 0.5f) - srcW / 2, 0, map.w - srcW);     // игрок в центре, но не за краем
    const int srcY = std::clamp(px(me.y + 0.5f) - srcH / 2, 0, map.h - srcH);
    visualizer.drawTexture(r->minimap(), inner.x, inner.y, inner.w, inner.h, srcX, srcY, srcW, srcH);

    const float fx = static_cast<float>(inner.w) / srcW, fy = static_cast<float>(inner.h) / srcH;
    auto toMini = [&](float cx, float cy) { return SDL_Point{inner.x + px((cx - srcX) * fx), inner.y + px((cy - srcY) * fy)}; };
    auto dot = [&](SDL_Point p, int size, SDL_Color c) {
        if (p.x < inner.x || p.y < inner.y || p.x >= inner.x + inner.w || p.y >= inner.y + inner.h) return;   // вне окна
        visualizer.fillRect({p.x - size / 2 - 1, p.y - size / 2 - 1, size + 2, size + 2}, {0, 0, 0, 255});
        visualizer.fillRect({p.x - size / 2, p.y - size / 2, size, size}, c);
    };

    // Замки и мосты: их нет на картинке карты, рисуем уменьшенные спрайты
    for (const Prop& p : map.props)
        if (SDL_Texture* tex = ResourceManager::getTexture(p.sprite))
        {
            const SDL_Point a = toMini(p.px / TILE_PX, (p.py - p.ph) / TILE_PX);
            const SDL_Point b = toMini((p.px + p.pw) / TILE_PX, p.py / TILE_PX);
            SDL_Rect dst{a.x, a.y, std::max(1, b.x - a.x), std::max(1, b.y - a.y)};
            SDL_Rect clip{};
            if (SDL_IntersectRect(&dst, &inner, &clip) && clip.w == dst.w && clip.h == dst.h)   // у края окна не рисуем
                visualizer.drawTexture(tex, dst.x, dst.y, dst.w, dst.h);
        }

    // Что сейчас видно на экране
    const SDL_Point v0 = toMini(static_cast<float>(lastCamX) / CELL_PX, static_cast<float>(lastCamY) / CELL_PX);
    const int vw = px(static_cast<float>(screen.w) / CELL_PX * fx), vh = px(static_cast<float>(screen.h) / CELL_PX * fy);
    SDL_Rect view{std::max(inner.x, v0.x), std::max(inner.y, v0.y), 0, 0};
    view.w = std::min(inner.x + inner.w, v0.x + vw) - view.x;
    view.h = std::min(inner.y + inner.h, v0.y + vh) - view.y;
    if (view.w > 0 && view.h > 0)                                      // контур: Visualizer::drawRect заливает
    {
        SDL_Renderer* rr = visualizer.getRenderer();
        SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(rr, 255, 255, 255, 170);
        SDL_RenderDrawRect(rr, &view);
    }

    // Города и подземелья, где отряд уже был: синие точки по центру входа
    if (map.mapId == 0)
        for (const Portal& p : map.portals)
            if (player.getVisited().contains(p.name))
                dot(toMini(p.x + p.w * 0.5f, p.y + p.h * 0.5f), 4, {80, 150, 255, 255});

    // Стоящий транспорт, напарники, игрок (мигает)
    static const SDL_Color vehicleColor[kVehicleKinds] = {{220, 170, 110, 255}, {170, 100, 50, 255}, {240, 210, 80, 255}};
    for (int i = 0; i < kVehicleKinds; ++i)
    {
        const VehicleState& vs = player.getVehicles()[i];
        if (vs.available && vs.mapId == player.getMove().mapId && player.getRiding() != static_cast<Vehicle>(i + 1) &&
            static_cast<Vehicle>(i + 1) != Vehicle::Canoe)
            dot(toMini(vs.cell.x + 0.5f, vs.cell.y + 0.5f), 3, vehicleColor[i]);
    }
    for (const auto& [id, remote] : game.getRemotePlayers())
        if (remote.getMove().mapId == player.getMove().mapId)
        {
            const Vec2f p = visualCell(remote.getMove());
            dot(toMini(p.x + 0.5f, p.y + 0.5f), 4, {110, 240, 130, 255});   // напарник: синий занят под места
        }
    const bool blink = (SDL_GetTicks() / 300) % 2 == 0;
    dot(toMini(me.x + 0.5f, me.y + 0.5f), 4, blink ? SDL_Color{255, 60, 60, 255} : SDL_Color{255, 255, 255, 255});
}

bool UIManager::drawVehicle(Vehicle v, Direction dir, int cellScreenX, int cellScreenY, bool flying, bool moving)
{
    // Лист спрайта: 4 колонки по направлениям (Up, Down, Left, Right, как в enum Direction), кадры квадратные.
    // 4 строки: 0-1 — стоит, 2-3 — движется (Pixel Remaster); 2 строки — одни кадры на всё (временные спрайты)
    SDL_Texture* tex = ResourceManager::getTexture(vehicleName(v));
    if (!tex) return false;
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
    const int fw = tw / 4, fh = fw;
    if (fw <= 0 || th < 2 * fh) return false;

    const Uint32 ticks = SDL_GetTicks();
    const int col = std::clamp(static_cast<int>(dir), 0, 3);
    const int base = th >= 4 * fh && moving ? 2 : 0;
    const Uint32 period = moving ? (v == Vehicle::Airship ? 90 : 220) : 500;     // винты быстро, волна медленно
    const SDL_Rect src{col * fw, (base + static_cast<int>((ticks / period) % 2)) * fh, fw, fh};

    const int footX = cellScreenX + CELL_PX / 2, footY = cellScreenY + CELL_PX;
    int lift = 0;
    if (flying)
    {
        // Тень на земле (из листа, если есть) и сам дирижабль выше неё, с лёгким покачиванием
        if (SDL_Texture* shadow = ResourceManager::getTexture("AirshipShadow"))
        {
            SDL_SetTextureAlphaMod(shadow, 140);
            visualizer.drawSprite(shadow, footX, footY - PIXEL_SCALE, PIXEL_SCALE);
        }
        else
        {
            const int sw = fw * PIXEL_SCALE * 3 / 4, sh = 3 * PIXEL_SCALE;
            visualizer.fillRect({footX - sw / 2, footY - sh - PIXEL_SCALE, sw, sh}, {0, 0, 0, 90});
        }
        lift = 10 * PIXEL_SCALE + static_cast<int>(std::lround(std::sin(ticks / 400.0) * PIXEL_SCALE));
    }
    visualizer.drawSprite(tex, footX, footY - lift, PIXEL_SCALE, &src);
    return true;
}

void UIManager::drawActor(const Player& p, int cellScreenX, int cellScreenY)
{
    const bool flying = p.getRiding() == Vehicle::Airship;
    if (p.getRiding() != Vehicle::None &&
        drawVehicle(p.getRiding(), p.getMove().dir, cellScreenX, cellScreenY, flying, flying || p.getMove().moving))
        return;                                                   // на транспорте героя не видно

    // Герой на карте — выбранный клавишей V (павший пропускается): Walk<набор>.png, 4 направления x 2 кадра
    if (const Hero* leader = p.displayedHero())
        if (SDL_Texture* sheet = ResourceManager::getTexture(std::string("Walk") + heroSpriteSet(leader->getClass())))
        {
            int tw = 0, th = 0;
            SDL_QueryTexture(sheet, nullptr, nullptr, &tw, &th);
            const int fw = tw / 4, fh = th / 2;
            const int col = std::clamp(static_cast<int>(p.getMove().dir), 0, 3);
            const int frame = p.getMove().moving ? static_cast<int>((SDL_GetTicks() / 150) % 2) : 0;
            const SDL_Rect src{col * fw, frame * fh, fw, fh};
            visualizer.drawSprite(sheet, cellScreenX + CELL_PX / 2, cellScreenY + CELL_PX, PIXEL_SCALE, &src);
            return;
        }

    SDL_Texture* tex = nullptr;                                   // запасной вариант: старая анимация ходьбы

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
    lastCamX = camX;
    lastCamY = camY;

    if (MapRenderer* r = rendererFor(map))
    {
        r->draw(visualizer.getRenderer(), camX, camY, scr.w, scr.h, PIXEL_SCALE);
        r->drawWater(visualizer.getRenderer(), camX, camY, scr.w, scr.h, PIXEL_SCALE, SDL_GetTicks());
    }

    // Проходимые объекты (мосты) лежат на земле: герой ходит по ним сверху
    for (const Prop& p : map.props)
        if (p.collision != PropCollision::Solid) drawProp(p, camX, camY);

    // Стоящий транспорт лежит на земле и воде, поэтому рисуется до героев
    for (int i = 0; i < kVehicleKinds; ++i)
    {
        const VehicleState& v = player.getVehicles()[i];
        if (!v.available || v.mapId != localMove.mapId || player.getRiding() == static_cast<Vehicle>(i + 1)) continue;
        if (static_cast<Vehicle>(i + 1) == Vehicle::Canoe) continue;           // каноэ не стоит: оно при отряде
        drawVehicle(static_cast<Vehicle>(i + 1), v.dir, v.cell.x * CELL_PX - camX, v.cell.y * CELL_PX - camY, false, false);
    }

    struct Actor { const Player* p; Vec2f pos; };
    std::vector<Actor> actors;
    actors.push_back({ &player, lp });
    for (const auto& [id, remote] : game.getRemotePlayers())
        if (remote.getMove().mapId == localMove.mapId)           // игроки с другой карты не видны
            actors.push_back({ &remote, visualCell(remote.getMove()) });

    // Замки и герои вместе сверху вниз: кто ниже на экране, тот ближе. Герой в ряду замка — перед ним.
    // Летящий дирижабль всегда поверх всего
    struct Drawable { float y; int order; const Actor* actor; const Prop* prop; const NotGamePerson* npc = nullptr; };
    std::vector<Drawable> list;
    for (const Prop& p : map.props)
        if (p.collision == PropCollision::Solid) list.push_back({static_cast<float>(p.bottom), 0, nullptr, &p});
    for (const auto& n : game.npcsHere())
        list.push_back({static_cast<float>(n->getCell().y), 1, nullptr, nullptr, n.get()});
    for (const Actor& a : actors)
        list.push_back({a.pos.y, a.p->getRiding() == Vehicle::Airship ? 2 : 1, &a, nullptr});
    std::sort(list.begin(), list.end(), [](const Drawable& a, const Drawable& b) {
        if ((a.order == 2) != (b.order == 2)) return b.order == 2;
        return a.y != b.y ? a.y < b.y : a.order < b.order;
    });

    for (const Drawable& d : list)
    {
        if (d.prop) drawProp(*d.prop, camX, camY);
        else if (d.npc) drawNpc(*d.npc, d.npc->getCell().x * CELL_PX - camX, d.npc->getCell().y * CELL_PX - camY);
        else drawActor(*d.actor->p, px(d.actor->pos.x * CELL_PX) - camX, px(d.actor->pos.y * CELL_PX) - camY);
    }

    // Надпись по H (бои вкл/выкл): сверху по центру, гаснет за последние полсекунды
    if (!mapNotice.empty() && SDL_GetTicks() - mapNoticeTicks < 1800)
    {
        const Uint32 age = SDL_GetTicks() - mapNoticeTicks;
        const Uint8 alpha = age < 1300 ? 255 : static_cast<Uint8>(255 * (1800 - age) / 500);
        drawOutlinedText(mapNotice, scr.w / 2, 16, {255, 232, 120, 255}, alpha);
    }

    if (game.canLandAirship())                                       // подсказка над дирижаблем
        drawOutlinedText(tr("E - land"), px(lp.x * CELL_PX) - camX + CELL_PX / 2,
                         px(lp.y * CELL_PX) - camY - 12 * PIXEL_SCALE - 20, {255, 255, 255, 255}, 255);
}

void UIManager::drawScene(const std::string &sceneId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return;

    // Экраны поверх игры (игровое меню и всё, что из него открыто) показывают приглушённую карту
    const bool overGame = game.isGameInProgress() &&
                          (sceneId == "GameMenu" || sceneId == "Saves" || sceneId == "Options" || sceneId == "Shop");

    if (sceneId == "Map" || overGame)
        drawMapWorld();                               // под интерфейсом

    if (overGame)
    {
        if (sceneId != "GameMenu" && sceneId != "Shop") visualizer.fillRect(screen, {0, 0, 0, 120});
    }
    else
        scene->draw(visualizer, game.getScreenRect());

    if (sceneId == "Map" && game.minimapEnabled() && !isDialogOpen())
        drawMinimap();                                // поверх интерфейса карты
    if (sceneId == "Map" && isDialogOpen())
        drawDialog();
    if (sceneId == "Map")
        if (const float a = game.getFadeAlpha(); a > 0.0f)      // переход через портал: затемнение поверх всего
            visualizer.fillRect(screen, {0, 0, 0, static_cast<Uint8>(255 * a)});
    if (sceneId == "GameMenu")
        drawGameMenu();
    if (sceneId == "Shop")
        drawShop();
    if (sceneId == "Options")
        drawConfig();
    if (sceneId == "Saves")
        drawSavesScreen();
    if (sceneId == "Battle")
        drawBattleOverlay();                          // поверх окон: стрелки и числа урона
    if (sceneId == "Battle" && bestiaryOpen)
        drawBestiary();

    // Из игры музыка карты продолжает играть; экран слотов никогда не меняет трек
    if (!overGame && sceneId != "Saves")
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
                                               ResourceManager::getFont(kUiFont), {0, 0, 0, 0}));
    setTexture("FPS", sceneId,
               ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()),
                                               ResourceManager::getFont(kUiFont), {0, 0, 0, 0}));

    if (sceneId == "Battle")                   // ветка для Map больше не нужна
        updateBattleData(scene);
    else if (sceneId == "Menu")
        updateMenuButtons();

}

void UIManager::updateBattleData(Scene &scene)
{
    const SDL_Rect scr = game.getScreenRect();
    const int heroCount = static_cast<int>(player.getHeroes().size());

    // «Имя 120/180» в окне отряда: слишком длинное имя обрезаем с многоточием, чтобы не залезало в соседнее окно
    TTF_Font* statusFont = ResourceManager::getFont(kUiFont);
    const int statusW = getRect("HerosWindow", "Battle").w - 24;
    auto statusLine = [&](std::string name, const GamePerson& h) {
        const std::string hp = " " + std::to_string(h.getCurrentHp()) + "/" + std::to_string(h.getMaxHp());
        if (ResourceManager::measureText(name + hp, statusFont).w > statusW)
        {
            while (!name.empty() && ResourceManager::measureText(name + "…" + hp, statusFont).w > statusW)
                utf8::popBack(name);
            name += "…";
        }
        return name + hp + "\n";
    };

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
            remoteHeroText += statusLine(hero.getName(), hero);
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
        heroText += statusLine(hero.getName(), hero);
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
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    const SDL_Color white{255, 255, 255, 255}, gray{120, 128, 168, 255};
    setTexture("ItemsButton", "Battle", ResourceManager::getTextTexture(tr("Items"), font, hasItems() ? white : gray));
    setTexture("MagicButton", "Battle",
               ResourceManager::getTextTexture(tr("Magic"), font, choosing && hasUsableSkill() ? white : gray));

    // Враги
    const auto& enemies = game.getBattle()->getEnemies();
    const Uint32 now = SDL_GetTicks();
    constexpr double kPiD = 3.14159265358979;
    for (int i = 0; i < static_cast<int>(enemies.size()) && i < MAX_ENEMY_COUNT; ++i)
    {
        if (const Uint32 hit = trackHp(lastEnemyHp[i], static_cast<int>(enemies[i].getCurrentHp()), enemySlotRect[i]))
        {
            enemyHitAt[i] = hit;
            if (!enemies[i].getIsAlive()) enemyDeathAt[i] = hit;     // исчезнет после удара, а не до него
        }
        if (!enemies[i].getIsAlive() && enemyDeathAt[i] == 0) enemyDeathAt[i] = now;   // пал без видимого удара

        SDL_Rect r = enemySlotRect[i];
        if (enemyLungeAt[i] && now - enemyLungeAt[i] < 350)      // шаг к героям и обратно
            r.x += static_cast<int>(10 * PIXEL_SCALE * std::sin(kPiD * (now - enemyLungeAt[i]) / 350.0));
        if (enemyHitAt[i] && now >= enemyHitAt[i] && now - enemyHitAt[i] < 300)   // вздрогнул от удара
            r.x += static_cast<int>(3 * PIXEL_SCALE * std::sin(kPiD * 6.0 * (now - enemyHitAt[i]) / 300.0));

        bool visible = true;                                     // смертельный удар: мигает и пропадает
        if (enemyDeathAt[i] && now >= enemyDeathAt[i])
            visible = now - enemyDeathAt[i] < 400 && (now / 60) % 2 == 0;

        const std::string id = "Enemy" + std::to_string(i + 1);
        setRect(id, "Battle", r);
        if (UIObject* obj = findUIObject(id, "Battle")) obj->isVisible = visible;
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

    // ── Враги: сетка до 3x3 слева, по колонкам сверху вниз ─────────────
    const auto& enemies = game.getBattle()->getEnemies();
    const int n = std::min<int>(static_cast<int>(enemies.size()), MAX_ENEMY_COUNT);

    std::array<SDL_Point, MAX_ENEMY_COUNT> nat{};   // родные размеры спрайтов
    for (int i = 0; i < n; ++i)
        if (const EnemyDef* def = GameData::instance().enemy(enemies[i].getEnemyId()))
            nat[i] = {def->srcRect.w, def->srcRect.h};

    const int columns = n <= 2 ? 1 : n <= 6 ? 2 : 3;
    const int rows = (n + columns - 1) / std::max(1, columns);
    const SDL_Rect area{margin, margin, 560 - margin, fieldH - 2 * margin};
    const int cellW = area.w / columns, cellH = area.h / std::max(1, rows);

    for (int i = 0; i < n; ++i)
    {
        // Свой масштаб у каждого: крупный враг уменьшается, чтобы влезть в клетку, мелкие остаются как есть.
        // Целым шагом, а не дробью — пиксели не плывут
        int es = S;
        while (es > 1 && (nat[i].x * es > cellW - 8 || nat[i].y * es > cellH - 4)) --es;
        const int c = i / rows, r = i % rows;
        const int cx = area.x + cellW * c + cellW / 2;
        const int bottom = area.y + cellH * (r + 1) - 6;                   // «ноги» на нижнем крае клетки
        const int w = nat[i].x * es, h = nat[i].y * es;
        enemySlotRect[i] = {cx - w / 2, bottom - h, w, h};
    }
    for (int i = 0; i < MAX_ENEMY_COUNT; ++i)
    {
        const std::string id = "Enemy" + std::to_string(i + 1);
        if (i < n && nat[i].x > 0) setRect(id, "Battle", enemySlotRect[i]);
        else                       setTexture(id, "Battle", nullptr);      // пустой слот: убираем остатки прошлого боя
    }

    // ── Герои: две колонки справа (свои и напарника) ──────────────────
    const int hw = kHeroFrame * S, hh = kHeroFrame * S;
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
    for (int i = 0; i < MAX_ENEMY_COUNT; i++)
    {
        const std::string id = "Enemy" + std::to_string(i + 1);
        setTexture(id, "Battle", i < static_cast<int>(enemies.size())
                                     ? ResourceManager::getTexture(enemies[i].getTextureName()) : nullptr);
        if (UIObject* obj = findUIObject(id, "Battle")) obj->isVisible = true;   // прошлый бой мог оставить скрытым
    }
    layoutBattle();                            // сначала враги, потому что addCharacters использует позиции героев
    resetHpTracking();                         // новый бой: первый кадр только запоминает HP
    fx.clear();
    closeList();
    closeBestiary();
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
        if (count > 0 && game.getItem(id).usableInBattle()) return true;    // экипировка в бою не применяется
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
    TTF_Font* font = ResourceManager::getFont(kUiFont);
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
        if (count == 0 || !game.getItem(itemId).usableInBattle()) continue;
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

    // Любая цель, даже «на себя» и «на всех», подтверждается: курсоры показывают, кого заденет, Esc отменяет
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
    if (currentType == TargetType::Enemy) return i >= 0 && i < MAX_ENEMY_COUNT ? enemySlotRect[i] : SDL_Rect{};
    if (i < 0 || i > 3) return {};
    if (currentType != TargetType::Ally && currentType != TargetType::Self) return {};
    SDL_Rect r = heroSlotRect[i];
    if (game.getBattle()->isPlayerChoosing() && i == game.getBattle()->getCurrentHeroIndex())
        r.x -= 8 * PIXEL_SCALE;                              // ходящий герой стоит на шаг впереди
    return r;
}

void UIManager::moveSelector()
{
    currentType = pendingAction.targetType;
    selectedTargetIndex = currentType == TargetType::Self ? currentHeroIndex : 0;
    snapSelectorToTarget();                                  // первая живая цель (у «на себя» и «на всех» — без хода)
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
    SDL_Texture* t = ResourceManager::getTextTexture(text, ResourceManager::getFont(kUiFont), kWhite);
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
    enemyHitAt.fill(0);
    enemyDeathAt.fill(0);
    enemyLungeAt.fill(0);
    actionBanner.clear();
    lastHeroHp.fill(-1);
    lastRemoteHp.fill(-1);
    popups.clear();
}

Uint32 UIManager::trackHp(int& last, int hp, const SDL_Rect& slot)
{
    Uint32 hitAt = 0;
    // Числа берём из разницы HP между кадрами: так они работают и у клиента, которому HP приходят снапшотом
    if (last >= 0 && hp != last && slot.w > 0)
    {
        const int delta = hp - last;
        const SDL_Color c = delta < 0 ? SDL_Color{255, 255, 255, 255} : SDL_Color{96, 255, 120, 255};
        const int x = slot.x + slot.w / 2;
        int y = slot.y + slot.h / 3;
        for (const auto& p : popups)                         // два удара подряд по одной цели: второе число выше
            if (p.x == x && static_cast<int>(SDL_GetTicks() - p.start) < 400) y = std::min(y, p.y - 22);

        // Свой эффект запускает бой (удар оружием, заклинание). Если его нет — например, у клиента в коопе,
        // которому HP пришли снапшотом, — хотя бы общий удар или лечение
        const SDL_Point center{slot.x + slot.w / 2, slot.y + slot.h / 2};
        Uint32 when = SDL_GetTicks();
        if (const FxInstance* e = recentEffectAt(center))
            when = e->start + static_cast<Uint32>(e->layer.delay * 1000.0f);   // число вместе с попаданием
        else
            playEffect(delta < 0 ? "hit" : "heal", center);
        popups.push_back({x, y, std::to_string(std::abs(delta)), c, when});
        if (delta < 0) hitAt = std::max<Uint32>(when, 1);
    }
    last = hp;
    return hitAt;
}

void UIManager::enemyLunge(int index)
{
    if (index >= 0 && index < MAX_ENEMY_COUNT) enemyLungeAt[index] = SDL_GetTicks();
}

void UIManager::showActionName(const std::string& text)
{
    actionBanner = text;
    actionBannerAt = SDL_GetTicks();
}

// ─────────────────────────────────────────
// Боевые эффекты
// ─────────────────────────────────────────

void UIManager::playEffect(const std::string& id, SDL_Point center, float delay)
{
    const EffectDef* def = GameData::instance().effect(id);
    if (!def) return;
    const Uint32 now = SDL_GetTicks();
    for (EffectLayer layer : def->layers)
    {
        SDL_Texture* tex = ResourceManager::getTexture(layer.sheet);
        if (!tex) continue;
        layer.delay += delay;
        fx.push_back({layer, tex, center, now});
    }
}

SDL_Rect UIManager::battlerRect(bool onHero, uint32_t owner, int index) const
{
    if (!onHero) return index >= 0 && index < MAX_ENEMY_COUNT ? enemySlotRect[index] : SDL_Rect{};
    if (index < 0 || index > 3) return {};
    if (owner == game.localPlayerId) return heroSlotRect[index];

    int slot = 0;                                            // герои напарников идут подряд, как в updateBattleData
    for (const auto& [id, remote] : game.getRemotePlayers())
    {
        if (!game.getBattle()->isParticipant(id)) continue;
        if (id == owner) return slot + index < 4 ? remoteSlotRect[slot + index] : SDL_Rect{};
        slot += static_cast<int>(remote.getHeroes().size());
    }
    return {};
}

void UIManager::playEffectOn(const std::string& id, bool onHero, uint32_t owner, int index, float delay)
{
    const SDL_Rect r = battlerRect(onHero, owner, index);
    if (r.w > 0) playEffect(id, {r.x + r.w / 2, r.y + r.h / 2}, delay);
}

void UIManager::playSwing(int swing, const std::string& heroImageId, float delay)
{
    if (swing < 0) return;
    char name[16];
    std::snprintf(name, sizeof(name), "Swing%02d", swing);
    SDL_Texture* tex = ResourceManager::getTexture(name);
    if (!tex) return;

    // Оружие у передней руки героя. Числа — в пикселях кадра героя 32x32 (на экране умножаются на PIXEL_SCALE):
    // рука около (11, 14), а на замахе герой ещё шагнул влево примерно на 12 px
    constexpr int kHandX = 11 - 12, kHandY = 14;
    constexpr float kSwingScale = 0.8f;                      // замах чуть меньше героя
    const SDL_Rect r = getRect(heroImageId, "Battle");
    const int S = PIXEL_SCALE;
    const SDL_Point hand{r.x + kHandX * S, r.y + r.h - kHeroFrame * S + kHandY * S};

    EffectLayer layer;
    layer.sheet = name;
    layer.cols = 4;
    layer.frames = 4;
    layer.fps = 14.0f;
    layer.scale = S * kSwingScale;
    layer.add = false;
    layer.delay = delay;
    fx.push_back({layer, tex, hand, SDL_GetTicks()});
}

const UIManager::FxInstance* UIManager::recentEffectAt(SDL_Point p) const
{
    const Uint32 now = SDL_GetTicks();
    for (auto it = fx.rbegin(); it != fx.rend(); ++it)
        if (now - it->start < 150 && std::abs(it->at.x - p.x) < 40 && std::abs(it->at.y - p.y) < 40)
            return &*it;
    return nullptr;
}

void UIManager::drawEffects()
{
    SDL_Renderer* rr = visualizer.getRenderer();
    const Uint32 now = SDL_GetTicks();
    std::erase_if(fx, [&](const FxInstance& f) {
        return static_cast<float>(now - f.start) / 1000.0f - f.layer.delay >= f.layer.length();
    });

    for (const FxInstance& f : fx)
    {
        const EffectLayer& L = f.layer;
        const float age = static_cast<float>(now - f.start) / 1000.0f - L.delay;
        if (age < 0.0f) continue;
        const float t = std::clamp(age / L.length(), 0.0f, 1.0f);

        int tw = 0, th = 0;
        SDL_QueryTexture(f.tex, nullptr, nullptr, &tw, &th);
        const int fw = tw / L.cols, fh = th / L.rows;
        const int frame = L.frames > 1 ? std::min(L.frames - 1, static_cast<int>(age * L.fps)) : 0;
        const SDL_Rect src{(frame % L.cols) * fw, (frame / L.cols) * fh, fw, fh};

        const float scale = L.scaleEnd < 0.0f ? L.scale : L.scale + (L.scaleEnd - L.scale) * t;
        const int w = static_cast<int>(fw * scale), h = static_cast<int>(fh * scale);
        const SDL_Rect dst{f.at.x + L.dx - w / 2, f.at.y + L.dy - h / 2, w, h};

        SDL_SetTextureBlendMode(f.tex, L.add ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(f.tex, L.color.r, L.color.g, L.color.b);
        SDL_SetTextureAlphaMod(f.tex, L.fade ? static_cast<Uint8>(255 * (1.0f - t)) : 255);
        SDL_RenderCopyEx(rr, f.tex, &src, &dst, L.angle, nullptr, SDL_FLIP_NONE);
        SDL_SetTextureColorMod(f.tex, 255, 255, 255);      // текстуры общие: возвращаем как было
        SDL_SetTextureAlphaMod(f.tex, 255);
    }
}

std::string UIManager::effectForSkill(const Skill& skill)
{
    if (!skill.effect.empty()) return skill.effect;
    switch (skill.effectType)
    {
        case SkillEffectType::Heal:   return "heal";
        case SkillEffectType::Buff:   return "buff";
        case SkillEffectType::Debuff: return "debuff";
        default: break;
    }
    switch (skill.element)
    {
        case Elemental::Fire: case Elemental::Lava:  return "fire";
        case Elemental::Lightning: case Elemental::Air: return "thunder";
        case Elemental::Ice: case Elemental::Water:  return "ice";
        case Elemental::Poison:                      return "poison";
        default:                                     return "slash";
    }
}

std::string UIManager::effectForItem(const Item& item)
{
    if (!item.effect.empty()) return item.effect;
    switch (item.effectType)
    {
        case ItemEffectType::Heal:        return "heal";
        case ItemEffectType::RestoreMana: return "mana";
        case ItemEffectType::Damage:      return "fire";
        case ItemEffectType::Buff:        return "buff";
        case ItemEffectType::Debuff:      return "debuff";
        default:                          return "hit";
    }
}

void UIManager::drawBattleOverlay()
{
    const Uint32 now = SDL_GetTicks();
    const int bob = (now / 220) % 2 * 3;                     // стрелка «дышит» на 3 px

    // Плашка «кто и что делает» во время хода
    if (!actionBanner.empty() && now - actionBannerAt < 1200 && uiState != UIChooseState::ChooseTarget)
    {
        TTF_Font* font = ResourceManager::getFont(kUiFont);
        const TextSize ts = ResourceManager::measureText(actionBanner, font);
        const SDL_Rect win{screen.w / 2 - (ts.w + 32) / 2, 10, ts.w + 32, ts.h + 16};
        visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), win, kWindowSkinSlice, 1);
        drawText(actionBanner, win.x + 16, win.y + 8, kWhite);
    }

    // Указатель на цель и окно с её именем. «На всех» — курсор на каждой живой цели, они мигают по очереди
    if (uiState == UIChooseState::ChooseTarget &&
        (currentType == TargetType::AllEnemies || currentType == TargetType::AllAllies))
    {
        const bool enemiesSide = currentType == TargetType::AllEnemies;
        const int count = enemiesSide ? static_cast<int>(game.getBattle()->getEnemies().size())
                                      : static_cast<int>(player.getHeroes().size());
        for (int i = 0; i < count && i < (enemiesSide ? MAX_ENEMY_COUNT : 4); ++i)
        {
            const bool alive = enemiesSide ? game.getBattle()->getEnemies()[i].getIsAlive()
                                           : player.getHeroes()[i].getIsAlive();
            if (!alive || (now / 180 + i) % 2) continue;         // мигают вразнобой, как в FF
            SDL_Rect t = enemiesSide ? enemySlotRect[i] : heroSlotRect[i];
            if (!enemiesSide && game.getBattle()->isPlayerChoosing() && i == game.getBattle()->getCurrentHeroIndex())
                t.x -= 8 * PIXEL_SCALE;                          // ходящий герой на шаг впереди
            drawPointer(t.x - 4, t.y + t.h / 2, PIXEL_SCALE);
        }
        const std::string label = tr(enemiesSide ? "All enemies" : "All allies");
        const TextSize ts = ResourceManager::measureText(label, ResourceManager::getFont(kUiFont));
        const SDL_Rect win{screen.w / 2 - (ts.w + 32) / 2, 10, ts.w + 32, ts.h + 16};
        visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), win, kWindowSkinSlice, 1);
        drawText(label, win.x + 16, win.y + 8, kWhite);
    }
    else if (uiState == UIChooseState::ChooseTarget)
    {
        const SDL_Rect t = selectedTargetRect();
        if (t.w > 0)
        {
            drawPointer(t.x - 4 - bob, t.y + t.h / 2, PIXEL_SCALE);

            const std::string name = currentType == TargetType::Enemy
                    ? game.getBattle()->getEnemies().at(selectedTargetIndex).getName()
                    : player.getHeroes().at(selectedTargetIndex).getName();
            SDL_Texture* tex = ResourceManager::getTextTexture(name, ResourceManager::getFont(kUiFont), kWhite);
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
    {
        float lx = 0.f, ly = 0.f;                                 // окно может быть растянуто: в координаты игры
        SDL_RenderWindowToLogical(visualizer.getRenderer(), mx, my, &lx, &ly);
        mx = static_cast<int>(lx);
        my = static_cast<int>(ly);
    }
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
        for (const char* id : {"AttackButton", "ItemsButton", "MagicButton", "BookButton", "Run"}) pointAt(id);

    drawEffects();

    // Всплывающие числа: подскок вверх, пауза, затухание. Старт может быть отложен до попадания
    constexpr int kRise = 250, kLife = 1100, kFade = 300;
    std::erase_if(popups, [&](const Popup& p) { return static_cast<int>(now - p.start) >= kLife; });
    for (const auto& p : popups)
    {
        const int age = static_cast<int>(now - p.start);
        if (age < 0) continue;
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
constexpr SpecializationNames kPickable[] = {                    // порядок выбора стрелками, как в FF1
    SpecializationNames::Warrior,  SpecializationNames::Thief,     SpecializationNames::Monk,
    SpecializationNames::RedMage,  SpecializationNames::WhiteMage, SpecializationNames::BlackMage };
constexpr int kPickableCount = static_cast<int>(std::size(kPickable));
}

void UIManager::initCreateScene()
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
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
        addImage("Portrait" + n, "CreatePlayer", {x + slotW / 2 - 64, top + 16, 128, 128}, nullptr);
        addButton("Prev" + n, "CreatePlayer", {x + 10, top + 65, 30, 30},
                  ResourceManager::getTextTexture("<", font, white), [this, i]() { cycleClass(i, -1); }, hover);
        addButton("Next" + n, "CreatePlayer", {x + slotW - 40, top + 65, 30, 30},
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
    pickedClass = {0, 1, 4, 5};                                // Воин, Вор, Белый и Чёрный маг
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
    const std::string className = tr(specializationNamesString.at(cls));
    setText("Class" + n, "CreatePlayer", className);
    if (auto* label = dynamic_cast<Label*>(findUIObject("Class" + n, "CreatePlayer")))
    {
        const SDL_Rect card = getRect("Slot" + n, "CreatePlayer");
        const int w = ResourceManager::measureText(className, ResourceManager::getFont(kUiFont)).w;
        label->background.rect.x = card.x + (card.w - w) / 2 - label->paddingX;
        label->rect.x = label->background.rect.x + label->paddingX;
    }
    setText("Stats" + n, "CreatePlayer",
            "HP  " + std::to_string(def->hp) + "\nMP  " + std::to_string(def->mp) +
            "\n" + tr("ATK") + " " + std::to_string(def->attack) + "\n" + tr("DEF") + " " + std::to_string(def->defence) +
            "\n" + tr("AGI") + " " + std::to_string(def->agility));

    const std::string current = getText("Name" + n, "CreatePlayer");
    if (current.empty() || current == autoName[slot])        // имя по умолчанию следует за классом, своё не трогаем
        setText("Name" + n, "CreatePlayer", className);
    autoName[slot] = className;
}

void UIManager::cycleClass(int slot, int dir)
{
    pickedClass[slot] = (pickedClass[slot] + dir + kPickableCount) % kPickableCount;
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
    addScene(gameStateString.at(GameState::Saves));
    addScene(gameStateString.at(GameState::GameMenu));
    addScene(gameStateString.at(GameState::Shop));      // окна рисует drawShop() поверх карты

    initMapScene();
    initMenuScene();
    initOptionsScene();
    initOnlineScene();
    initBattleScene();
    initCreateScene();
    initSavesScene();
    initGameMenuScene();
    applyLanguage();
}

void UIManager::applyLanguage()
{
    // Надписи, которые сцены создают один раз картинками: пересоздаём на текущем языке.
    // Ширину кнопки считаем по тексту, чтобы буквы не сплющивались
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    auto relabel = [&](const std::string& id, const std::string& scene, const std::string& text, SDL_Color color, int h) {
        SDL_Texture* tex = ResourceManager::getTextTexture(text, font, color);
        setTexture(id, scene, tex);
        const TextSize ts = ResourceManager::measureText(text, font);
        SDL_Rect r = getRect(id, scene);
        if (ts.h > 0) { r.w = ts.w * h / ts.h; r.h = h; }
        setRect(id, scene, r);
    };

    const SDL_Color menuColor{0, 0, 0, 0};
    for (const auto& [id, text] : std::initializer_list<std::pair<const char*, const char*>>{
             {"Continue", "Continue"}, {"New game", "New game"}, {"Load", "Load"},
             {"Online", "Co-op"}, {"Options", "Options"}, {"Quit", "Quit"}})
        relabel(id, "Menu", tr(text), menuColor, 30);
    updateMenuButtons();

    const SDL_Color white{255, 255, 255, 255}, black{0, 0, 0, 255};
    int commandsW = 0;
    for (const auto& [id, text] : std::initializer_list<std::pair<const char*, const char*>>{
             {"AttackButton", "Attack"}, {"ItemsButton", "Items"}, {"MagicButton", "Magic"},
             {"BookButton", "Book"}, {"Run", "Run"}})
    {
        relabel(id, "Battle", tr(text), white, 25);
        commandsW = std::max(commandsW, getRect(id, "Battle").w);
    }
    if (auto* l = dynamic_cast<Label*>(findUIObject("ActorInfo", "Battle")))
    {
        l->background.rect.x = 12 + commandsW + 16;
        l->rect.x = l->background.rect.x + l->paddingX;
    }

    // Окно списка умений и предметов — правее самой широкой команды, чтобы не закрывать её
    SDL_Rect listWin = getRect("EnvironmentWindow", "Battle");
    const int listX = std::max(84, 12 + commandsW + 12);
    listWin.w += listWin.x - listX;
    listWin.x = listX;
    setRect("EnvironmentWindow", "Battle", listWin);
    const int rows = ENV_BUTTON_COUNT / 2;
    const int cellW = (listWin.w - 3 * 28) / 2;
    for (int i = 0; i < ENV_BUTTON_COUNT; ++i)
    {
        listRect[i].x = listWin.x + 28 + (i / rows) * (cellW + 28);
        listRect[i].w = cellW;
        setRect("EnButton" + std::to_string(i + 1), "Battle", listRect[i]);
        setRect("EnInfo" + std::to_string(i + 1), "Battle", listRect[i]);
    }
    setText("ResultHint", "Battle", tr("Press Enter"));

    setText("CreateTitle", "CreatePlayer", tr("Choose your party"));
    setTexture("StartButton", "CreatePlayer", ResourceManager::getTextTexture(tr("Start"), font, black));
    setTexture("BackToMenu", "CreatePlayer", ResourceManager::getTextTexture(tr("Back"), font, black));
    for (int i = 0; i < 4; ++i) refreshPartySlot(i);

    setText("IPImage", "Online", tr("Your IP: ") + game.getIP());
    setText("ConnectImage", "Online", tr("Enter friend ip"));
    setTexture("StartServerButton", "Online", ResourceManager::getTextTexture(tr("Start server"), font, menuColor));
    setTexture("ConnectButton", "Online", ResourceManager::getTextTexture(tr("Connect"), font, menuColor));
}

void UIManager::initMapScene()
{
    TTF_Font *font = ResourceManager::getFont(kUiFont);

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
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    addImage("MenuBackground", "Menu", screen, ResourceManager::getTexture("MenuBackground"));
    addImage("Time", "Menu", {screen.w - 90, 10, 80, 30},
             ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));
    addMusic("MenuTheme", "Menu");
    addSound("ButtonHover", "Menu");
    addPanel("ButtonPanel", "Menu");

    auto hover = [this]() { playSound("Menu", "ButtonHover"); };
    // Стартовое меню: сохраняться отсюда нечего, это делается в игровом меню. Continue и Load красит updateMenuButtons()
    struct MenuItem { const char* id; const char* text; std::function<void()> onClick; };
    const MenuItem items[] = {
        {"Continue", "Continue", [this]() { game.continueGame(); }},
        {"New game", "New game", [this]() { game.openPartyCreation(); }},
        {"Load",     "Load",     [this]() { game.openSaveScreen(false); }},
        {"Online",   "Co-op",    [this]() { game.openOnlineMenu(); }},
        {"Options",  "Options",  [this]() { game.openOptions(GameState::Menu); }},
        {"Quit",     "Quit",     [this]() { game.endGame(); }},
    };
    // Один масштаб на все пункты: ширина считается от текста, поэтому буквы везде одного размера
    constexpr int kButtonH = 30;
    for (int i = 0; i < static_cast<int>(std::size(items)); ++i)
    {
        const TextSize ts = ResourceManager::measureText(items[i].text, font);
        const int w = ts.h > 0 ? ts.w * kButtonH / ts.h : 0;
        addButton(items[i].id, "Menu", {120, screen.h / 2 + 100 + i * 25, w, kButtonH},
                  ResourceManager::getTextTexture(items[i].text, font, {0, 0, 0, 0}),
                  items[i].onClick, hover, "ButtonPanel");
    }
    addImage("FPS", "Menu", {10, 10, 100, 30},
             ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
}

void UIManager::initOptionsScene()
{
    // Само окно настроек рисует drawConfig(): громкость, «Back» и подсказка по управлению
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    addImage("OptionsBackground", "Options", screen,
             ResourceManager::getTexture("OptionsBackground"), {0, 0, 1920, 1000});
    addImage("FPS", "Options", {0, 0, 0, 0}, ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
    addImage("Time", "Options", {0, 0, 0, 0}, ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));
    addMusic("OptionsTheme", "Options");
}

void UIManager::initGameMenuScene()
{
    // Под меню рисуется карта (drawScene), окна рисует drawGameMenu()
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    addImage("FPS", "GameMenu", {0, 0, 0, 0}, ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
    addImage("Time", "GameMenu", {0, 0, 0, 0}, ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));
}

void UIManager::initOnlineScene()
{
    TTF_Font *font = ResourceManager::getFont(kUiFont);
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
    std::string text = tr("Victory!") + "\n\n";
    text += trf("Obtained {0} Gil", {std::to_string(r.gold)}) + "\n";
    text += trf("Each hero earned {0} EXP", {std::to_string(r.expEach)}) + "\n";
    for (const auto& up : r.levelUps)
        text += "\n" + trf("{0} reached level {1}!", {up.name, std::to_string(up.newLevel)});
    if (!r.items.empty())
        text += "\n";
    for (const auto& item : r.items)
        text += "\n" + trf("Found {0}", {item});

    // Окно растёт под текст: уровни и трофеи могут добавить много строк
    const int lines = static_cast<int>(std::count(text.begin(), text.end(), '\n')) + 1;
    const int h = std::max(230, 40 + lines * 26 + 40);
    const int top = std::max(10, screen.h / 2 - 70 - h / 2);
    setRect("ResultWindow", "Battle", {screen.w / 2 - 230, top, 460, h});
    setText("ResultText", "Battle", text);
    auto moveLabel = [this](const std::string& id, int y) {      // метка рисует текст по своему rect, фон по background
        if (auto* l = dynamic_cast<Label*>(findUIObject(id, "Battle")))
        {
            l->background.rect.y = y;
            l->rect.y = y + l->paddingY;
        }
    };
    moveLabel("ResultText", top + 15);
    moveLabel("ResultHint", top + h - 45);
    setEnVI("ActionPanel", "Battle", false);
    closeList();
    uiState = UIChooseState::ChooseAction;
    setEnVI("ResultPanel", "Battle", true);
}

void UIManager::setBattleBackground(const std::string& name)
{
    SDL_Texture* tex = ResourceManager::getTexture("BB" + name);
    if (!tex) tex = ResourceManager::getTexture("BBPlains");
    if (!tex) tex = ResourceManager::getTexture("BattleImage0");
    setTexture("BattleBackground", "Battle", tex);

    // Заполняем весь экран без искажения: масштаб по большей стороне, лишнее по бокам (или снизу) обрезаем
    int tw = 0, th = 0;
    if (tex) SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
    if (tw <= 0 || th <= 0) return;
    const float scale = std::max(static_cast<float>(screen.w) / tw, static_cast<float>(screen.h) / th);
    const int sw = std::min(tw, static_cast<int>(screen.w / scale)), sh = std::min(th, static_cast<int>(screen.h / scale));
    setSrcRect("BattleBackground", "Battle", {(tw - sw) / 2, 0, sw, sh});
}

void UIManager::hideBattleResults()
{
    setEnVI("ResultPanel", "Battle", false);
    setEnVI("ActionPanel", "Battle", true);                      // для следующего боя
}

void UIManager::initBattleScene()
{
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    auto hover = [this]() { playSound("Battle", "ButtonHover"); };
    const SDL_Color white{255, 255, 255, 255};

    const int barY = 452;                     // верх нижней панели
    const int barH = screen.h - barY;
    const int colW = 200;                     // ширина окон со статусом

    // ── Всё, что лежит ПОД панелями, добавляется до addPanel ─────────────
    // Задник на весь экран, как в Pixel Remaster: нижние окна полупрозрачные и лежат поверх него
    addImage("BattleBackground", "Battle",
             {0, 0, screen.w, screen.h}, ResourceManager::getTexture("BattleImage0"));

    addWindow("CommandWindow", "Battle", {0, barY, screen.w - 3 * colW, barH});
    addWindow("HerosWindow",   "Battle", {screen.w - 3 * colW, barY, colW, barH});
    addWindow("EnemiesWindow", "Battle", {screen.w - 2 * colW, barY, colW, barH});
    addWindow("RemoteWindow",  "Battle", {screen.w - colW,     barY, colW, barH});
    for (const char* id : {"CommandWindow", "HerosWindow", "EnemiesWindow", "RemoteWindow"})
        if (auto* w = dynamic_cast<Window*>(findUIObject(id, "Battle"))) w->alpha = 205;

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
    addButton("Run", "Battle", {12, barY + 114, 60, 25},
              ResourceManager::getTextTexture("Run", font, white),
              [this]() { game.endRandomBattle(); }, hover, "ActionPanel");
    addButton("BookButton", "Battle", {12, barY + 88, 60, 25},
              ResourceManager::getTextTexture("Book", font, white),
              [this]() { openBestiaryFromBattle(); }, hover, "ActionPanel");

    // Окно списка умений и предметов закрывает всю нижнюю панель правее команд.
    // Две колонки по четыре строки: слева название, справа стоимость или количество.
    const SDL_Rect listWin{84, barY, screen.w - 84, barH};
    addWindow("EnvironmentWindow", "Battle", listWin, "EnvironmentPanel");
    if (auto* w = dynamic_cast<Window*>(findUIObject("EnvironmentWindow", "Battle"))) w->alpha = 225;
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
    for (int i = 0; i < MAX_ENEMY_COUNT; i++)
        addImage("Enemy" + std::to_string(i + 1), "Battle", {0, 0, 0, 0}, nullptr, {0, 0, 0, 0}, "EnemyPanel");
    for (int i = 0; i < 4; i++)
    {
        const std::string n = std::to_string(i + 1);
        addImage("Hero" + n,       "Battle", {0, 0, 0, 0}, nullptr, {0, 0, 0, 0}, "HeroPanel");   // вся картинка
        addImage("RemoteHero" + n, "Battle", {0, 0, 0, 0}, nullptr, {0, 0, 0, 0}, "HeroPanel");
    }

    addPanel("ResultPanel", "Battle");
    addWindow("ResultWindow", "Battle", {screen.w / 2 - 230, 110, 460, 230}, "ResultPanel");
    addLabel("ResultText", "Battle", {screen.w / 2 - 210, 125, 0, 0}, nullptr, "", font,
             {255, 255, 255, 255}, 8, 4, "ResultPanel");
    addLabel("ResultHint", "Battle", {screen.w / 2 - 210, 300, 0, 0}, nullptr, "Press Enter", font,
             {255, 255, 255, 255}, 8, 4, "ResultPanel");
    setEnVI("ResultPanel", "Battle", false);
}
// ─────────────────────────────────────────
// Экран сохранений
// ─────────────────────────────────────────

namespace {
    const SDL_Color kSaveText  {255, 255, 255, 255};
    const SDL_Color kSaveDim   {150, 160, 200, 255};             // подписи и пустые слоты
    const SDL_Color kSaveHot   {255, 232, 120, 255};             // название выбранного слота
    const SDL_Color kSaveBad   {255, 120, 120, 255};

    std::string formatPlayTime(uint32_t seconds)
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02u:%02u", seconds / 3600, seconds / 60 % 60);
        return buf;
    }

    std::string formatDate(int64_t unixTime)
    {
        if (unixTime <= 0) return "-";
        const std::time_t t = static_cast<std::time_t>(unixTime);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &t);
#else
        localtime_r(&t, &local);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", &local);
        return buf;
    }
}

void UIManager::initSavesScene()
{
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    addImage("SavesBackground", "Saves", screen,
             ResourceManager::getTexture("OptionsBackground"), {0, 0, 1920, 1000});
    // Время и FPS обновляются в каждой сцене, поэтому они нужны и здесь; прячем под окнами
    addImage("FPS", "Saves", {0, 0, 0, 0}, ResourceManager::getTextTexture("FPS: 0", font, {0, 0, 0, 0}));
    addImage("Time", "Saves", {0, 0, 0, 0}, ResourceManager::getTextTexture("00:00", font, {0, 0, 0, 0}));
}

std::string UIManager::slotName(int slot)
{
    return slot == 0 ? tr("Autosave") : trf("Slot {0}", {std::to_string(slot)});
}

void UIManager::openSaveScreen(bool saving)
{
    savesSaving = saving;
    savesRows.clear();
    for (int i = saving ? 1 : 0; i < Game::kSaveSlots; ++i)   // в автослот вручную не пишем
        savesRows.push_back(i);
    for (int i = 0; i < Game::kSaveSlots; ++i)
        slotCache[i] = game.getSlotSummary(i);

    savesSelected = 0;
    if (!saving)                                               // при загрузке сразу на самом свежем сохранении
    {
        int64_t latest = -1;
        for (int r = 0; r < static_cast<int>(savesRows.size()); ++r)
        {
            const auto& s = slotCache[savesRows[r]];
            if (s && !s->corrupted && s->savedAt > latest) { latest = s->savedAt; savesSelected = r; }
        }
    }
    dialogSlot = -1;
    savesNotice.clear();
}

void UIManager::showSavesNotice(const std::string& text)
{
    savesNotice = text;
    savesNoticeTicks = SDL_GetTicks();
}

void UIManager::activateSlot(int slot)
{
    const auto& s = slotCache[slot];
    if (savesSaving)
    {
        if (s)                                                 // занятый слот: сначала спросим
        {
            dialogSlot = slot;
            dialogYes = false;
            dialogLine1 = trf("Overwrite {0}?", {slotName(slot)});
            dialogLine2 = tr(s->corrupted ? "It is damaged anyway" : "The old save will be lost");
            return;
        }
        dialogSlot = slot;                                     // пустой слот: пишем сразу, без вопроса
        answerDialog(true);
        return;
    }

    if (!s)            { showSavesNotice(trf("{0} is empty", {slotName(slot)})); return; }
    if (s->corrupted)  { showSavesNotice(trf("{0} is damaged", {slotName(slot)})); return; }
    if (game.isGameInProgress())
    {
        dialogSlot = slot;
        dialogYes = false;
        dialogLine1 = trf("Load {0}?", {slotName(slot)});
        dialogLine2 = tr("Unsaved progress will be lost");
        return;
    }
    dialogSlot = slot;
    answerDialog(true);
}

void UIManager::answerDialog(bool yes)
{
    const int slot = dialogSlot;
    dialogSlot = -1;
    if (!yes || slot < 0) return;

    if (savesSaving)
    {
        const bool ok = game.saveToSlot(slot);
        slotCache[slot] = game.getSlotSummary(slot);
        showSavesNotice(ok ? trf("Saved to {0}", {slotName(slot)}) : tr("Could not save"));
        return;
    }

    if (!game.loadFromSlot(slot))                              // успех сам переводит игру на карту
    {
        slotCache[slot] = game.getSlotSummary(slot);
        showSavesNotice(trf("Could not load {0}", {slotName(slot)}));
    }
}

SDL_Rect UIManager::slotCardRect(int row) const
{
    constexpr int top = 80, bottom = 16, gap = 6;
    const int n = std::max<int>(1, static_cast<int>(savesRows.size()));
    const int h = std::min(110, (screen.h - top - bottom - gap * (n - 1)) / n);
    return {40, top + row * (h + gap), screen.w - 80, h};
}

SDL_Rect UIManager::dialogOptionRect(bool yes) const
{
    const SDL_Rect win{screen.w / 2 - 200, screen.h / 2 - 75, 400, 150};
    return {win.x + (yes ? 90 : 250), win.y + 100, 70, 30};
}

void UIManager::handleSavesEvent(const SDL_Event& e)
{
    const bool keyDown = e.type == SDL_KEYDOWN;
    const SDL_Scancode k = keyDown ? e.key.keysym.scancode : SDL_SCANCODE_UNKNOWN;
    const bool fresh = keyDown && e.key.repeat == 0;           // Enter и Esc не повторяются при удержании
    const bool confirm = fresh && (k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_KP_ENTER || k == SDL_SCANCODE_SPACE);
    const bool back = (fresh && k == SDL_SCANCODE_ESCAPE) ||
                      (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT);
    const SDL_Point mouse = e.type == SDL_MOUSEMOTION ? SDL_Point{e.motion.x, e.motion.y}
                                                      : SDL_Point{e.button.x, e.button.y};
    const bool click = e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;

    if (dialogSlot >= 0)                                       // вопрос «точно?» перехватывает всё
    {
        if (back || (fresh && k == SDL_SCANCODE_N)) { dialogSlot = -1; return; }
        if (fresh && k == SDL_SCANCODE_Y)           { answerDialog(true); return; }
        if (keyDown && (k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_RIGHT ||
                        k == SDL_SCANCODE_A || k == SDL_SCANCODE_D))
            dialogYes = !dialogYes;
        if (confirm) { answerDialog(dialogYes); return; }
        if (e.type == SDL_MOUSEMOTION || click)
            for (const bool yes : {true, false})
            {
                const SDL_Rect r = dialogOptionRect(yes);
                if (!SDL_PointInRect(&mouse, &r)) continue;
                dialogYes = yes;
                if (click) answerDialog(yes);
                return;
            }
        return;
    }

    if (back) { game.closeSaveScreen(); return; }

    const int n = static_cast<int>(savesRows.size());
    if (n == 0) return;
    if (keyDown && (k == SDL_SCANCODE_UP || k == SDL_SCANCODE_W))   savesSelected = (savesSelected - 1 + n) % n;
    if (keyDown && (k == SDL_SCANCODE_DOWN || k == SDL_SCANCODE_S)) savesSelected = (savesSelected + 1) % n;
    if (confirm) { activateSlot(savesRows[savesSelected]); return; }

    if (e.type == SDL_MOUSEMOTION || click)
        for (int r = 0; r < n; ++r)
        {
            const SDL_Rect card = slotCardRect(r);
            if (!SDL_PointInRect(&mouse, &card)) continue;
            savesSelected = r;
            if (click) activateSlot(savesRows[r]);
            return;
        }
}

int UIManager::drawText(const std::string& text, int x, int y, SDL_Color color) const
{
    if (text.empty()) return 0;
    SDL_Texture* t = ResourceManager::getTextTexture(text, ResourceManager::getFont(kUiFont), color);
    if (!t) return 0;
    int w = 0, h = 0;
    SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
    visualizer.drawTexture(t, x, y, w, h);
    return w;
}

void UIManager::drawSavesScreen()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);

    // Заголовок; справа подсказка или сообщение о результате
    const SDL_Rect title{40, 16, screen.w - 80, 52};
    visualizer.drawNinePatch(skin, title, kWindowSkinSlice, 1);
    drawText(tr(savesSaving ? "Save game" : "Load game"), title.x + 20, title.y + 14, kSaveText);
    const bool noticeOn = !savesNotice.empty() && SDL_GetTicks() - savesNoticeTicks < 2500;
    const std::string hint = noticeOn ? savesNotice : tr("Enter - select   Esc - back");
    const TextSize hs = ResourceManager::measureText(hint, font);
    drawText(hint, title.x + title.w - 20 - hs.w, title.y + 14, noticeOn ? kSaveHot : kSaveDim);

    for (int r = 0; r < static_cast<int>(savesRows.size()); ++r)
    {
        const int slot = savesRows[r];
        const SDL_Rect card = slotCardRect(r);
        const bool selected = r == savesSelected;
        visualizer.drawNinePatch(skin, card, kWindowSkinSlice, 1);
        if (selected)
            drawPointer(card.x - 2 - (SDL_GetTicks() / 220) % 2 * 3, card.y + card.h / 2, PIXEL_SCALE);

        const int line1 = card.y + 10, line2 = card.y + card.h / 2 - 12, line3 = card.y + card.h - 34;
        drawText(slotName(slot), card.x + 20, line1, selected ? kSaveHot : kSaveText);

        const auto& s = slotCache[slot];
        if (!s)            { drawText(tr("Empty"), card.x + 260, line2, kSaveDim); continue; }
        if (s->corrupted)  { drawText(tr("Damaged save"), card.x + 260, line2, kSaveBad); continue; }

        drawText(tr(s->location), card.x + 20, line3, kSaveDim);

        // Отряд: спрайт в масштабе 2 и уровень под ним
        for (int i = 0; i < static_cast<int>(s->heroes.size()) && i < 4; ++i)
        {
            const auto& h = s->heroes[i];
            const int cx = card.x + 290 + i * 72;
            if (SDL_Texture* tex = ResourceManager::getTexture("Image" + std::to_string(heroLook(h.cls))))
                visualizer.drawTexture(tex, cx - 24, card.y + 6, 48, 48);
            const std::string lv = tr("Lv") + " " + std::to_string(h.level);
            drawText(lv, cx - ResourceManager::measureText(lv, font).w / 2, card.y + 58, kSaveText);
        }

        const int right = card.x + card.w - 210;
        drawText(tr("Time") + " " + formatPlayTime(s->playSeconds), right, line1, kSaveText);
        drawText(formatDate(s->savedAt), right, line2, kSaveDim);
        drawText(trf("{0} Gil", {std::to_string(s->gold)}), right, line3, kSaveText);
    }

    if (dialogSlot < 0) return;

    // Вопрос поверх затемнённого экрана
    visualizer.fillRect(screen, {0, 0, 0, 140});

    const SDL_Rect win{screen.w / 2 - 200, screen.h / 2 - 75, 400, 150};
    visualizer.drawNinePatch(skin, win, kWindowSkinSlice, 1);
    const int w1 = ResourceManager::measureText(dialogLine1, font).w;
    const int w2 = ResourceManager::measureText(dialogLine2, font).w;
    drawText(dialogLine1, win.x + (win.w - w1) / 2, win.y + 18, kSaveText);
    drawText(dialogLine2, win.x + (win.w - w2) / 2, win.y + 50, kSaveDim);
    for (const bool yes : {true, false})
    {
        const SDL_Rect r = dialogOptionRect(yes);
        drawText(tr(yes ? "Yes" : "No"), r.x + 10, r.y, yes == dialogYes ? kSaveHot : kSaveText);
        if (yes == dialogYes) drawPointer(r.x + 4, r.y + 12, 2);
    }
}

void UIManager::updateMenuButtons()
{
    // Недоступные пункты серые, а не скрытые: меню не прыгает
    TTF_Font *font = ResourceManager::getFont(kUiFont);
    const SDL_Color on{0, 0, 0, 0}, off{120, 120, 120, 255};
    auto set = [&](const std::string& id, const std::string& text, bool enabled) {
        setTexture(id, "Menu", ResourceManager::getTextTexture(text, font, enabled ? on : off));
        setEnabled(id, "Menu", enabled);
    };
    set("Continue", tr("Continue"), game.canContinue());
    set("Load", tr("Load"), game.canLoad());
}

// ─────────────────────────────────────────
// Игровое меню (Esc на карте)
// ─────────────────────────────────────────

namespace {
    const SDL_Color kGood{120, 230, 140, 255};                  // стат вырастет
    const SDL_Color kBad {255, 120, 120, 255};                  // стат упадёт

    bool keyIs(const SDL_Event& e, std::initializer_list<SDL_Scancode> keys)
    {
        if (e.type != SDL_KEYDOWN) return false;
        for (SDL_Scancode k : keys) if (e.key.keysym.scancode == k) return true;
        return false;
    }
    bool confirmKey(const SDL_Event& e)
    {
        return e.type == SDL_KEYDOWN && e.key.repeat == 0 &&
               keyIs(e, {SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_SPACE});
    }
    bool backKey(const SDL_Event& e)
    {
        return (e.type == SDL_KEYDOWN && e.key.repeat == 0 && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) ||
               (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT);
    }
    bool leftClick(const SDL_Event& e) { return e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT; }
    SDL_Point mousePos(const SDL_Event& e)
    {
        return e.type == SDL_MOUSEMOTION ? SDL_Point{e.motion.x, e.motion.y} : SDL_Point{e.button.x, e.button.y};
    }
    bool isMouse(const SDL_Event& e) { return e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN; }

    std::string signedValue(int v) { return (v >= 0 ? "+" : "") + std::to_string(v); }

    std::string elementList(const std::vector<Elemental>& list)
    {
        std::string s;
        for (Elemental e : list) s += (s.empty() ? "" : ", ") + tr(elementName(e));
        return s.empty() ? "-" : s;
    }

    // Общие окна страниц
    constexpr SDL_Rect kHeader{24, 24, 852, 52};
}

void UIManager::openGameMenu()
{
    closeBestiary();
    gameMenuSelected = 0;
    menuPage = MenuPage::Main;
    menuNotice.clear();
}

void UIManager::openMenuPage(MenuPage page)
{
    menuPage = page;
    listSel = listScroll = 0;
    if (page == MenuPage::Equip) equipSlotSel = 0;
}

void UIManager::showMenuNotice(const std::string& text)
{
    menuNotice = text;
    menuNoticeTicks = SDL_GetTicks();
}

std::vector<UIManager::MenuCommand> UIManager::gameMenuCommands()
{
    auto pickHero = [this](MenuPage page) {
        return [this, page]() { pickFor = page; partySel = 0; menuPage = MenuPage::PickHero; };
    };
    return {
        {tr("Items"),           true,           [this]() { openMenuPage(MenuPage::Items); }},
        {tr("Magic"),           true,           pickHero(MenuPage::Magic)},
        {tr("Equipment"),       true,           pickHero(MenuPage::Equip)},
        {tr("Status"),          true,           pickHero(MenuPage::Status)},
        {tr("Bestiary"),        true,           [this]() { openBestiary(); }},
        {tr("Save"),            game.canSave(), [this]() { game.openSaveScreen(true); }},
        {tr("Load"),            game.canLoad(), [this]() { game.openSaveScreen(false); }},
        {tr("Config"),          true,           [this]() { game.openOptions(GameState::GameMenu); }},
        {tr("Return to Title"), true,           [this]() { game.returnToTitle(); }},
    };
}

SDL_Rect UIManager::gameMenuRowRect(int row) const
{
    return {24 + 16, 24 + 14 + row * 40, 220 - 32, 36};
}

SDL_Rect UIManager::partyRowRect(int i) const
{
    const SDL_Rect party{264, 24, screen.w - 264 - 24, screen.h - 48};
    const int rowH = (party.h - 24) / 4;
    return {party.x + 8, party.y + 12 + i * rowH, party.w - 16, rowH};
}

SDL_Rect UIManager::targetRowRect(int i) const
{
    return {492, 98 + i * 90, 368, 86};                       // окно выбора цели справа: {476, 84, 400, 380}
}

// ── Список в две колонки ────────────────────────────

SDL_Rect UIManager::listCellRect(const ListLayout& l, int scroll, int index)
{
    const int visible = index - scroll * l.cols;
    const int row = visible / l.cols, col = visible % l.cols;
    const int w = l.area.w / l.cols;
    return {l.area.x + col * w, l.area.y + row * l.rowH, w - 24, l.rowH};
}

void UIManager::listNavigate(const ListLayout& l, int& sel, int& scroll, int count, SDL_Scancode k)
{
    if (count <= 0) { sel = scroll = 0; return; }
    switch (k)
    {
        case SDL_SCANCODE_UP:    case SDL_SCANCODE_W: sel -= l.cols; break;
        case SDL_SCANCODE_DOWN:  case SDL_SCANCODE_S: sel += l.cols; break;
        case SDL_SCANCODE_LEFT:  case SDL_SCANCODE_A: sel -= 1; break;
        case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D: sel += 1; break;
        default: break;
    }
    sel = std::clamp(sel, 0, count - 1);
    const int row = sel / l.cols;                                  // прокрутка за курсором
    if (row < scroll) scroll = row;
    if (row >= scroll + l.rows) scroll = row - l.rows + 1;
}

int UIManager::listHit(const ListLayout& l, int scroll, int count, SDL_Point p)
{
    for (int i = scroll * l.cols; i < count && i < (scroll + l.rows) * l.cols; ++i)
    {
        const SDL_Rect r = listCellRect(l, scroll, i);
        if (SDL_PointInRect(&p, &r)) return i;
    }
    return -1;
}

// ── Данные для страниц ──────────────────────────────

std::vector<std::pair<int, int>> UIManager::inventoryList() const
{
    std::vector<std::pair<int, int>> out;
    for (const auto& [id, count] : player.getItemCounts())        // map по id: сначала расходники, потом экипировка
        if (count > 0) out.emplace_back(id, count);
    return out;
}

std::vector<int> UIManager::equipCandidates() const
{
    std::vector<int> out{0};                                       // первая строка — «снять»
    const auto& heroes = player.getHeroes();
    if (menuHero >= static_cast<int>(heroes.size())) return out;
    const auto slot = static_cast<EquipSlot>(equipSlotSel);
    for (const auto& [id, count] : player.getItemCounts())
    {
        const Item& item = game.getItem(id);
        if (count > 0 && item.slot == slot && item.canEquip(heroes[menuHero].getClass()))
            out.push_back(id);
    }
    return out;
}

std::string UIManager::itemDescription(const Item& item) const
{
    if (item.isEquipment())
    {
        std::string s = tr(equipSlotName(item.slot));
        if (item.attack)  s += "   " + tr("ATK") + " " + signedValue(item.attack);
        if (item.defence) s += "   " + tr("DEF") + " " + signedValue(item.defence);
        if (item.agility) s += "   " + tr("AGI") + " " + signedValue(item.agility);
        std::string who;
        for (auto c : item.classes)
            who += (who.empty() ? "" : ", ") + tr(specializationNamesString.at(c));
        return s + "   " + (who.empty() ? tr("Anyone") : who);
    }
    const std::string power = std::to_string(item.power);
    switch (item.effectType)
    {
        case ItemEffectType::Heal:        return trf("Restores {0} HP to one ally", {power});
        case ItemEffectType::RestoreMana: return trf("Restores {0} MP to one ally", {power});
        case ItemEffectType::Damage:      return trf("Deals {0} damage to an enemy. Battle only", {power});
        case ItemEffectType::Buff:        return tr("Strengthens an ally. Battle only");
        case ItemEffectType::Debuff:      return tr("Weakens an enemy. Battle only");
        default:                          return "";
    }
}

std::string UIManager::skillDescription(const Skill& skill) const
{
    const std::string who = tr(skill.targetType == TargetType::AllAllies  ? "all allies"
                             : skill.targetType == TargetType::AllEnemies ? "all enemies"
                             : skill.targetType == TargetType::Self       ? "self"
                             : skill.targetType == TargetType::Ally       ? "one ally" : "one enemy");
    const std::string power = std::to_string(skill.power);
    switch (skill.effectType)
    {
        case SkillEffectType::Heal:   return trf("Restores {0} HP. Target: {1}", {power, who});
        case SkillEffectType::Damage: return trf("Deals {0} damage. Target: {1}. Battle only", {power, who});
        case SkillEffectType::Buff:   return trf("Strengthens. Target: {0}. Battle only", {who});
        case SkillEffectType::Debuff: return trf("Weakens. Target: {0}. Battle only", {who});
        default:                      return "";
    }
}

bool UIManager::skillUsableInField(const Hero& caster, const Skill& skill) const
{
    return skill.effectType == SkillEffectType::Heal && caster.getCurrentHp() > 0 &&
           caster.getCurrentMp() >= skill.manaCost && skill.targetType != TargetType::Enemy &&
           skill.targetType != TargetType::AllEnemies;
}

bool UIManager::applyFieldEffect(Hero& target, bool restoresMp, int power)
{
    if (target.getCurrentHp() == 0) return false;                 // зелья и Heal не воскрешают, как в FF1
    if (restoresMp)
    {
        if (target.getCurrentMp() >= target.getMaxMp()) return false;
        target.restoreMana(static_cast<uint32_t>(power));
        return true;
    }
    if (target.getCurrentHp() >= target.getMaxHp()) return false;
    target.heal(static_cast<uint32_t>(power));
    return true;
}

void UIManager::useItemOn(int heroIndex)
{
    const auto list = inventoryList();
    if (listSel >= static_cast<int>(list.size())) { menuPage = MenuPage::Items; return; }
    const Item& item = game.getItem(list[listSel].first);
    auto& heroes = player.getLinkTOHeroes();
    if (heroIndex >= static_cast<int>(heroes.size())) return;

    if (!applyFieldEffect(heroes[heroIndex], item.effectType == ItemEffectType::RestoreMana, item.power))
    {
        showMenuNotice(tr("No effect"));
        return;
    }
    player.consumeItem(item.id);
    showMenuNotice(trf("{0} used on {1}", {item.name, heroes[heroIndex].getName()}));
    if (!player.hasItem(item.id))                                  // кончились: назад к списку
    {
        menuPage = MenuPage::Items;
        listSel = std::max(0, std::min(listSel, static_cast<int>(inventoryList().size()) - 1));
    }
}

void UIManager::castOn(int heroIndex)
{
    auto& heroes = player.getLinkTOHeroes();
    if (menuHero >= static_cast<int>(heroes.size())) return;
    Hero& caster = heroes[menuHero];
    const auto skills = caster.getAvailableSkills();
    if (listSel >= static_cast<int>(skills.size())) return;
    const Skill& skill = game.getSkill(skills[listSel].skillId);
    if (!skillUsableInField(caster, skill)) { showMenuNotice(tr("Not enough MP")); menuPage = MenuPage::Magic; return; }

    // Self и AllAllies не спрашивают цель: heroIndex тогда -1
    bool any = false;
    if (skill.targetType == TargetType::AllAllies)
        for (auto& h : heroes) any |= applyFieldEffect(h, false, skill.power);
    else
        any = applyFieldEffect(heroes[skill.targetType == TargetType::Self ? menuHero : heroIndex], false, skill.power);

    if (!any) { showMenuNotice(tr("No effect")); return; }
    caster.spendMana(skill.manaCost);
    showMenuNotice(trf("{0} cast {1}", {caster.getName(), skill.name}));
    if (!skillUsableInField(caster, skill)) menuPage = MenuPage::Magic;
}

void UIManager::cycleMenuHero(int dir)
{
    const int n = static_cast<int>(player.getHeroes().size());
    if (n == 0) return;
    menuHero = (menuHero + dir + n) % n;
    listSel = listScroll = 0;
    if (menuPage == MenuPage::EquipPick) menuPage = MenuPage::Equip;
}

// ── Ввод ─────────────────────────────────────────────

void UIManager::handleGameMenuEvent(const SDL_Event& e)
{
    if (bestiaryOpen) { handleBestiaryEvent(e); return; }       // Esc в книге возвращает в меню

    // Q/E листают героя на страницах одного героя, как LB/RB в ремастере
    const bool heroPage = menuPage == MenuPage::Magic || menuPage == MenuPage::Equip ||
                          menuPage == MenuPage::EquipPick || menuPage == MenuPage::Status;
    if (heroPage && e.type == SDL_KEYDOWN && e.key.repeat == 0)
    {
        if (e.key.keysym.scancode == SDL_SCANCODE_Q) { cycleMenuHero(-1); return; }
        if (e.key.keysym.scancode == SDL_SCANCODE_E) { cycleMenuHero(+1); return; }
    }

    switch (menuPage)
    {
        case MenuPage::Main:
        case MenuPage::PickHero:    handleMenuMain(e); break;
        case MenuPage::Items:
        case MenuPage::Magic:       handleMenuList(e); break;
        case MenuPage::ItemTarget:
        case MenuPage::MagicTarget: handleMenuTarget(e); break;
        case MenuPage::Equip:
        case MenuPage::EquipPick:   handleMenuEquip(e); break;
        case MenuPage::Status:
            if (backKey(e) || confirmKey(e)) menuPage = MenuPage::Main;
            break;
    }
}

void UIManager::handleMenuMain(const SDL_Event& e)
{
    const int heroes = static_cast<int>(player.getHeroes().size());

    if (menuPage == MenuPage::PickHero)                            // курсор на отряде
    {
        if (backKey(e)) { menuPage = MenuPage::Main; return; }
        if (heroes == 0) return;
        if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))   partySel = (partySel - 1 + heroes) % heroes;
        if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S})) partySel = (partySel + 1) % heroes;
        bool go = confirmKey(e);
        if (isMouse(e))
        {
            const SDL_Point m = mousePos(e);
            for (int i = 0; i < heroes; ++i)
            {
                const SDL_Rect r = partyRowRect(i);
                if (!SDL_PointInRect(&m, &r)) continue;
                partySel = i;
                go = go || leftClick(e);
            }
        }
        if (go)
        {
            menuHero = partySel;
            openMenuPage(pickFor);
        }
        return;
    }

    if (backKey(e)) { game.closeGameMenu(); return; }

    const auto commands = gameMenuCommands();
    const int n = static_cast<int>(commands.size());
    if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))   gameMenuSelected = (gameMenuSelected - 1 + n) % n;
    if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S})) gameMenuSelected = (gameMenuSelected + 1) % n;

    int activate = confirmKey(e) ? gameMenuSelected : -1;
    if (isMouse(e))
    {
        const SDL_Point m = mousePos(e);
        for (int r = 0; r < n; ++r)
        {
            const SDL_Rect rr = gameMenuRowRect(r);
            if (!SDL_PointInRect(&m, &rr)) continue;
            gameMenuSelected = r;
            if (leftClick(e)) activate = r;
        }
    }
    if (activate >= 0 && commands[activate].enabled)
        commands[activate].action();
}

namespace {
    // Items и Magic: список во всю ширину под заголовком, описание внизу
    constexpr MenuListLayout kItemList{{44, 98, 812, 352}, 2, 8, 44};
}

void UIManager::handleMenuList(const SDL_Event& e)
{
    const bool items = menuPage == MenuPage::Items;
    if (backKey(e)) { menuPage = MenuPage::Main; return; }

    int count = 0;
    if (items) count = static_cast<int>(inventoryList().size());
    else if (menuHero < static_cast<int>(player.getHeroes().size()))
        count = static_cast<int>(player.getHeroes()[menuHero].getAvailableSkills().size());

    if (e.type == SDL_KEYDOWN) listNavigate(kItemList, listSel, listScroll, count, e.key.keysym.scancode);
    if (e.type == SDL_MOUSEWHEEL)
        listScroll = std::clamp(listScroll - e.wheel.y, 0, std::max(0, (count + 1) / 2 - kItemList.rows));

    bool go = confirmKey(e);
    if (isMouse(e))
    {
        const int hit = listHit(kItemList, listScroll, count, mousePos(e));
        if (hit >= 0) { listSel = hit; go = go || leftClick(e); }
    }
    if (!go || listSel >= count) return;

    if (items)
    {
        const Item& item = game.getItem(inventoryList()[listSel].first);
        if (!item.usableInField())
        {
            showMenuNotice(tr(item.isEquipment() ? "Equip it from the Equipment menu" : "Can only be used in battle"));
            return;
        }
        partySel = 0;
        menuPage = MenuPage::ItemTarget;
        return;
    }

    const Hero& caster = player.getHeroes()[menuHero];
    const Skill& skill = game.getSkill(caster.getAvailableSkills()[listSel].skillId);
    if (!skillUsableInField(caster, skill))
    {
        showMenuNotice(skill.effectType != SkillEffectType::Heal ? tr("Can only be used in battle")
                       : caster.getCurrentHp() == 0 ? trf("{0} is down", {caster.getName()}) : tr("Not enough MP"));
        return;
    }
    if (skill.targetType == TargetType::Ally) { partySel = menuHero; menuPage = MenuPage::MagicTarget; }
    else castOn(-1);                                                // на себя или на всех: сразу
}

void UIManager::handleMenuTarget(const SDL_Event& e)
{
    const bool item = menuPage == MenuPage::ItemTarget;
    if (backKey(e)) { menuPage = item ? MenuPage::Items : MenuPage::Magic; return; }
    const int n = static_cast<int>(player.getHeroes().size());
    if (n == 0) return;
    if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))   partySel = (partySel - 1 + n) % n;
    if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S})) partySel = (partySel + 1) % n;

    bool go = confirmKey(e);
    if (isMouse(e))
    {
        const SDL_Point m = mousePos(e);
        for (int i = 0; i < n; ++i)
        {
            const SDL_Rect r = targetRowRect(i);
            if (!SDL_PointInRect(&m, &r)) continue;
            partySel = i;
            go = go || leftClick(e);
        }
    }
    if (!go) return;
    if (item) useItemOn(partySel);
    else      castOn(partySel);
}

namespace {
    // Equipment: слоты слева, статы справа, подходящие вещи внизу
    constexpr SDL_Rect kSlotsWin{24, 84, 420, 236};
    constexpr SDL_Rect kStatsWin{456, 84, 420, 236};
    constexpr SDL_Rect kCandWin {24, 328, 852, 248};
    constexpr MenuListLayout kCandList{{44, 342, 812, 220}, 2, 5, 44};
    SDL_Rect slotRowRect(int i) { return {kSlotsWin.x + 16, kSlotsWin.y + 14 + i * 42, kSlotsWin.w - 32, 40}; }
}

void UIManager::handleMenuEquip(const SDL_Event& e)
{
    auto& heroes = player.getLinkTOHeroes();
    if (menuHero >= static_cast<int>(heroes.size())) { menuPage = MenuPage::Main; return; }

    if (menuPage == MenuPage::Equip)                               // выбираем слот
    {
        if (backKey(e)) { menuPage = MenuPage::Main; return; }
        if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))   equipSlotSel = (equipSlotSel - 1 + kEquipSlots) % kEquipSlots;
        if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S})) equipSlotSel = (equipSlotSel + 1) % kEquipSlots;
        bool go = confirmKey(e);
        if (isMouse(e))
        {
            const SDL_Point m = mousePos(e);
            for (int i = 0; i < kEquipSlots; ++i)
            {
                const SDL_Rect r = slotRowRect(i);
                if (!SDL_PointInRect(&m, &r)) continue;
                equipSlotSel = i;
                go = go || leftClick(e);
            }
        }
        if (go) { listSel = listScroll = 0; menuPage = MenuPage::EquipPick; }
        return;
    }

    // Выбираем вещь для слота
    if (backKey(e)) { menuPage = MenuPage::Equip; return; }
    const auto cands = equipCandidates();
    const int count = static_cast<int>(cands.size());
    if (e.type == SDL_KEYDOWN) listNavigate(kCandList, listSel, listScroll, count, e.key.keysym.scancode);
    bool go = confirmKey(e);
    if (isMouse(e))
    {
        const int hit = listHit(kCandList, listScroll, count, mousePos(e));
        if (hit >= 0) { listSel = hit; go = go || leftClick(e); }
    }
    if (!go || listSel >= count) return;

    const int newId = cands[listSel];
    const auto slot = static_cast<EquipSlot>(equipSlotSel);
    if (newId != 0 && !player.consumeItem(newId)) return;          // вещь берём из сумки
    const int old = heroes[menuHero].equip(slot, newId);
    if (old != 0) player.addItem(old, 1);                          // снятое возвращается в сумку
    game.sendPartyInfo();                                          // напарник и хост видят новую экипировку
    menuPage = MenuPage::Equip;
}

// ── Отрисовка ────────────────────────────────────────

void UIManager::drawMenuHeader(const std::string& title, const std::string& right)
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), kHeader, kWindowSkinSlice, 1);
    drawText(title, kHeader.x + 20, kHeader.y + 14, kSaveText);
    const bool noticeOn = !menuNotice.empty() && SDL_GetTicks() - menuNoticeTicks < 2000;
    const std::string text = noticeOn ? menuNotice : right;
    drawText(text, kHeader.x + kHeader.w - 20 - ResourceManager::measureText(text, font).w, kHeader.y + 14,
             noticeOn ? kSaveHot : kSaveDim);
}

void UIManager::drawPartyPanel(const SDL_Rect& party, int pointerRow, bool compact)
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), party, kWindowSkinSlice, 1);
    const auto& heroes = player.getHeroes();
    for (int i = 0; i < static_cast<int>(heroes.size()) && i < 4; ++i)
    {
        const Hero& h = heroes[i];
        const SDL_Rect row = compact ? targetRowRect(i) : partyRowRect(i);
        const bool dead = h.getCurrentHp() == 0;
        if (i == pointerRow)
            drawPointer(row.x + 6 - (SDL_GetTicks() / 220) % 2 * 3, row.y + row.h / 2, PIXEL_SCALE);

        const int scale = compact ? 2 : 3;
        const std::string img = "Image" + std::to_string(heroLook(h.getClass())) + (dead ? "Dead" : "");
        if (SDL_Texture* tex = ResourceManager::getTexture(img))
            visualizer.drawTexture(tex, row.x + 16, row.y + (row.h - kHeroFrame * scale) / 2, kHeroFrame * scale, kHeroFrame * scale);

        const int tx = row.x + 16 + kHeroFrame * scale + 12;
        drawText(h.getName(), tx, row.y + (compact ? 8 : 14), dead ? kSaveBad : kSaveText);
        if (!compact)
        {
            const auto cls = specializationNamesString.find(h.getClass());
            drawText(cls != specializationNamesString.end() ? tr(cls->second) : "", tx, row.y + 46, kSaveDim);
        }
        drawText(tr("Lv") + " " + std::to_string(h.getLevel()), tx, row.y + (compact ? 44 : 78), kSaveText);

        // HP и MP: число и полоска
        const int bw = compact ? 170 : 230;
        const int bx = row.x + row.w - 16 - bw;
        auto bar = [&](int y, const char* label, uint32_t cur, uint32_t max, SDL_Color fill) {
            drawText(label, bx, y, kSaveDim);
            const std::string v = std::to_string(cur) + "/" + std::to_string(max);
            drawText(v, bx + bw - ResourceManager::measureText(v, font).w, y, kSaveText);
            const SDL_Rect back{bx, y + 28, bw, compact ? 5 : 8};
            visualizer.fillRect(back, {10, 16, 60, 255});
            if (max > 0)
                visualizer.fillRect({back.x, back.y, static_cast<int>(back.w * static_cast<uint64_t>(cur) / max), back.h}, fill);
        };
        bar(row.y + (compact ? 4 : 14), "HP", h.getCurrentHp(), h.getMaxHp(), {96, 220, 120, 255});
        bar(row.y + (compact ? 44 : 66), "MP", h.getCurrentMp(), h.getMaxMp(), {96, 160, 255, 255});
    }
}

void UIManager::drawGameMenu()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    visualizer.fillRect(screen, {0, 0, 0, 120});                  // карта видна, но приглушена
    if (bestiaryOpen) { drawBestiary(); return; }

    switch (menuPage)
    {
        case MenuPage::Items:       drawItemsPage(); return;
        case MenuPage::ItemTarget:  drawItemsPage(); drawTargetOverlay(); return;
        case MenuPage::Magic:       drawMagicPage(); return;
        case MenuPage::MagicTarget: drawMagicPage(); drawTargetOverlay(); return;
        case MenuPage::Equip:
        case MenuPage::EquipPick:   drawEquipPage(); return;
        case MenuPage::Status:      drawStatusPage(); return;
        default: break;
    }

    // Главная страница: команды
    const auto commands = gameMenuCommands();
    const int n = static_cast<int>(commands.size());
    visualizer.drawNinePatch(skin, {24, 24, 220, 28 + n * 40}, kWindowSkinSlice, 1);
    for (int r = 0; r < n; ++r)
    {
        const SDL_Rect rr = gameMenuRowRect(r);
        const bool selected = r == gameMenuSelected;
        const SDL_Color c = !commands[r].enabled ? kSaveDim : (selected ? kSaveHot : kSaveText);
        drawText(commands[r].text, rr.x + 22, rr.y + 6, c);
        if (selected && menuPage == MenuPage::Main)
            drawPointer(rr.x + 16 - (SDL_GetTicks() / 220) % 2 * 2, rr.y + rr.h / 2, 2);
    }

    // Золото, время, место
    const SDL_Rect info{24, screen.h - 24 - 132, 220, 132};
    visualizer.drawNinePatch(skin, info, kWindowSkinSlice, 1);
    auto row = [&](int y, const std::string& label, const std::string& value) {
        drawText(label, info.x + 16, y, kSaveDim);
        drawText(value, info.x + info.w - 16 - ResourceManager::measureText(value, font).w, y, kSaveText);
    };
    row(info.y + 14, tr("Gil"), std::to_string(player.getPlayerGold()));
    row(info.y + 50, tr("Time"), formatPlayTime(game.getPlaySeconds()));
    drawText(tr(game.currentLocation()), info.x + 16, info.y + 86, kSaveText);

    drawPartyPanel({264, 24, screen.w - 264 - 24, screen.h - 48},
                   menuPage == MenuPage::PickHero ? partySel : -1, false);
}

void UIManager::drawItemsPage()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    drawMenuHeader(tr("Items"), trf("{0} Gil", {std::to_string(player.getPlayerGold())}));

    visualizer.drawNinePatch(skin, {24, 84, 852, 380}, kWindowSkinSlice, 1);
    const auto list = inventoryList();
    if (list.empty()) drawText(tr("No items"), kItemList.area.x + 22, kItemList.area.y + 8, kSaveDim);
    const int last = std::min<int>(static_cast<int>(list.size()), (listScroll + kItemList.rows) * kItemList.cols);
    for (int i = listScroll * kItemList.cols; i < last; ++i)
    {
        const Item& item = game.getItem(list[i].first);
        const SDL_Rect cell = listCellRect(kItemList, listScroll, i);
        const bool sel = i == listSel;
        const SDL_Color c = item.usableInField() ? (sel ? kSaveHot : kSaveText) : kSaveDim;
        drawText(item.name, cell.x + 22, cell.y + 8, c);
        const std::string cnt = "x" + std::to_string(list[i].second);
        drawText(cnt, cell.x + cell.w - ResourceManager::measureText(cnt, font).w, cell.y + 8, c);
        if (sel && menuPage == MenuPage::Items)
            drawPointer(cell.x + 16 - (SDL_GetTicks() / 220) % 2 * 2, cell.y + cell.h / 2, 2);
    }

    visualizer.drawNinePatch(skin, {24, 472, 852, 104}, kWindowSkinSlice, 1);
    if (listSel < static_cast<int>(list.size()))
    {
        const Item& item = game.getItem(list[listSel].first);
        drawText(item.name, 44, 488, kSaveText);
        drawText(itemDescription(item), 44, 524, kSaveDim);
    }
}

void UIManager::drawMagicPage()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    const auto& heroes = player.getHeroes();
    if (menuHero >= static_cast<int>(heroes.size())) return;
    const Hero& hero = heroes[menuHero];
    drawMenuHeader(tr("Magic") + " - " + hero.getName(),
                   "MP " + std::to_string(hero.getCurrentMp()) + "/" + std::to_string(hero.getMaxMp()) + "   " + tr("Q/E hero"));

    visualizer.drawNinePatch(skin, {24, 84, 852, 380}, kWindowSkinSlice, 1);
    const auto skills = hero.getAvailableSkills();
    if (skills.empty()) drawText(tr("No spells"), kItemList.area.x + 22, kItemList.area.y + 8, kSaveDim);
    const int last = std::min<int>(static_cast<int>(skills.size()), (listScroll + kItemList.rows) * kItemList.cols);
    for (int i = listScroll * kItemList.cols; i < last; ++i)
    {
        const Skill& skill = game.getSkill(skills[i].skillId);
        const SDL_Rect cell = listCellRect(kItemList, listScroll, i);
        const bool sel = i == listSel;
        const SDL_Color c = skillUsableInField(hero, skill) ? (sel ? kSaveHot : kSaveText) : kSaveDim;
        drawText(skill.name, cell.x + 22, cell.y + 8, c);
        const std::string cost = std::to_string(skill.manaCost) + " MP";
        drawText(cost, cell.x + cell.w - ResourceManager::measureText(cost, font).w, cell.y + 8, c);
        if (sel && menuPage == MenuPage::Magic)
            drawPointer(cell.x + 16 - (SDL_GetTicks() / 220) % 2 * 2, cell.y + cell.h / 2, 2);
    }

    visualizer.drawNinePatch(skin, {24, 472, 852, 104}, kWindowSkinSlice, 1);
    if (listSel < static_cast<int>(skills.size()))
    {
        const Skill& skill = game.getSkill(skills[listSel].skillId);
        drawText(skill.name, 44, 488, kSaveText);
        drawText(skillDescription(skill), 44, 524, kSaveDim);
    }
}

void UIManager::drawTargetOverlay()
{
    drawPartyPanel({476, 84, 400, 380}, partySel, true);
}

void UIManager::drawEquipPage()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    const auto& heroes = player.getHeroes();
    if (menuHero >= static_cast<int>(heroes.size())) return;
    const Hero& hero = heroes[menuHero];
    drawMenuHeader(tr("Equipment") + " - " + hero.getName(), tr("Q/E hero   Esc back"));

    // Слоты
    visualizer.drawNinePatch(skin, kSlotsWin, kWindowSkinSlice, 1);
    for (int i = 0; i < kEquipSlots; ++i)
    {
        const SDL_Rect r = slotRowRect(i);
        const bool sel = i == equipSlotSel;
        drawText(tr(equipSlotName(static_cast<EquipSlot>(i))), r.x + 22, r.y + 6, kSaveDim);
        const int id = hero.getEquipped(static_cast<EquipSlot>(i));
        drawText(id ? game.getItem(id).name : "-", r.x + 130, r.y + 6, sel ? kSaveHot : kSaveText);
        if (sel)
            drawPointer(r.x + 16 - (menuPage == MenuPage::Equip ? (SDL_GetTicks() / 220) % 2 * 2 : 0), r.y + r.h / 2, 2);
    }

    // Статы: сейчас и, если выбираем вещь, что станет
    const auto cands = equipCandidates();
    Hero preview = hero;
    const bool previewing = menuPage == MenuPage::EquipPick && listSel < static_cast<int>(cands.size());
    if (previewing) preview.equip(static_cast<EquipSlot>(equipSlotSel), cands[listSel]);

    visualizer.drawNinePatch(skin, kStatsWin, kWindowSkinSlice, 1);
    const struct { const char* name; uint32_t now, then; } stats[] = {
        {"Attack",  hero.getAttackPower(nullptr), preview.getAttackPower(nullptr)},
        {"Defense", hero.getDefense(),            preview.getDefense()},
        {"Agility", hero.getSpeed(),              preview.getSpeed()},
    };
    for (int i = 0; i < 3; ++i)
    {
        const int y = kStatsWin.y + 28 + i * 60;
        const std::string statName = tr(stats[i].name);
        drawText(statName, kStatsWin.x + 24, y, kSaveDim);
        const std::string now = std::to_string(stats[i].now);
        drawText(now, kStatsWin.x + 220 - ResourceManager::measureText(now, font).w, y, kSaveText);
        if (!previewing) continue;
        const SDL_Color c = stats[i].then > stats[i].now ? kGood : stats[i].then < stats[i].now ? kBad : kSaveText;
        drawText(">", kStatsWin.x + 250, y, kSaveDim);
        const std::string then = std::to_string(stats[i].then);
        drawText(then, kStatsWin.x + 380 - ResourceManager::measureText(then, font).w, y, c);
    }

    // Что можно надеть в выбранный слот (при выборе слота — приглушённый предпросмотр)
    visualizer.drawNinePatch(skin, kCandWin, kWindowSkinSlice, 1);
    const int last = std::min<int>(static_cast<int>(cands.size()), (listScroll + kCandList.rows) * kCandList.cols);
    for (int i = listScroll * kCandList.cols; i < last; ++i)
    {
        const SDL_Rect cell = listCellRect(kCandList, listScroll, i);
        const bool sel = previewing && i == listSel;
        const SDL_Color c = menuPage != MenuPage::EquipPick ? kSaveDim : (sel ? kSaveHot : kSaveText);
        if (cands[i] == 0) { drawText(tr("Remove"), cell.x + 22, cell.y + 8, c); }
        else
        {
            const Item& item = game.getItem(cands[i]);
            drawText(item.name, cell.x + 22, cell.y + 8, c);
            const std::string cnt = "x" + std::to_string(player.getItemCounts().at(cands[i]));
            drawText(cnt, cell.x + cell.w - ResourceManager::measureText(cnt, font).w, cell.y + 8, c);
        }
        if (sel) drawPointer(cell.x + 16 - (SDL_GetTicks() / 220) % 2 * 2, cell.y + cell.h / 2, 2);
    }
    if (cands.size() == 1)
        drawText(tr("Nothing else fits this slot"), kCandList.area.x + 22, kCandList.area.y + kCandList.rowH + 8, kSaveDim);
}

void UIManager::drawStatusPage()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    const auto& heroes = player.getHeroes();
    if (menuHero >= static_cast<int>(heroes.size())) return;
    const Hero& h = heroes[menuHero];
    drawMenuHeader(tr("Status"), tr("Q/E hero   Esc back"));

    const SDL_Rect win{24, 84, 852, 492};
    visualizer.drawNinePatch(skin, win, kWindowSkinSlice, 1);

    const bool dead = h.getCurrentHp() == 0;
    const std::string img = "Image" + std::to_string(heroLook(h.getClass())) + (dead ? "Dead" : "");
    if (SDL_Texture* tex = ResourceManager::getTexture(img))
        visualizer.drawTexture(tex, win.x + 24, win.y + 16, 96, 96);
    drawText(h.getName(), win.x + 130, win.y + 30, dead ? kSaveBad : kSaveText);
    const auto cls = specializationNamesString.find(h.getClass());
    drawText((cls != specializationNamesString.end() ? tr(cls->second) : "") + "   " + tr("Lv") + " " + std::to_string(h.getLevel()),
             win.x + 130, win.y + 64, kSaveDim);

    auto line = [&](int x, int y, int right, const std::string& label, const std::string& value, SDL_Color c) {
        drawText(label, x, y, kSaveDim);
        drawText(value, right - ResourceManager::measureText(value, font).w, y, c);
    };
    const int top = win.y + 140;
    line(win.x + 40, top,       win.x + 380, "HP",  std::to_string(h.getCurrentHp()) + "/" + std::to_string(h.getMaxHp()), kSaveText);
    line(win.x + 40, top + 34,  win.x + 380, "MP",  std::to_string(h.getCurrentMp()) + "/" + std::to_string(h.getMaxMp()), kSaveText);
    line(win.x + 40, top + 68,  win.x + 380, tr("EXP"), std::to_string(h.getExperience()), kSaveText);
    line(win.x + 40, top + 102, win.x + 380, tr("Next level"),
         std::to_string(h.getXpToNextLevel() > h.getExperience() ? h.getXpToNextLevel() - h.getExperience() : 0), kSaveText);

    // Итог и вклад экипировки в скобках
    auto stat = [&](int y, const char* name, uint32_t total, uint32_t base) {
        const int bonus = static_cast<int>(total) - static_cast<int>(base);
        line(win.x + 450, y, win.x + 700, tr(name), std::to_string(total), kSaveText);
        if (bonus != 0) drawText("(" + signedValue(bonus) + ")", win.x + 716, y, bonus > 0 ? kGood : kBad);
    };
    stat(top,      "Attack",  h.getAttackPower(nullptr), h.getBaseAttack());
    stat(top + 34, "Defense", h.getDefense(),            h.getBaseDefense());
    stat(top + 68, "Agility", h.getSpeed(),              h.getBaseSpeed());

    // Экипировка и стихии
    const int eq = win.y + 300;
    drawText(tr("Equipment"), win.x + 40, eq, kSaveText);
    for (int i = 0; i < kEquipSlots; ++i)
    {
        const int id = h.getEquipped(static_cast<EquipSlot>(i));
        drawText(tr(equipSlotName(static_cast<EquipSlot>(i))), win.x + 40, eq + 34 + i * 30, kSaveDim);
        drawText(id ? game.getItem(id).name : "-", win.x + 160, eq + 34 + i * 30, kSaveText);
    }
    drawText(tr("Resists"), win.x + 450, eq, kSaveText);
    drawText(elementList(h.getResists()), win.x + 450, eq + 34, kSaveDim);
    drawText(tr("Weak to"), win.x + 450, eq + 80, kSaveText);
    drawText(elementList(h.getVulnerabilities()), win.x + 450, eq + 114, kSaveDim);
}

// ─────────────────────────────────────────
// Настройки: громкость и подсказка по управлению
// ─────────────────────────────────────────

namespace {
    // Строки окна: две полоски громкости, три переключателя и «Back»
    enum ConfigRow { kRowMusic, kRowSound, kRowEncounters, kRowMinimap, kRowScreen, kRowLanguage, kRowBack, kConfigRows };
    bool isVolumeRow(int r) { return r == kRowMusic || r == kRowSound; }
    bool isToggleRow(int r) { return r == kRowEncounters || r == kRowMinimap || r == kRowScreen || r == kRowLanguage; }
    SDL_Rect configWindow(const SDL_Rect& screen) { return {120, 20, screen.w - 240, screen.h - 40}; }
}

SDL_Rect UIManager::configRowRect(int row) const
{
    const SDL_Rect win = configWindow(screen);
    return {win.x + 30, win.y + 66 + row * 40, win.w - 60, 36};
}

SDL_Rect UIManager::volumeBarRect(int row) const
{
    const SDL_Rect r = configRowRect(row);
    return {r.x + 190, r.y + 10, AudioManager::kVolumeSteps * 30 - 4, 20};
}

void UIManager::changeVolume(int row, int level)
{
    if (row == kRowMusic)
    {
        if (level == AudioManager::getMusicVolume()) return;
        AudioManager::setMusicVolume(level);
    }
    else if (row == kRowSound)
    {
        if (level == AudioManager::getSoundVolume()) return;
        AudioManager::setSoundVolume(level);
        int channel = -1;                                         // образец, чтобы было слышно новую громкость
        if (Mix_Chunk* sample = ResourceManager::getSound("ButtonHover"))
            AudioManager::playSound(channel, sample);
    }
}

std::vector<std::string> UIManager::optionLabels(int row) const
{
    if (row == kRowEncounters) return {tr("On"), tr("Off")};
    if (row == kRowMinimap)    return {tr("Off"), "x1", "x2", "x3"};
    if (row == kRowScreen)     return {tr("Window"), tr("Fullscreen")};
    if (row == kRowLanguage)   return {languageName(Language::Ru), languageName(Language::En)};
    return {};
}

int UIManager::optionValue(int row) const
{
    if (row == kRowEncounters) return game.encountersEnabled() ? 0 : 1;
    if (row == kRowMinimap)    return game.getMinimapZoom();
    if (row == kRowScreen)     return game.isFullscreen() ? 1 : 0;
    if (row == kRowLanguage)   return currentLanguage() == Language::Ru ? 0 : 1;
    return 0;
}

void UIManager::setOption(int row, int index)
{
    const int n = static_cast<int>(optionLabels(row).size());
    if (n == 0) return;
    index = (index % n + n) % n;                                      // по кругу в обе стороны
    if (row == kRowEncounters) game.setEncountersEnabled(index == 0);
    if (row == kRowMinimap)    game.setMinimapZoom(index);
    if (row == kRowScreen)     game.setFullscreen(index == 1);
    if (row == kRowLanguage)   game.setLanguage(index == 0 ? Language::Ru : Language::En);
}

SDL_Rect UIManager::optionRect(int row, int index) const
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    const SDL_Rect rr = configRowRect(row);
    int x = volumeBarRect(row).x;
    const auto labels = optionLabels(row);
    for (int i = 0; i < static_cast<int>(labels.size()); ++i)
    {
        const int w = ResourceManager::measureText(labels[i], font).w;
        if (i == index) return {x - 8, rr.y, w + 16, rr.h};
        x += w + 36;
    }
    return {};
}

void UIManager::handleConfigEvent(const SDL_Event& e)
{
    const bool keyDown = e.type == SDL_KEYDOWN;
    const SDL_Scancode k = keyDown ? e.key.keysym.scancode : SDL_SCANCODE_UNKNOWN;
    const bool fresh = keyDown && e.key.repeat == 0;

    if ((fresh && k == SDL_SCANCODE_ESCAPE) || (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT))
    {
        game.closeOptions();
        return;
    }

    if (keyDown && (k == SDL_SCANCODE_UP || k == SDL_SCANCODE_W))
        configSelected = (configSelected - 1 + kConfigRows) % kConfigRows;
    if (keyDown && (k == SDL_SCANCODE_DOWN || k == SDL_SCANCODE_S))
        configSelected = (configSelected + 1) % kConfigRows;

    const bool left  = keyDown && (k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_A);
    const bool right = keyDown && (k == SDL_SCANCODE_RIGHT || k == SDL_SCANCODE_D);
    const bool enter = fresh && (k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_KP_ENTER || k == SDL_SCANCODE_SPACE);

    if (isVolumeRow(configSelected))
    {
        const int current = configSelected == kRowMusic ? AudioManager::getMusicVolume() : AudioManager::getSoundVolume();
        if (left)  changeVolume(configSelected, current - 1);
        if (right) changeVolume(configSelected, current + 1);
    }
    if (isToggleRow(configSelected))
    {
        if (fresh && left)           setOption(configSelected, optionValue(configSelected) - 1);
        if ((fresh && right) || enter) setOption(configSelected, optionValue(configSelected) + 1);
    }
    if (enter && configSelected == kRowBack)
    {
        game.closeOptions();
        return;
    }

    // Мышь: наведение выбирает строку, клик переключает, клик или протяжка по полоске ставит громкость
    if (e.type == SDL_MOUSEBUTTONUP) draggingVolume = false;
    const bool press = e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
    if (e.type != SDL_MOUSEMOTION && !press) return;

    const SDL_Point m = e.type == SDL_MOUSEMOTION ? SDL_Point{e.motion.x, e.motion.y}
                                                  : SDL_Point{e.button.x, e.button.y};
    if (draggingVolume && isVolumeRow(configSelected))
    {
        const SDL_Rect bar = volumeBarRect(configSelected);
        changeVolume(configSelected, std::clamp((m.x - bar.x + 15) / 30, 0, AudioManager::kVolumeSteps));
        return;
    }
    for (int r = 0; r < kConfigRows; ++r)
    {
        const SDL_Rect rr = configRowRect(r);
        if (!SDL_PointInRect(&m, &rr)) continue;
        configSelected = r;
        if (!press) return;
        if (r == kRowBack)  { game.closeOptions(); return; }
        if (isToggleRow(r))                                       // клик по варианту выбирает его
        {
            for (int i = 0; i < static_cast<int>(optionLabels(r).size()); ++i)
            {
                const SDL_Rect o = optionRect(r, i);
                if (SDL_PointInRect(&m, &o)) { setOption(r, i); return; }
            }
            setOption(r, optionValue(r) + 1);                     // мимо вариантов: следующий
            return;
        }

        const SDL_Rect bar = volumeBarRect(r);
        if (m.x >= bar.x - 15)                                    // левее полоски — по подписи, громкость не трогаем
        {
            draggingVolume = true;
            changeVolume(r, std::clamp((m.x - bar.x + 15) / 30, 0, AudioManager::kVolumeSteps));
        }
        return;
    }
}

void UIManager::drawConfig()
{
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    const SDL_Rect win = configWindow(screen);
    visualizer.drawNinePatch(skin, win, kWindowSkinSlice, 1);
    drawText(tr("Config"), win.x + 30, win.y + 20, kSaveText);

    const char* labels[kConfigRows] = {"Music", "Sound", "Encounters", "Minimap", "Screen", "Language", "Back"};
    const int levels[2] = {AudioManager::getMusicVolume(), AudioManager::getSoundVolume()};
    for (int r = 0; r < kConfigRows; ++r)
    {
        const SDL_Rect rr = configRowRect(r);
        const bool selected = r == configSelected;
        drawText(tr(labels[r]), rr.x + 22, rr.y + 6, selected ? kSaveHot : kSaveText);
        if (selected) drawPointer(rr.x + 16 - (SDL_GetTicks() / 220) % 2 * 2, rr.y + rr.h / 2, 2);

        const SDL_Rect bar = volumeBarRect(r);
        if (isToggleRow(r))
        {
            // Варианты в ряд: выбранный яркий, остальные приглушены
            const auto labels = optionLabels(r);
            const int value = optionValue(r);
            for (int i = 0; i < static_cast<int>(labels.size()); ++i)
            {
                const SDL_Rect o = optionRect(r, i);
                drawText(labels[i], o.x + 8, rr.y + 8, i == value ? (selected ? kSaveHot : kSaveText) : kSaveDim);
            }
            continue;
        }
        if (!isVolumeRow(r)) continue;

        // Полоска из десяти делений и число справа
        for (int s = 0; s < AudioManager::kVolumeSteps; ++s)
        {
            const SDL_Rect seg{bar.x + s * 30, bar.y, 26, bar.h};
            visualizer.fillRect(seg, s < levels[r] ? (selected ? kSaveHot : kSaveText) : SDL_Color{20, 30, 100, 255});
        }
        drawText(std::to_string(levels[r] * 10) + "%", bar.x + bar.w + 20, rr.y + 8, kSaveText);
    }

    // Подсказка по управлению
    const int helpY = configRowRect(kConfigRows - 1).y + 52;
    drawText(tr("Controls"), win.x + 30, helpY, kSaveText);
    const char* help[] = {
        "Map: WASD or arrows - move, Enter - talk",
        "E - land airship, M - minimap, H - encounters, V - hero",
        "Battle: arrows - target, Enter - OK, B - book",
        "Esc or right click - back, F11 - fullscreen",
    };
    for (int i = 0; i < static_cast<int>(std::size(help)); ++i)
        drawText(tr(help[i]), win.x + 50, helpY + 32 + i * 26, kSaveDim);
}
