#include <SDL_log.h>

#include <utility>

#include "uiManager.h"
#include "game.h"

constexpr int ENV_BUTTON_COUNT = 16;

void UIManager::setState(const UIChooseState &state) {
    this->uiState = state;
}

UIChooseState UIManager::getState() const {
    return uiState;
}


UIManager::Scene *UIManager::findScene(const std::string &id)
{
    auto sit = scenes.find(id);
    if (sit == scenes.end())
    {
        SDL_Log("UIManager: scene '%s' not found", id.c_str());
        return nullptr;
    }

    return &sit->second;
}

UIManager::UIObject* UIManager::findInPanel(const std::string& id, Panel* panel)
{
    if (!panel) return nullptr;

    auto itImage = panel->images.find(id);
    if (itImage != panel->images.end())
        return &itImage->second;

    auto itBtn = panel->buttons.find(id);
    if (itBtn != panel->buttons.end())
        return &itBtn->second;

    auto itLbl = panel->labels.find(id);
    if (itLbl != panel->labels.end())
        return &itLbl->second;

    auto itEdt = panel->edits.find(id);
    if (itEdt != panel->edits.end())
        return &itEdt->second;

    auto itPanel = panel->panels.find(id);
    if (itPanel != panel->panels.end())
        return &itPanel->second;

    for (auto& [childId, childPanel] : panel->panels)
    {
        UIObject* found = findInPanel(id, &childPanel);
        if (found) return found;
    }

    return nullptr;
}

UIManager::UIObject* UIManager::findUIObject(const std::string &id, const std::string &sceneId)
{
    Scene *scene = findScene(sceneId);
    if (!scene) return nullptr;

    UIObject* result = findInPanel(id, scene);
    if (!result)
        SDL_Log("UIManager: element '%s' not found in scene '%s'", id.c_str(), sceneId.c_str());

    return result;
}


void UIManager::Panel::addButtonLocal(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture,
                                      std::function<void()> onClick, std::function<void()> onHover)
{
    Button b;
    b.rect = rect;
    b.texture = texture;
    b.onClick = std::move(onClick);
    b.onHover = std::move(onHover);
    buttons[id] = std::move(b);
    order.push_back(id);
}

void UIManager::Panel::addEditLocal(const std::string& id, const SDL_Rect& rect, const SDL_Rect& srect, SDL_Texture* texture,
                                      std::function<void()> onClick, std::function<void()> onHover)
{
    Edit ed;
    ed.rect = rect;
    ed.srect = srect;
    ed.texture = texture;
    ed.onClick = std::move(onClick);
    ed.onHover = std::move(onHover);
    edits[id] = std::move(ed);
    order.push_back(id);
}

void UIManager::Panel::addImageLocal(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect)
{
    Image im;
    im.rect = rect;
    im.texture = texture;
    im.srect = srect;
    images[id] = im;
    order.push_back(id);
}

void UIManager::Panel::addLabelLocal(const std::string& id, const SDL_Rect& textRect, const SDL_Rect& imageRect,
                                     SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect)
{
    Label lbl;
    lbl.rect = textRect;
    lbl.texture = text;
    lbl.image.rect = imageRect;
    lbl.image.texture = texture;
    lbl.image.srect = srect;
    labels[id] = lbl;
    order.push_back(id);
}

void UIManager::Panel::addPanelLocal(const std::string& id)
{
    if (!panels.contains(id)) {
        panels[id] = Panel();
        order.push_back(id);
    }
}

void UIManager::Scene::playMusicLocal(const Visualizer& visualizer, const std::string& musicId)
{
    auto musicIt = musicPlayer.music.find(musicId);
    if (musicIt == musicPlayer.music.end()) {
        SDL_Log("Scene::playMusicLocal: music '%s' not found", musicId.c_str());
        return;
    }
    Mix_Music* music = musicIt->second;
    if (musicPlayer.currentMusicId == musicId && Mix_PlayingMusic()) return;
    if (Mix_PlayingMusic()) visualizer.stopMusic();
    visualizer.playMusic(music);
    musicPlayer.currentMusicId = musicId;
}

void UIManager::Scene::playSoundLocal(const Visualizer& visualizer, const std::string& soundId)
{
    auto soundIt = musicPlayer.sound.find(soundId);
    if (soundIt == musicPlayer.sound.end())
    {
        SDL_Log("Scene::playSoundLocal: sound '%s' not found", soundId.c_str());
        return;
    }
    Mix_Chunk* chunk = soundIt->second;
    if (musicPlayer.currentSoundId == soundId && Mix_Playing(musicPlayer.chanel)) return;
    if (Mix_Playing(musicPlayer.chanel)) visualizer.stopSound(musicPlayer.chanel);
    visualizer.playSound(musicPlayer.chanel, chunk);
    musicPlayer.currentSoundId = soundId;
}
void UIManager::Scene::drawPanel(const Panel& panel, Visualizer& visualizer)
{
    for (const auto& key : panel.order)
    {
        auto itImage = panel.images.find(key);
        if (itImage != panel.images.end()) {
            const auto& im = itImage->second;
            if (!im.isVisible)
                continue;
            if (im.srect.w == 0 || im.srect.h == 0)
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h);
            else
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h,
                                       im.srect.x, im.srect.y, im.srect.w, im.srect.h);
            continue;
        }

        auto itBtn = panel.buttons.find(key);
        if (itBtn != panel.buttons.end()) {
            const auto& btn = itBtn->second;
            if (!btn.isVisible)
                continue;
            visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
            continue;
        }

        auto itEd = panel.edits.find(key);
        if (itEd != panel.edits.end()) {
            const auto& edn = itEd->second;
            if (!edn.isVisible)
                continue;
            edn.texture = ResourceManager::getTextTexture(edn.text, ResourceManager::getFont("RetroByte"), {0,0,0,0});
            visualizer.drawTexture(ResourceManager::getTexture("WhiteBox"), edn.rect.x, edn.rect.y, edn.rect.w, edn.rect.h);
            visualizer.drawTexture(edn.texture, edn.rect.x, edn.rect.y, edn.rect.w, edn.rect.h);
            continue;
        }

        auto itLbl = panel.labels.find(key);
        if (itLbl != panel.labels.end()) {
            const auto& lbl = itLbl->second;
            if (!lbl.isVisible)
                continue;
            if (lbl.image.srect.w == 0 || lbl.image.srect.h == 0)
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h);
            else
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h,
                                       lbl.image.srect.x, lbl.image.srect.y, lbl.image.srect.w, lbl.image.srect.h);
            visualizer.drawTexture(lbl.texture, lbl.rect.x, lbl.rect.y, lbl.rect.w, lbl.rect.h);
            continue;
        }

        auto itPanel = panel.panels.find(key);
        if (itPanel != panel.panels.end()) {
            drawPanel(itPanel->second, visualizer);
            continue;
        }

        SDL_Log("UIManager::Scene::drawPanel: element '%s' not found in panel", key.c_str());
    }
}


