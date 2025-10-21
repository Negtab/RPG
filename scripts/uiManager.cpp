#include "uiManager.h"

#include <utility>

#include "game.h"
#include "types.h"


void UIManager::addScene(const std::string& id)
{
    scenes[id] = Scene();
}

void UIManager::addPanel(const std::string& id, const std::string& sceneId)
{
    //scenes[id].panels[id] = ;
}

void UIManager::addButton(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick, std::function<void()> onHover)
{
    scenes[sceneId].order.push_back(id);
    scenes[sceneId].buttons[id] = {rect, texture, std::move(onClick), std::move(onHover)};
}
void UIManager::addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture, SDL_Rect srect)
{
    scenes[sceneId].order.push_back(id);
    scenes[sceneId].images[id] = {rect, texture, srect};
}

void UIManager::addLabel(const std::string& id, const std::string& sceneId, const SDL_Rect& textRect, const SDL_Rect& imageRect, SDL_Texture* text, SDL_Texture* texture, SDL_Rect srect)
{
    scenes[sceneId].order.push_back(id);
    scenes[sceneId].labels[id] = {textRect, text, imageRect, texture, srect};
}

void UIManager::addMusic(const std::string& id, const std::string& sceneId)
{
    scenes[sceneId].musicPlayer.music[id] = resourceManager.getMusic(id);
}

void UIManager::addSound(const std::string& id, const std::string& sceneId)
{
    scenes[sceneId].musicPlayer.sound[id] = resourceManager.getSound(id);
}

void UIManager::drawPanel(const Panel& panel)
{
    for (const auto& key : panel.order) {
        if (auto it = panel.images.find(key); it != panel.images.end()) {
            const auto& im = it->second;
            if (im.srect.w == 0 || im.srect.h == 0)
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h);
            else
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h, im.srect.x, im.srect.y, im.srect.w, im.srect.h);

            continue;
        }
        if (auto it = panel.buttons.find(key); it != panel.buttons.end())
        {
            const auto& btn = it->second;
            visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
            continue;
        }
        if (auto it = panel.labels.find(key); it != panel.labels.end())
        {
            const auto& lbl = it->second;
            if (lbl.image.srect.w == 0 || lbl.image.srect.h == 0)
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h);
            else
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h, lbl.image.srect.x, lbl.image.srect.y, lbl.image.srect.w, lbl.image.srect.h);
            visualizer.drawTexture(lbl.texture, lbl.rect.x, lbl.rect.y, lbl.rect.w, lbl.rect.h);
            continue;
        }
        SDL_Log("UIManager: element '%s' not found in panel", key.c_str());
    }
}

void UIManager::drawScene(const std::string& sceneId)
{
    Scene& scene = scenes[sceneId];
    const std::string music = sceneId + "Theme";

    if (sceneId == "Map")
    {
        scene.images["Map"].rect.x = player.getPlayerCoords().x - 900;
        scene.images["Map"].rect.y = player.getPlayerCoords().y - 500;
    }

    for (const auto& key : scene.order) {
        if (auto it = scene.images.find(key); it != scene.images.end()) {
            const auto& im = it->second;
            if (im.srect.w == 0 || im.srect.h == 0)
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h);
            else
                visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h, im.srect.x, im.srect.y, im.srect.w, im.srect.h);

            continue;
        }
        if (auto it = scene.buttons.find(key); it != scene.buttons.end())
        {
            const auto& btn = it->second;
            visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
            continue;
        }
        if (auto it = scene.labels.find(key); it != scene.labels.end())
        {
            const auto& lbl = it->second;
            if (lbl.image.srect.w == 0 || lbl.image.srect.h == 0)
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h);
            else
                visualizer.drawTexture(lbl.image.texture, lbl.image.rect.x, lbl.image.rect.y, lbl.image.rect.w, lbl.image.rect.h, lbl.image.srect.x, lbl.image.srect.y, lbl.image.srect.w, lbl.image.srect.h);
            visualizer.drawTexture(lbl.texture, lbl.rect.x, lbl.rect.y, lbl.rect.w, lbl.rect.h);
            continue;
        }
        if (auto it = scene.panels.find(key); it != scene.panels.end())
        {
            const auto& pan = it->second;
            drawPanel(pan);
        }

        SDL_Log("UIManager: element '%s' not found in scene '%s'", key.c_str(), sceneId.c_str());
    }

    if (!music.empty())
        playMusic(sceneId, music);
}

