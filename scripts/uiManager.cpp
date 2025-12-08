#include <SDL_log.h>

#include <utility>

#include "uiManager.h"
#include "game.h"

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
            visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
            continue;
        }

        auto itLbl = panel.labels.find(key);
        if (itLbl != panel.labels.end()) {
            const auto& lbl = itLbl->second;
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

    if (panelId == "")
        scenes[sceneId].addButtonLocal(id, rect, texture, std::move(onClick), std::move(onHover));
    else if (scenes[sceneId].panels.contains(panelId))
        scenes[sceneId].panels[panelId].addButtonLocal(id, rect, texture, std::move(onClick), std::move(onHover));
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

    if (panelId == "")
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

    if (panelId == "")
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
    Mix_Music* m = resourceManager.getMusic(musicId);
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
    Mix_Chunk* c = resourceManager.getSound(soundId);
    if (!c)
    {
        SDL_Log("addSound: resource '%s' not found", soundId.c_str());
        return;
    }
    scenes[sceneId].musicPlayer.sound[soundId] = c;
}

void UIManager::addAnimation(const std::string &id, const std::string &sceneId, std::vector<SDL_Texture*> frames, std::vector<SDL_Rect*> rect, const float &time)
{
    if (!scenes.contains(sceneId)) {
        SDL_Log("addLabel: scene '%s' not found.", sceneId.c_str());
        return;
    }

    scenes[sceneId].animPlayer.animations[id] = {time, std::move(frames), std::move(rect)};
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

    if (sceneId == "Map")
    {
        auto it = scene.images.find("Map");
        if (it != scene.images.end()) {
            it->second.rect.x = player.getPlayerCoords().x - 900;
            it->second.rect.y = player.getPlayerCoords().y - 500;
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

    if (!music.empty())
        scene.playMusicLocal(visualizer, music);
}

void UIManager::addEnemys()
{
    std::vector<Enemy> enemies = this->game.getBattle()->getEnemies();
    for (int i = 0; i < 4; i++)
        setTexture(("Enemy" + std::to_string(i + 1)), "Battle", resourceManager.getTexture(enemies.at(i).getName()));
}

void UIManager::addCharacters()
{
    for (int i = 0; i < MAX_HERO_COUNT; i++)
        setTexture(("Hero" + std::to_string(i + 1)), "Battle", resourceManager.getTexture(("Image" + std::to_string(i + 1))));
}

void UIManager::setEnabled(const std::string &id, const std::string &sceneId, const bool &enabled)
{
    auto obj = findUIObject(id, sceneId);
    if (obj != nullptr)
        obj->isEnabled = enabled;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setEnVI(const std::string &id, const std::string &sceneId, const bool &enabled)
{
    auto obj = findUIObject(id, sceneId);
    if (obj != nullptr) {
        obj->isEnabled = enabled;
        obj->isVisible = enabled;
    }
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}


void UIManager::setVisible(const std::string &id, const std::string &sceneId, const bool &visible)
{
    auto obj = findUIObject(id, sceneId);
    if (obj != nullptr)
    {
        obj->isVisible = visible;
        obj->isEnabled = visible;
    }
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setRect(const std::string &id, const std::string &sceneId, const SDL_Rect &rect)
{
    auto obj = findUIObject(id, sceneId);
    if (obj != nullptr)
        findUIObject(id, sceneId)->rect = rect;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::setTexture(const std::string &id, const std::string &sceneId, SDL_Texture *texture)
{
    auto obj = findUIObject(id, sceneId);
    if (obj != nullptr)
        findUIObject(id, sceneId)->texture = texture;
    else
        SDL_Log("Object '%s' not found %s", id.c_str(), sceneId.c_str());
}

void UIManager::onClickAttack()
{
    game.getBattle()->setState(BattleState::Animation);
    game.getBattle()->setChoose(true);
}

void UIManager::onClickMagic() {
    game.getBattle()->setState(BattleState::SelectSkill);
    setEnVI("MagicPanel","Battle", true);
    setEnVI("InventoryPanel","Battle", false);
}

void UIManager::onClickItem() {
    game.getBattle()->setState(BattleState::SelectItem);
    setEnVI("MagicPanel","Battle", false);
    setEnVI("InventoryPanel","Battle", true);
}

void UIManager::onChooseSkill(int skillId)
{
    setEnVI("SelectEnemy", "Battle", true);
}

void UIManager::onChooseEnemy() {
    game.getBattle()->setState(BattleState::SelectTarget);
    setEnVI("Selector", "Battle", true);

}


void UIManager::initialize()
{
    SDL_Rect screen = game.getScreenRect();
    addScene(gameStateToString(GameState::Battle));
    addScene(gameStateToString(GameState::Instruction));
    addScene(gameStateToString(GameState::Menu));
    addScene(gameStateToString(GameState::CreatePlayer));
    addScene(gameStateToString(GameState::Map));
    addScene(gameStateToString(GameState::Inventory));

    // --- Map
    addImage("Map", "Map", {-900, -500, 18000, 18000}, resourceManager.getTexture("MapBackground"));
    addImage("Character", "Map", {screen.w/2 - 25, screen.h/2 - 25, 45, 45}, resourceManager.getTexture("MovingCharacter"), {0,0,25,25});
    addImage("Press E", "Map", {screen.w/2 - 25, screen.h/2 - 25, 40, 40}, resourceManager.getTextTexture("Press E", resourceManager.getFont("RetroByte"), {0, 0,0,100}));

    setVisible("Press E", "Map", false);

    addMusic("MapTheme", "Map");
    // --- Menu
    addImage("Menu", "Menu", screen, resourceManager.getTexture("MenuBackground"));

    addMusic("MenuTheme", "Menu");
    addSound("ButtonHover", "Menu");
    addPanel("ButtonPanel", "Menu");

    addButton("Continue", "Menu",{screen.w/2 - 250/2, screen.h/2 - 100/2, 170, 50},
        resourceManager.getTextTexture("Continue", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() { game.startGame();},
        [this]() {this->playSound("Menu", "ButtonHover");},
        "ButtonPanel");
    addButton("Quit", "Menu",{screen.w/2 - 250/2, screen.h/2 - 100/2 + 100, 100, 50},
        resourceManager.getTextTexture("Quit", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {game.endGame(); },
        [this]() {this->playSound("Menu", "ButtonHover");},
        "ButtonPanel");
    // --- Battle

    addImage("BattleBackground", "Battle", {0, 0, 732, 452}, resourceManager.getTexture("BattleImage0"));
    addImage("DownMenuBackground", "Battle", {0, 453, 732, screen.h - 452}, resourceManager.getTexture("BattleTile"));
    /*addImage("DopAction", "Battle", {150, 453, 732 - 150, screen.h - 452}, resourceManager.getTexture("BattleTile"));
    addImage("Book", "Battle", {732, 453, screen.w - 732, screen.h - 452}, resourceManager.getTexture("BattleTile"));
    addImage("Characters", "Battle", {732, 0, screen.w - 732, 452}, resourceManager.getTexture("BattleTile"));*/

    addPanel("ActionPanel", "Battle");

    addPanel("CharacterStPanel", "Battle");
    addPanel("EnemyStPanel", "Battle");

    addPanel("InventoryPanel", "Battle");
    addPanel("MagicPanel", "Battle");

    addPanel("EnemyPanel", "Battle");
    addPanel("HeroPanel", "Battle");

    addMusic("BattleTheme", "Battle");

    addButton("AttackButton", "Battle", {0, 453, 60, 25},
        resourceManager.getTextTexture("Attack", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->scenes["Battle"].changePanelOrder("Attack", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");
    addButton("ItemsButton", "Battle", {0, 478, 60, 25},
        resourceManager.getTextTexture("Items", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->scenes["Battle"].changePanelOrder("Items", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");
    addButton("MagicButton", "Battle", {0, 503, 60, 25},
        resourceManager.getTextTexture("Magic", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->scenes["Battle"].changePanelOrder("Magic", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");

    for (int i = 0; i < 4; i++)
        addImage(("Enemy" + std::to_string(i + 1)), "Battle", {screen.w/2 - 100, screen.h/2 - 230 + (i * 100), 76, 94}, nullptr, {0, 0, 108, 144}, "EnemyPanel");
    for (int i = 0; i < 4; i++)
        addImage(("Hero" + std::to_string(i + 1)), "Battle", {screen.w/2 + 140, screen.h/2 - 230 + (i * 100), 56, 74}, nullptr, {0, 0, 16, 24}, "HeroPanel");


    addButton("Run", "Battle", {0, 528, 60, 20},
        resourceManager.getTextTexture("Run", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->game.endRandomBattle();},
        [this]() {this->playSound("Battle", "ButtonHover");},
        "ActionPanel");


}