void UIManager::Scene::changePanelOrder(const std::string& panelId, int newOrder)
{
    auto it = std::
    find(order.begin(), order.end(), panelId);
    if (it == order.end()) return;
    int oldIndex = static_cast<int>(std::distance(order.begin(), it));
    if (oldIndex == newOrder) return;
    if (newOrder < 0) newOrder = 0;
    if (newOrder >= static_cast<int>(order.size())) newOrder = static_cast<int>(order.size()) - 1;

    std::string val = *it;
    order.erase(it);
    order.insert(order.begin() + newOrder, std::move(val));
}

void UIManager::addScene(const std::string& id)
{
    if (scenes.find(id) == scenes.end())
    {
        scenes[id] = Scene();
        scenes[id].name = id;
    }
}

void UIManager::addPanel(const std::string &id, const std::string &sceneId, const std::string &panelId)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("addPanel: scene '%s' not found.", sceneId.c_str());
        return;
    }

    if (panelId.empty())
        scenes[sceneId].addPanelLocal(id);
    else if (scenes[sceneId].panels.contains(panelId))
        scenes[sceneId].panels[panelId].addPanelLocal(id);
    else
        SDL_Log("Panel '%s' not found.", panelId.c_str());
}



void UIManager::addButton(const std::string& id, const std::string& sceneId, const SDL_Rect& rect,
                          SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover, std::string panelId)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("addButton: scene '%s' not found.", sceneId.c_str());
        return;
    }

    if (panelId.empty())
        scenes[sceneId].addButtonLocal(id, rect, texture, std::move(onClick), std::move(onHover));
    else if (scenes[sceneId].panels.contains(panelId))
        scenes[sceneId].panels[panelId].addButtonLocal(id, rect, texture, std::move(onClick), std::move(onHover));
    else
        SDL_Log("Panel '%s' not found.", panelId.c_str());
}

void UIManager::addEdit(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, const SDL_Rect& srect,
                          SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover, std::string panelId)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("addEdit: scene '%s' not found.", sceneId.c_str());
        return;
    }

    if (panelId.empty())
        scenes[sceneId].addEditLocal(id, rect, srect, texture, std::move(onClick), std::move(onHover));
    else if (scenes[sceneId].panels.contains(panelId))
        scenes[sceneId].panels[panelId].addEditLocal(id, rect, srect, texture, std::move(onClick), std::move(onHover));
    else
        SDL_Log("Panel '%s' not found.", panelId.c_str());
}

void UIManager::addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect, std::string panelId)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("addImage: scene '%s' not found.", sceneId.c_str());
        return;
    }

    if (panelId.empty())
        scenes[sceneId].addImageLocal(id, rect, texture, srect);
    else if (scenes[sceneId].panels.contains(panelId))
        scenes[sceneId].panels[panelId].addImageLocal(id, rect, texture, srect);
    else
        SDL_Log("Panel '%s' not found.", panelId.c_str());
}

void UIManager::addLabel(const std::string& id, const std::string& sceneId, const SDL_Rect& textRect,
                         const SDL_Rect& imageRect, SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect, std::string panelId)
{
    if (!scenes.contains(sceneId)) {
        SDL_Log("addLabel: scene '%s' not found.", sceneId.c_str());
        return;
    }

    if (panelId.empty())
        scenes[sceneId].addLabelLocal(id, textRect, imageRect, text, texture, srect);
    else if (scenes[sceneId].panels.contains(panelId))
        scenes[sceneId].panels[panelId].addLabelLocal(id, textRect, imageRect, text, texture, srect);
    else
        SDL_Log("Panel '%s' not found.", panelId.c_str());
}

void UIManager::addMusic(const std::string& musicId, const std::string& sceneId)
{
    auto sit = scenes.find(sceneId);
    if (sit == scenes.end()) {
        SDL_Log("addMusic: scene '%s' not found.", sceneId.c_str());
        return;
    }
    Mix_Music* m = ResourceManager::getMusic(musicId);
    if (!m)
    {
        SDL_Log("addMusic: resource '%s' not found", musicId.c_str());
        return;
    }
    scenes[sceneId].musicPlayer.music[musicId] = m;
}

void UIManager::addSound(const std::string& soundId, const std::string& sceneId)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("addSound: scene '%s' not found.", sceneId.c_str());
        return;
    }
    Mix_Chunk* c = ResourceManager::getSound(soundId);
    if (!c)
    {
        SDL_Log("addSound: resource '%s' not found", soundId.c_str());
        return;
    }
    scenes[sceneId].musicPlayer.sound[soundId] = c;
}

void UIManager::addAnimation(const std::string& id, const std::string& sceneId, std::vector<SDL_Texture*> frames, std::vector<SDL_Rect> rects, float frameTime, AnimPlayMode mode)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("Scene '%s' not found", sceneId.c_str());
        return;
    }

    scenes[sceneId].animPlayer.animations[id] =
    {
        frameTime,
        std::move(frames),
        std::move(rects)
    };
}

void UIManager::addAnimation(const std::string& id, const std::string& sceneId, std::vector<SDL_Texture*> frames, SDL_Rect rect, float frameTime, AnimPlayMode mode)
{
    if (!scenes.contains(sceneId))
    {
        SDL_Log("Scene '%s' not found", sceneId.c_str());
        return;
    }

    scenes[sceneId].animPlayer.animations[id] =
    {
        frameTime,
        std::move(frames),
        {},
        rect,
        0,
        0.0f,
        false,
        mode
    };
}