void UIManager::playSound(const std::string& sceneId, const std::string& soundId)
{
    auto sceneIt = scenes.find(sceneId);
    if (sceneIt == scenes.end())
    {
        SDL_Log("Scene '%s' not found", sceneId.c_str());
        return;
    }

    auto& scene = sceneIt->second;
    auto soundIt = scene.musicPlayer.sound.find(soundId);
    if (soundIt == scene.musicPlayer.sound.end())
    {
        SDL_Log("Music '%s' not found in scene '%s'", soundId.c_str(), sceneId.c_str());
        return;
    }

    Mix_Chunk* sound = soundIt->second;

    if (scene.musicPlayer.currentSoundId == soundId && Mix_Playing(scene.musicPlayer.chanel))
        return;

    if (Mix_Playing(scene.musicPlayer.chanel))
        visualizer.stopSound(scene.musicPlayer.chanel);

    visualizer.playSound(scene.musicPlayer.chanel, sound);
    scene.musicPlayer.currentSoundId = soundId;
}

void UIManager::playMusic(const std::string& sceneId, const std::string& musicId)
{
    auto sceneIt = scenes.find(sceneId);
    if (sceneIt == scenes.end())
    {
        SDL_Log("Scene '%s' not found", sceneId.c_str());
        return;
    }

    auto& scene = sceneIt->second;
    auto musicIt = scene.musicPlayer.music.find(musicId);
    if (musicIt == scene.musicPlayer.music.end())
    {
        SDL_Log("Music '%s' not found in scene '%s'", musicId.c_str(), sceneId.c_str());
        return;
    }

    Mix_Music* music = musicIt->second;

    // Если уже играет эта же композиция — ничего не делаем
    if (scene.musicPlayer.currentMusicId == musicId && Mix_PlayingMusic())
        return;

    // Если что-то играет — остановим через Visualizer
    if (Mix_PlayingMusic())
        visualizer.stopMusic();

    // Запускаем новую
    visualizer.playMusic(music);
    scene.musicPlayer.currentMusicId = musicId;
}


void UIManager::handleClickEvent(const SDL_Event& event, const std::string& sceneId)
{
    if (event.type == SDL_MOUSEBUTTONDOWN)
    {
        int x = event.button.x;
        int y = event.button.y;

        for (auto& [_, btn] : scenes[sceneId].buttons)
        {
            if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
                y >= btn.rect.y && y <= btn.rect.y + btn.rect.h)
            {
                if (btn.onClick) btn.onClick();
                break;
            }
        }
    }
}

void UIManager::handleHoverEvent(const SDL_Event& event, const std::string& sceneId)
{
    if (event.type == SDL_MOUSEMOTION)
    {
        int x = event.button.x;
        int y = event.button.y;

        for (auto& [_, btn] : scenes[sceneId].buttons)
        {
            if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
                y >= btn.rect.y && y <= btn.rect.y + btn.rect.h)
            {
                if (btn.onHover) btn.onHover();
                break;
            }
        }
    }
}