void UIManager::playAnimation(const std::string& animId, const std::string& sceneId, bool restart)
{
    auto sit = scenes.find(sceneId);
    if (sit == scenes.end())
    {
        SDL_Log("Scene '%s' not found when trying to play animation '%s'", sceneId.c_str(), animId.c_str());
        return;
    }

    sit->second.animPlayer.play(animId, restart);
}

bool UIManager::isAnimationFinished(const std::string& sceneId) const
{
    auto sit = scenes.find(sceneId);
    if (sit == scenes.end())
        return true;

    const AnimPlayer& p = sit->second.animPlayer;
    return p.currentAnimationId.empty() && p.animationQueue.empty();
}

void AnimPlayer::play(const std::string& id, bool restart)
{
    if (!animations.contains(id))
        return;

    Anim& anim = animations[id];

    if (!currentAnimationId.empty())
    {
        // ❗ loop перебивает loop сразу
        if (anim.mode == AnimPlayMode::Loop)
        {
            currentAnimationId = id;
        }
        else
        {
            animationQueue.push({id, restart});
            return;
        }
    }
    else
    {
        currentAnimationId = id;
    }

    if (restart || anim.finished)
    {
        anim.currentFrame = 0;
        anim.accumulator = 0.0f;
        anim.finished = false;
    }
}


void AnimPlayer::stopLoop()
{
    if (currentAnimationId.empty())
        return;

    if (animations[currentAnimationId].mode == AnimPlayMode::Loop)
        currentAnimationId.clear();
}


void AnimPlayer::update(float dt)
{
    if (currentAnimationId.empty())
        return;

    Anim& anim = animations[currentAnimationId];
    anim.accumulator += dt;

    while (anim.accumulator >= anim.frameTime)
    {
        anim.accumulator -= anim.frameTime;
        anim.currentFrame++;

        if (anim.currentFrame >= anim.frames.size())
        {
            if (anim.mode == AnimPlayMode::Loop)
            {
                anim.currentFrame = 0;
            }
            else // OneShot
            {
                anim.currentFrame = anim.frames.size() - 1;
                anim.finished = true;
                break;
            }
        }
    }

    if (anim.finished && anim.mode == AnimPlayMode::OneShot)
    {
        if (!animationQueue.empty())
        {
            QueuedAnim next = animationQueue.front();
            animationQueue.pop();

            currentAnimationId = next.id;
            Anim& nextAnim = animations[next.id];
            nextAnim.currentFrame = 0;
            nextAnim.accumulator = 0.0f;
            nextAnim.finished = false;
        }
        else
        {
            currentAnimationId.clear();
        }
    }
}


void AnimPlayer::draw(Visualizer& v)
{
    // Если нет активной анимации — рисовать нечего
    if (currentAnimationId.empty())
        return;

    const Anim& anim = animations.at(currentAnimationId);

    // Защита: если кадров нет
    if (anim.frames.empty())
        return;

    SDL_Texture* frame = anim.frames[anim.currentFrame];
    const SDL_Rect& rect = anim.rects[anim.currentFrame];

    v.drawTexture(frame, rect.x, rect.y, rect.w, rect.h);
}

void AnimPlayer::draw(Visualizer& v, const SDL_Rect& screen)
{
    if (currentAnimationId.empty())
        return;

    const Anim& anim = animations.at(currentAnimationId);

    SDL_Rect rect;
    rect.x = screen.w/2 - anim.frameRect.w/2;
    rect.y = screen.h/2 - anim.frameRect.h/2;
    rect.w = anim.frameRect.w;
    rect.h = anim.frameRect.h;

    SDL_Texture* frame = anim.frames[anim.currentFrame];
    v.drawTexture(frame, rect.x, rect.y, rect.w, rect.h);
}

void UIManager::updatePlayerAnimation(AnimPlayer &animPlayer, const Player& player)
{
    if (player.getDirection() == Direction::Idle)
    {
        animPlayer.stopLoop();
        currentPlayerAnim.clear();
        setEnVI("Character", "Map", true);
        return;
    }

    std::string nextAnim;

    setEnVI("Character", "Map", false);

    switch (player.getDirection())
    {
        case Direction::Up:    nextAnim = "MoveUpAnimation"; break;
        case Direction::Down:  nextAnim = "MoveDownAnimation"; break;
        case Direction::Left:  nextAnim = "MoveLeftAnimation"; break;
        case Direction::Right: nextAnim = "MoveRightAnimation"; break;
        default: return;
    }

    if (currentPlayerAnim != nextAnim)
    {
        currentPlayerAnim = nextAnim;
        playAnimation(nextAnim, "Map", true);
    }
}

void UIManager::updateRemoteAnimation(AnimPlayer& animPlayer, const Player& player)
{
    if (player.getDirection() == Direction::Idle)
    {
        animPlayer.stopLoop();
        return;
    }

    std::string nextAnim;
    switch (player.getDirection())
    {
        case Direction::Up:    nextAnim = "MoveUpAnimation"; break;
        case Direction::Down:  nextAnim = "MoveDownAnimation"; break;
        case Direction::Left:  nextAnim = "MoveLeftAnimation"; break;
        case Direction::Right: nextAnim = "MoveRightAnimation"; break;
        default: return;
    }

    animPlayer.play(nextAnim, false);
}


void UIManager::playSound(const std::string& sceneId, const std::string& soundId)
{
    auto sceneIt = scenes.find(sceneId);
    if (sceneIt == scenes.end())
    {
        SDL_Log("Scene '%s' not found", sceneId.c_str());
        return;
    }

    sceneIt->second.playSoundLocal(visualizer, soundId);
}

void UIManager::Panel::handleClickLocal(int x, int y)
{
    for (auto& [id, btn] : buttons)
    {
        if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
            y >= btn.rect.y && y <= btn.rect.y + btn.rect.h)
        {
            if (btn.onClick && btn.isEnabled) btn.onClick();
            return;
        }
    }

    for (auto& [id, idn] : edits)
    {
        if (x >= idn.rect.x && x <= idn.rect.x + idn.rect.w &&
            y >= idn.rect.y && y <= idn.rect.y + idn.rect.h)
        {
            if (idn.onClick && idn.isEnabled) idn.onClick();
            return;
        }
    }

    for (auto& [id, panel] : panels)
        panel.handleClickLocal(x, y);

}