void UIManager::changePanelOrder(const std::string& sceneId, const std::string& panelId, const int& newOrder)
{
    auto scene = scenes[sceneId];
    if (auto it = scene.labels.find(panelId); it != scene.labels.end())
        for (int i = 0; i < scene.order.size(); i++)
            if (scene.order[i] == panelId)
            {
                if (i == newOrder) return;
                if (i < newOrder)
                {
                    int j;
                    for (j = newOrder + 1; j < i - 1; j++)
                        std::swap(scene.order[j], scene.order[j + 1]);
                    std::swap(scene.order[i], scene.order[j]);
                }
                if (i > newOrder)
                {
                    int j;
                    for (j = i + 1; j < newOrder - 1; j++)
                        std::swap(scene.order[j], scene.order[j + 1]);
                    std::swap(scene.order[i], scene.order[j]);
                }
                return;
            }

}

void UIManager::addEnemys(const Game& game)
{
    //std::vector<std::string> enemies = game;
}

void UIManager::initialize()
{
    addScene(gameStateToString(GameState::Battle));
    addScene(gameStateToString(GameState::Instruction));
    addScene(gameStateToString(GameState::Menu));
    addScene(gameStateToString(GameState::CreatePlayer));
    addScene(gameStateToString(GameState::Map));
    addScene(gameStateToString(GameState::Inventory));

    screen.x = 0;//-900;
    screen.y = 0;//-500;
    screen.w = 900;//18000;
    screen.h = 600;//18000;
    // --- Map
    addImage("Map", "Map", {-900, -500, 18000, 18000}, resourceManager.getTexture("MapBackground"));
    addImage("Character", "Map", {screen.w/2 - 25, screen.h/2 - 25, 28, 28}, resourceManager.getTexture("MovingCharacter"), {0,0,48,48});
    addMusic("MapTheme", "Map");
    // --- Menu
    addImage("Menu", "Menu", screen, resourceManager.getTexture("MenuBackground"));
    addMusic("MenuTheme", "Menu");
    addSound("ButtonHover", "Menu");
    addButton("Continue", "Menu",
        {screen.w/2 - 250/2, screen.h/2 - 100/2, 170, 50}, resourceManager.getTextTexture("Continue", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() { game.startGame();},
        [this]() {this->playSound("Menu", "ButtonHover");} );
    addButton("Quit", "Menu",
        {screen.w/2 - 250/2, screen.h/2 - 100/2 + 100, 100, 50}, resourceManager.getTextTexture("Quit", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {game.endGame(); },
        [this]() {this->playSound("Menu", "ButtonHover");} );
    // --- Battle
    addMusic("BattleTheme", "Battle");
    addImage("BattleBackground", "Battle", {0, 0, 732, 452}, resourceManager.getTexture("BattleImage0"));
    addImage("Action", "Battle", {0, 453, 150, screen.h - 452}, resourceManager.getTexture("BattleTile"));
    addImage("Dop Action", "Battle", {150, 453, 732 - 150, screen.h - 452}, resourceManager.getTexture("BattleTile"));
    addImage("Book", "Battle", {732, 453, screen.w - 732, screen.h - 452}, resourceManager.getTexture("BattleTile"));
    addImage("Characters", "Battle", {732, 0, screen.w - 732, 452}, resourceManager.getTexture("BattleTile"));
    addButton("Attack", "Battle", {0, 453, 150, screen.h - 452}, resourceManager.getTextTexture("Attack", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->changePanelOrder("Battle", "Attack", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");});
    addButton("Items", "Battle", {0, 453, 150, screen.h - 452}, resourceManager.getTextTexture("Items", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->changePanelOrder("Battle", "Items", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");});
    addButton("Magic", "Battle", {0, 453, 150, screen.h - 452}, resourceManager.getTextTexture("Magic", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->changePanelOrder("Battle", "Magic", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");});
    addButton("Run", "Battle", {0, 453, 150, screen.h - 452}, resourceManager.getTextTexture("Run", resourceManager.getFont("RetroByte"), {0, 0, 0,0}),
        [this]() {this->changePanelOrder("Battle", "Run", 0);},
        [this]() {this->playSound("Battle", "ButtonHover");});
}