void UIManager::Scene::handleClickLocal(int x, int y)
{
    Panel::handleClickLocal(x, y);
}

void UIManager::Panel::handleHoverLocal(int x, int y)
{
    for (auto& [id, btn] : buttons)
    {
        if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
            y >= btn.rect.y && y <= btn.rect.y + btn.rect.h)
        {
            if (btn.onHover) btn.onHover();
            return;
        }
    }

    for (auto& [id, panel] : panels)
    {
        panel.handleHoverLocal(x, y);
    }
}

void UIManager::Scene::handleHoverLocal(int x, int y)
{
    Panel::handleHoverLocal(x, y);
}

void UIManager::handleClickEvent(const SDL_Event& event, const std::string& sceneId)
{
    if (event.type != SDL_MOUSEBUTTONDOWN) return;

    int x = event.button.x;
    int y = event.button.y;

    auto sit = scenes.find(sceneId);
    if (sit == scenes.end()) return;

    sit->second.handleClickLocal(x, y);
}

void UIManager::handleHoverEvent(const SDL_Event& event, const std::string& sceneId)
{
    if (event.type != SDL_MOUSEMOTION) return;

    int x = event.motion.x;
    int y = event.motion.y;

    auto sit = scenes.find(sceneId);
    if (sit == scenes.end()) return;

    sit->second.handleHoverLocal(x, y);
}

void UIManager::drawScene(const std::string& sceneId)
{
    auto sit = scenes.find(sceneId);
    if (sit == scenes.end())
    {
        SDL_Log("drawScene: scene '%s' not found", sceneId.c_str());
        return;
    }
    Scene& scene = sit->second;
    const std::string music = sceneId + "Theme";

    setTexture("Time", sceneId, ResourceManager::getTextTexture(game.getCurrentTime(), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    Point localPos = player.getPlayerCoords();

    if (sceneId == "Map")
    {
        setTexture("FPS", "Map", ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));


        auto it = scene.images.find("MapBackground");
        if (it != scene.images.end()) {
            it->second.rect.x = localPos.x - 900;
            it->second.rect.y = localPos.y - 500;
        }
    }

    if (sceneId == "Battle")
    {
        setTexture("FPS", "Battle", ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
        const int enemyCount = game.getBattle()->getEnemies().size();
        const int heroCount = player.getHeroes().size();
        auto it = scene.images.find("HerosStatus");
        if (it != scene.images.end())
        {
            std::string text, text1;
            SDL_Rect drect, screen = game.getScreenRect();
            for (int i = 0; i < heroCount; i++)
            {
                text +=  "Hero" + std::to_string(i + 1) + " " + std::to_string(player.getHeroes().at(i).getCurrentHp()) + "/" + std::to_string(player.getHeroes().at(i).getMaxHp()) + "\n";

                std::string text1 = "Image" + std::to_string(i + 1);

                if (player.getHeroes().at(i).getCurrentHp() <= 0)
                {
                    drect = {screen.w - 100, screen.h/2 - 230 + (i * 100), 66, 84};
                    text1 += "Dead";
                }
                else
                    drect = {screen.w - 100, screen.h/2 - 230 + (i * 100), 56, 74};


                setRect("Hero" + std::to_string(i + 1), "Battle", drect);
                setTexture("Hero" + std::to_string(i + 1), "Battle", ResourceManager::getTexture(text1));
            }
            setTexture("HerosStatus", "Battle", ResourceManager::getTextTexture(text, ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}, 245));
        }

        it = scene.images.find("EnemiesStatus");
        if (it != scene.images.end())
        {
            std::string text;
            for (int i = 0; i < enemyCount; i++)
            {
                if (!game.getBattle()->getEnemies().at(i).getIsAlive())
                    setTexture("Enemy" + std::to_string(i + 1), "Battle", nullptr);
                text +=  "Enemy" + std::to_string(i + 1) + " " + std::to_string(game.getBattle()->getEnemies().at(i).getCurrentHp()) + "/" + std::to_string(game.getBattle()->getEnemies().at(i).getMaxHp()) + "\n";
            }

            SDL_Rect rect = getRect("EnemiesStatus", "Battle");
            rect.h = enemyCount * 40;
            setRect("EnemiesStatus", "Battle", rect);

            setTexture("EnemiesStatus", "Battle", ResourceManager::getTextTexture(text, ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}, 245));
        }
    }

    for (const auto& key : scene.order)
    {
        auto itImage = scene.images.find(key);
        if (itImage != scene.images.end()) {
            if (!itImage->second.isVisible)
                continue;
            const auto& im = itImage->second;
            if (im.srect.w == 0 || im.srect.h == 0)
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h);
            else
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h,
                                       im.srect.x, im.srect.y, im.srect.w, im.srect.h);
            continue;
        }

        auto itEd = scene.edits.find(key);
        if (itEd != scene.edits.end()) {
            const auto& edn = itEd->second;
            if (!edn.isVisible)
                continue;
            edn.texture = ResourceManager::getTextTexture(edn.text, ResourceManager::getFont("RetroByte"), {0,0,0,0});
            visualizer.drawTexture(ResourceManager::getTexture("WhiteBox"), edn.rect.x, edn.rect.y, edn.rect.w, edn.rect.h);
            visualizer.drawTexture(edn.texture, edn.rect.x, edn.rect.y, edn.rect.w, edn.rect.h);
            continue;
        }

        auto itBtn = scene.buttons.find(key);
        if (itBtn != scene.buttons.end())
        {
            if (!itBtn->second.isVisible)
                continue;
            const auto& btn = itBtn->second;
            visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
            continue;
        }

        auto itLbl = scene.labels.find(key);
        if (itLbl != scene.labels.end())
        {
            if (!itLbl->second.isVisible)
                continue;
            const auto& lbl = itLbl->second;
            if (lbl.image.srect.w == 0 || lbl.image.srect.h == 0)
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h);
            else
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h,
                                       lbl.image.srect.x, lbl.image.srect.y, lbl.image.srect.w, lbl.image.srect.h);
            visualizer.drawTexture(lbl.texture, lbl.rect.x, lbl.rect.y, lbl.rect.w, lbl.rect.h);
            continue;
        }

        auto itPanel = scene.panels.find(key);
        if (itPanel != scene.panels.end())
        {
            if (!itPanel->second.isVisible)
                continue;
            scene.drawPanel(itPanel->second, visualizer);
            continue;
        }

        SDL_Log("UIManager: element '%s' not found in scene '%s'", key.c_str(), sceneId.c_str());
    }

    if (sceneId == "Map")
        for (const auto& [id, remote] : game.getRemotePlayers())
        {
            Point remotePos = remote.getPlayerCoords();

            // Смещение удалённого игрока относительно локального
            int screenX = (localPos.x - remotePos.x) + screen.w / 2;
            int screenY = (localPos.y - remotePos.y) + screen.h / 2;

            for (const auto& [id, remote] : game.getRemotePlayers())
            {
                Point remotePos = remote.getPlayerCoords();
                int screenX = (localPos.x - remotePos.x) + screen.w / 2;
                int screenY = (localPos.y - remotePos.y) + screen.h / 2;

                const AnimPlayer& anim = remote.animPlayer;

                if (remote.getDirection() == Direction::Idle || anim.currentAnimationId.empty())
                {
                    // Стоит — статичный спрайт
                    visualizer.drawTexture(
                        ResourceManager::getTexture("MovingCharacter"),
                        screenX, screenY, 45, 45, 0, 0, 16, 25
                    );
                }
                else
                {
                    // Идёт — текущий кадр анимации
                    const auto& currentAnim = anim.animations.at(anim.currentAnimationId);
                    if (!currentAnim.frames.empty())
                    {
                        SDL_Texture* frame = currentAnim.frames[currentAnim.currentFrame];
                        visualizer.drawTexture(frame, screenX, screenY, 45, 45);
                    }
                }
            }
        }

    if (!music.empty())
        scene.playMusicLocal(visualizer, music);

    if (sceneId == "Map")
        scene.animPlayer.draw(visualizer, game.getScreenRect());
    else
        scene.animPlayer.draw(visualizer);
}

void UIManager::addEnemys()
{
    const std::vector<Enemy> &enemies = this->game.getBattle()->getEnemies();
    for (int i = 0; i < enemies.size(); i++)
        setTexture(("Enemy" + std::to_string(i + 1)), "Battle", ResourceManager::getTexture(enemies.at(i).getName()));
}

void UIManager::addCharacters()
{
    for (int i = 0; i < MAX_HERO_COUNT; i++)
        setTexture(("Hero" + std::to_string(i + 1)), "Battle", ResourceManager::getTexture(("Image" + std::to_string(i + 1))));
}

void UIManager::setEnabled(const std::string &id, const std::string &sceneId, const bool &enabled)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        obj->isEnabled = enabled;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setEnVI(const std::string &id, const std::string &sceneId, const bool &enabled)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr) {
        obj->isEnabled = enabled;
        obj->isVisible = enabled;
    }
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}


void UIManager::setVisible(const std::string &id, const std::string &sceneId, const bool &visible)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
    {
        obj->isVisible = visible;
        obj->isEnabled = visible;
    }
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setRect(const std::string &id, const std::string &sceneId, const SDL_Rect &rect)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        obj->rect = rect;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setSrcRect(const std::string& id, const std::string& sceneId, const SDL_Rect& srect)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        dynamic_cast<Image*>(obj)->srect = srect;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}


void UIManager::setText(const std::string& id, const std::string& sceneId, const std::string& text)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        dynamic_cast<Edit*>(obj)->text = text;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}


std::string UIManager::getText(const std::string& id, const std::string& sceneId)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        return dynamic_cast<Edit*>(obj)->text;
    SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
    return "";
}

void UIManager::setFocus(const std::string& id, const std::string& sceneId, const bool& focus)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        dynamic_cast<Edit*>(obj)->isFocused = focus;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}


bool UIManager::getFocus(const std::string& id, const std::string& sceneId)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        return dynamic_cast<Edit*>(obj)->isFocused;
    SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
    return false;
}

int UIManager::getPos(const std::string& id, const std::string& sceneId)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        return dynamic_cast<Edit*>(obj)->pos;
    SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
    return 0;
}


void UIManager::setPos(const std::string& id, const std::string& sceneId, const int& pos)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
    {
        auto* edit = dynamic_cast<Edit*>(obj);
        int clamped = std::clamp(pos, 0, (int)edit->text.length()); // >= вместо >
        edit->pos = clamped;
    }
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setTexture(const std::string &id, const std::string &sceneId, SDL_Texture *texture)
{
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        obj->texture = texture;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setOnClick(const std::string &id, const std::string &sceneId, const std::function<void()>& onClick) {
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
    {
        if (const auto but = dynamic_cast<Button*>(obj))
            but->onClick = onClick;
    }
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

SDL_Rect UIManager::getRect(const std::string &id, const std::string &sceneId) {
    if (const auto obj = findUIObject(id, sceneId); obj != nullptr)
        return findUIObject(id, sceneId)->rect;
    else
    {
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
        return {0,0,0,0};
    }
}

void UIManager::onActionButton(ActionType type)
{
    pendingAction = Action{};
    currentHeroIndex = game.getBattle()->getCurrentHeroIndex();
    pendingAction.actorIndex = currentHeroIndex;
    pendingAction.type = type;

    switch (type)
    {
        case ActionType::Attack:
            pendingAction.targetType = TargetType::Enemy;
            uiState = UIChooseState::ChooseTarget;
            moveSelector();
            setEnVI("ActorSelector", "Battle", true);
            setEnVI("TargetSelector", "Battle", true);
            break;

        case ActionType::Magic:
            uiState = UIChooseState::ChooseSkill;
            initMagic();
            setEnVI("EnvironmentPanel", "Battle", true);
            break;

        case ActionType::Item:
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

void UIManager::initMagic() {
    for (int i = 0 ; i < player.getHeroes().at(currentHeroIndex).getAvailableSkills().size() && i < ENV_BUTTON_COUNT; i+=2)
    {
        std::string text = game.getSkill(player.getHeroes().at(currentHeroIndex).getAvailableSkills().at(i).skillId).name;
        setTexture("EnButton" + std::to_string(i + 1), "Battle", ResourceManager::getTextTexture(text, ResourceManager::getFont("RetroByte"), {0,0,0,0}));
        setOnClick("EnButton" + std::to_string(i + 1), "Battle", [this, i](){this->onEnvironmentCLick(player.getHeroes().at(currentHeroIndex).getAvailableSkills().at(i).skillId, true);});
        setEnVI("EnButton" + std::to_string(i + 1), "Battle", true);
    }
}

void UIManager::initItems() {
    for (int i = 2 ; i < player.getAvailableItems().size() && i < ENV_BUTTON_COUNT; i+=2)
    {
        std::string text = game.getItem(player.getAvailableItems().at(i)).name;
        setTexture("EnButton" + std::to_string(i), "Battle", ResourceManager::getTextTexture(text, ResourceManager::getFont("RetroByte"), {0,0,0,0}));
        setOnClick("EnButton" + std::to_string(i), "Battle", [this, i](){this->onEnvironmentCLick(player.getAvailableItems().at(i), false);});
        setEnVI("EnButton" + std::to_string(i), "Battle", true);
    }
}

void UIManager::onEnvironmentCLick(const int &id, const bool &isMagic)
{
    pendingAction = Action{};
    pendingAction.actorIndex = game.getBattle()->getCurrentHeroIndex();
    pendingAction.payloadId = id;

    if (isMagic)
    {
        pendingAction.targetType = game.getSkill(id).targetType;
        pendingAction.type = ActionType::Magic;
    }
    else
    {
        pendingAction.targetType = game.getItem(id).targetType;
        pendingAction.type = ActionType::Item;
    }

    if (pendingAction.targetType != TargetType::Self)
    {
        uiState = UIChooseState::ChooseTarget;
        moveSelector();
        setEnVI("ActorSelector", "Battle", true);
        setEnVI("TargetSelector", "Battle", true);
        setEnVI("EnvironmentPanel", "Battle", false);
    }
}


void UIManager::onSkillSelected(int skillId)
{
    pendingAction.payloadId = skillId;

    const Skill& skill = game.getSkill(skillId);
    pendingAction.targetType = skill.targetType;

    setEnVI("MagicPanel", "Battle", false);

    if (skill.targetType == TargetType::Self ||
        skill.targetType == TargetType::AllEnemies ||
        skill.targetType == TargetType::AllAllies)
    {
        game.getBattle()->confirmAction(pendingAction);
        uiState = UIChooseState::ChooseAction;
        setEnVI("ActionPanel", "Battle", true);
    }
    else
    {
        uiState = UIChooseState::ChooseTarget;
        moveSelector();
        setEnVI("ActorSelector", "Battle", true);
        setEnVI("TargetSelector", "Battle", true);
    }
}


void UIManager::onItemSelected(int itemId)
{
    pendingAction.payloadId = itemId;

    const Item& item = game.getItem(itemId);
    pendingAction.targetType = item.targetType;

    setEnVI("InventoryPanel", "Battle", false);

    if (pendingAction.targetType == TargetType::Self ||
        pendingAction.targetType == TargetType::AllEnemies ||
        pendingAction.targetType == TargetType::AllAllies)
    {
        game.getBattle()->confirmAction(pendingAction);
        uiState = UIChooseState::ChooseAction;
        //setEnVI("ActionPanel", "Battle", true);
    }
    else
    {
        uiState = UIChooseState::ChooseTarget;
        moveSelector();
        setEnVI("ActorSelector", "Battle", true);
        setEnVI("TargetSelector", "Battle", true);
    }
}

void UIManager::onTargetSelected(int index)
{
    pendingAction.targetIndex = index;

    game.getBattle()->confirmAction(pendingAction);

    setEnVI("TargetSelector", "Battle", false);

    uiState = UIChooseState::ChooseAction;
    setEnVI("ActionPanel", "Battle", true);
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

int UIManager::getTargetCount(TargetType type) const
{
    switch (type)
    {
        case TargetType::Enemy:
            return game.getBattle()->getEnemies().size();

        case TargetType::Ally:
            return player.getHeroes().size();

        default:
            return 0;
    }
}

void UIManager::moveSelector()
{
    currentType = pendingAction.targetType;
    selectedTargetIndex = 0;

    SDL_Rect targetRect{};
    SDL_Rect actorRect = getRect("Hero" + std::to_string(currentHeroIndex + 1), "Battle");

    if (currentType == TargetType::Enemy)
    {
        for (int i = 0; i < game.getBattle()->getEnemies().size(); i++)
            if (game.getBattle()->getEnemies().at(i).getIsAlive())
            {
                targetRect = getRect("Enemy" + std::to_string(i + 1), "Battle");
                break;
            }
    }
    else if (currentType == TargetType::Ally)
    {
        for (int i = 0; i < player.getHeroes().size(); i++)
            if (player.getHeroes().at(i).getIsAlive())
            {
                targetRect = getRect("Hero" + std::to_string(i + 1), "Battle");
                break;
            }
    }
    else if (currentType == TargetType::Self)
        targetRect = actorRect;

    SDL_Rect targetSel = getRect("TargetSelector", "Battle");
    SDL_Rect actorSel = getRect("ActorSelector", "Battle");

    targetSel.x = targetRect.x + targetRect.w / 2 - targetSel.w / 2;
    targetSel.y = targetRect.y - targetRect.h;

    actorSel.x = actorRect.x + actorRect.w / 2 - actorSel.w / 2;
    actorSel.y = actorRect.y - actorRect.h;

    setRect("TargetSelector", "Battle", targetSel);
    setRect("ActorSelector", "Battle", actorSel);
}



void UIManager::moveSelectorToNext()
{
    if (auto selector = findUIObject("TargetSelector", "Battle"))
    {
        if (!selector->isEnabled) return;
    }
    else return;


    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex + 1) % max;

    SDL_Rect targetRect, oldRect = getRect("TargetSelector", "Battle");
    if (currentType == TargetType::Enemy || currentType == TargetType::AllEnemies)
    {
        while (!game.getBattle()->getEnemies().at(selectedTargetIndex).getIsAlive())
            selectedTargetIndex = (selectedTargetIndex + 1) % max;
        targetRect = getRect("Enemy" + std::to_string(selectedTargetIndex + 1), "Battle");
    }
    else if (currentType == TargetType::Ally || currentType == TargetType::AllAllies)
    {
        while (!player.getHeroes().at(selectedTargetIndex).getIsAlive())
            selectedTargetIndex = (selectedTargetIndex + 1) % max;
        targetRect = getRect("Hero" + std::to_string(selectedTargetIndex + 1), "Battle");
    }

    oldRect.x = targetRect.x + (targetRect.w / 2) - (oldRect.w / 2);
    oldRect.y = targetRect.y - targetRect.h;

    setRect("TargetSelector", "Battle", oldRect);
}

void UIManager::moveSelectorToPrevious() {
    if (auto selector = findUIObject("TargetSelector", "Battle"))
    {
        if (!selector->isEnabled) return;
    }
    else return;

    const int max = getTargetCount(currentType);
    if (max == 0) return;

    selectedTargetIndex = (selectedTargetIndex - 1 + max) % max;

    SDL_Rect targetRect, oldRect = getRect("TargetSelector", "Battle");
    if (currentType == TargetType::Enemy || currentType == TargetType::AllEnemies)
    {
        while (!game.getBattle()->getEnemies().at(selectedTargetIndex).getIsAlive())
            selectedTargetIndex = (selectedTargetIndex - 1) % max;
        targetRect = getRect("Enemy" + std::to_string(selectedTargetIndex + 1), "Battle");
    }
    else if (currentType == TargetType::Ally || currentType == TargetType::AllAllies)
    {
        while (!player.getHeroes().at(selectedTargetIndex).getIsAlive())
            selectedTargetIndex = (selectedTargetIndex - 1) % max;
        targetRect = getRect("Hero" + std::to_string(selectedTargetIndex + 1), "Battle");
    }

    oldRect.x = targetRect.x + (targetRect.w / 2) - (oldRect.w / 2);
    oldRect.y = targetRect.y - targetRect.h;

    setRect("TargetSelector", "Battle", oldRect);
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

    // --- Map
    addImage("MapBackground", "Map", {-900, -500, 18000, 18000}, ResourceManager::getTexture("MapBackground"));
    addImage("Character", "Map", {screen.w/2 - 25, screen.h/2 - 25, 45, 45}, ResourceManager::getTexture("MovingCharacter"), {0,0,16,25});
    addImage("Press E", "Map", {screen.w/2 - 25, screen.h/2 - 25, 40, 40}, ResourceManager::getTextTexture("Press E", ResourceManager::getFont("RetroByte"), {0, 0,0,100}));
    addAnimation("MoveLeftAnimation", "Map", ResourceManager::getAnimation("MoveLeftAnimation"),  {0,0,45,45}, 0.2f);
    addAnimation("MoveRightAnimation", "Map", ResourceManager::getAnimation("MoveRightAnimation"), {0,0,45,45}, 0.2f);
    addAnimation("MoveUpAnimation", "Map", ResourceManager::getAnimation("MoveUpAnimation"), {0,0,45,45}, 0.2f);
    addAnimation("MoveDownAnimation", "Map", ResourceManager::getAnimation("MoveDownAnimation"),  {0,0,45,45}, 0.2f);

    setVisible("Press E", "Map", false);

    addMusic("MapTheme", "Map");

    // --- Online
    addMusic("OnlineTheme", "Online");
    addImage("OnlineBackground", "Online", screen, ResourceManager::getTexture("OptionsBackground"), {0,0,1920,1000});

    addImage("IPImage", "Online", {screen.w - 360, screen.h/2 - 25, 170, 50}, ResourceManager::getTextTexture("Your IP: " + game.getIP(), ResourceManager::getFont("RetroByte"), {0,0,0,0}));
    addButton("StartServerButton", "Online", {screen.w - 360,screen.h/2 +30,170,50},
        ResourceManager::getTextTexture("Start server", ResourceManager::getFont("RetroByte"), {0,0,0,0}),
        [this](){ game.startServer(); },
        [this](){ this->playSound("Menu","ButtonHover"); });

    addImage("ConnectImage", "Online", {190,screen.h/2 - 25,170,50}, ResourceManager::getTextTexture("Enter friend IP", ResourceManager::getFont("RetroByte"), {0,0,0,0}));
    addEdit("IPEdit", "Online", {190,screen.h/2 +30,170,50}, {0,0,0,0},
      ResourceManager::getTextTexture("1.1.1.1", ResourceManager::getFont("RetroByte"), {0,0,0,0}),
      [this](){ this->setFocus("IPEdit", "Online", true); },
      [this](){ this->playSound("Menu","ButtonHover"); });
    addButton("ConnectButton", "Online", {190,screen.h/2 + 85,170,50},
        ResourceManager::getTextTexture("Connect", ResourceManager::getFont("RetroByte"), {0,0,0,0}),
        [this](){ game.connectToServer(getText("IPEdit", "Online")); },
        [this](){ this->playSound("Menu","ButtonHover"); });


    // --- Menu
    addImage("MenuBackground", "Menu", screen, ResourceManager::getTexture("MenuBackground"));

    addMusic("MenuTheme", "Menu");
    addSound("ButtonHover", "Menu");
    addPanel("ButtonPanel", "Menu");

    addButton("Online", "Menu", {190,screen.h/2 +30,170,50},
        ResourceManager::getTextTexture("Co-op", ResourceManager::getFont("RetroByte"), {0,0,0,0}),
        [this](){ game.openOnlineMenu(); },
        [this](){ this->playSound("Menu","ButtonHover"); },
        "ButtonPanel");
    addButton("Continue", "Menu",{190, screen.h/2 + 80, 170, 50},
        ResourceManager::getTextTexture("Continue", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() { game.startGame();},
        [this]() {this->playSound("Menu", "ButtonHover");},
        "ButtonPanel");
    addButton("New game", "Menu",{190, screen.h/2 + 130, 170, 50},
        ResourceManager::getTextTexture("New game", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() { game.startGame();},
        [this]() {this->playSound("Menu", "ButtonHover");},
        "ButtonPanel");
    addButton("Quit", "Menu",{190, screen.h/2 + 230, 100, 50},
        ResourceManager::getTextTexture("Quit", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {game.endGame(); },
        [this]() {this->playSound("Menu", "ButtonHover");},
        "ButtonPanel");
    addButton("Options", "Menu", {190, screen.h/2 + 180, 150, 50},
        ResourceManager::getTextTexture("Options", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->game.openOptions(GameState::Menu);},
        [this]() {this->playSound("Menu", "ButtonHover");},
        "ButtonPanel");

    // --- Options
    std::string optionsText = "Options:\nOn Map:\nFor moving use WASD or arrows\nIn Battle:\nTo select target use WASD or arrows\nTo confirm the taget use SPACE or ENTER(RETURN)";
    addImage("OptionsBackground", "Options", screen, ResourceManager::getTexture("OptionsBackground"), {0,0,1920,1000});
    addImage("InstructionText", "Options", {screen.w/2 - 350,screen.h/2 - 150,700, 240},
         ResourceManager::getTextTexture(optionsText, ResourceManager::getFont("RetroByte"), {0, 0, 0,0}, screen.w - 200));
    addButton("BackButton", "Options", {screen.w/2 - 110,screen.h - 100, 220,90},
         ResourceManager::getTextTexture("Back to menu", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
         [this]() {this->game.closeOptions();},
         [this]() {this->playSound("Menu", "ButtonHover");});
    addMusic("OptionsTheme", "Options");

    // --- Battle
    addImage("BattleBackground", "Battle", {0, 0, screen.w, 452}, ResourceManager::getTexture("BattleImage0"));
    addImage("DownMenuBackground", "Battle", {0, 452, screen.w, screen.h - 452}, ResourceManager::getTexture("BattleTile"));

    addPanel("ActionPanel", "Battle");

    addPanel("EnvironmentPanel", "Battle");

    addPanel("EnemyPanel", "Battle");
    addPanel("HeroPanel", "Battle");

    addMusic("BattleTheme", "Battle");

    addButton("AttackButton", "Battle", {0, 453, 60, 25},
        ResourceManager::getTextTexture("Attack", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->onActionButton(ActionType::Attack);},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");
    addButton("ItemsButton", "Battle", {0, 478, 60, 25},
        ResourceManager::getTextTexture("Items", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->onActionButton(ActionType::Item);},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");
    addButton("MagicButton", "Battle", {0, 503, 60, 25},
        ResourceManager::getTextTexture("Magic", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->onActionButton(ActionType::Magic);},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");
    addButton("Run", "Battle", {0, 528, 60, 20},
        ResourceManager::getTextTexture("Run", ResourceManager::getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->game.endRandomBattle();},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");

    addImage("TargetSelector", "Battle", {0,0,100,100}, ResourceManager::getTexture("TargetSelector"), {0,0,100,100});
    setVisible("TargetSelector", "Battle", false);

    addImage("ActorSelector", "Battle", {0,0,50,50}, ResourceManager::getTexture("ActorSelector"), {0,0,100,100});
    setVisible("ActorSelector", "Battle", false);

    for (int i = 0; i < 4; i++)
    {
        addImage("Enemy" + std::to_string(i + 1), "Battle", {screen.w/2 - 100, screen.h/2 - 230 + (i * 100), 76, 94}, nullptr, {0, 0, 108, 144}, "EnemyPanel");
        addImage("Hero" + std::to_string(i + 1), "Battle", {screen.w - 100, screen.h/2 - 230 + (i * 100), 56, 74}, nullptr, {0, 0, 16, 24}, "HeroPanel");
    }

    for (int i = 0; i < ENV_BUTTON_COUNT/2; i+=2)
    {
        addButton("EnButton" + std::to_string(i + 1), "Battle", {60 + i * 60,452,60,25}, nullptr, nullptr, nullptr, "EnvironmentPanel");
        addButton("EnButton" + std::to_string(i + 2), "Battle", {60 + i * 60, 452, 60,25}, nullptr, nullptr, nullptr, "EnvironmentPanel");
    }
    for (int i = 8; i < ENV_BUTTON_COUNT; i+=2)
    {
        addButton("EnButton" + std::to_string(i + 1), "Battle", {60 + i * 60,480,60,25}, nullptr, nullptr, nullptr, "EnvironmentPanel");
        addButton("EnButton" + std::to_string(i + 2), "Battle", {60 + i * 60,480,60,25}, nullptr, nullptr, nullptr, "EnvironmentPanel");
    }
    setEnVI("EnvironmentPanel", "Battle", false);

    addImage("HerosStatus", "Battle",{screen.w - 600, 452, 300, screen.h - 452} , nullptr, {0, 0, 300, screen.h - 452});
    addImage("EnemiesStatus","Battle", {screen.w - 300, 452, 300, screen.h - 452}, nullptr, {0, 0, 300, screen.h - 452});


    for (int i = 0; i < player.getHeroes().size(); i++)
    {
        addAnimation("Hero" + std::to_string(i + 1) + "Animation", "Battle", ResourceManager::getAnimation("Hero" + std::to_string(i + 1) + "Animation"),
            {{screen.w - 100,screen.h/2 - 230 + (i * 100),56,94}, {screen.w - 150,screen.h/2 - 230 + (i * 100),56,94}, {screen.w - 100,screen.h/2 - 230 + (i * 100),56,94}}, 0.2f);
    }


    addImage("FPS", "Battle", {10,10,100,30},
        ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    addImage("FPS", "Map", {10,10,100,30},
            ResourceManager::getTextTexture("FPS: " + std::to_string(game.getCurrentFPS()), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));


    addImage("Time", "Menu",  {screen.w - 90, 10, 80, 30},
        ResourceManager::getTextTexture(game.getCurrentTime(), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    addImage("Time", "Battle",  {screen.w - 90, 10, 80, 30},
        ResourceManager::getTextTexture(game.getCurrentTime(), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    addImage("Time", "Map",  {screen.w - 90, 10, 80, 30},
        ResourceManager::getTextTexture(game.getCurrentTime(), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    addImage("Time", "Options",  {screen.w - 90, 10, 80, 30},
        ResourceManager::getTextTexture(game.getCurrentTime(), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
    addImage("Time", "Online",  {screen.w - 90, 10, 80, 30},
        ResourceManager::getTextTexture(game.getCurrentTime(), ResourceManager::getFont("RetroByte"), {0, 0, 0, 0}));
}

void UIManager::update(float dt)
{
    auto sit = scenes.find(gameStateString.at(game.getGameState()));
    if (sit == scenes.end())
        return;

    if (gameStateString.at(game.getGameState()) == "Map")
        updatePlayerAnimation(sit->second.animPlayer, player);

    Scene& scene = sit->second;
    scene.animPlayer.update(dt);
}
