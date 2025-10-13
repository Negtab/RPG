#include "uiManager.h"

#include "game.h"
#include "types.h"


void UIManager::addScene(const std::string& id)
{
    scenes[id] = Scene();
}

void UIManager::addButton(const std::string& id, const std::string& sceneId,const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick)
{
    scenes[sceneId].buttons[id] = {rect, texture, onClick};
}
void UIManager::addImage(const std::string& id, const std::string& sceneId, const SDL_Rect& rect, SDL_Texture* texture)
{
    scenes[sceneId].images[id] = {rect, texture};
}
void UIManager::addMusic(const std::string& id, const std::string& sceneId)
{
    scenes[sceneId].musicPlayer.music[id] = resourceManager.getMusic(id);
}

void UIManager::drawScene(const std::string& sceneId)
{
    const std::string& music = sceneId + "Theme";
    if (sceneId == "Map")
    {
        scenes[sceneId].images["Map"].rect.x += player.getPlayerCoords().x;
        scenes[sceneId].images["Map"].rect.y += player.getPlayerCoords().y;
    }
    for (auto& [_, im] : scenes[sceneId].images)
        visualizer.drawTexture(im.texture, im.rect.x, im.rect.y, im.rect.w, im.rect.h);
    for (auto& [_, btn] : scenes[sceneId].buttons)
        visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
    if (music != "")
        playMusic(sceneId, music);
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


void UIManager::handleEvent(const SDL_Event& event, const std::string& sceneId)
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
    addImage("Map", "Map", {-900, -500, 18000, 18000}, resourceManager.getTexture("MapBackground"));
    addImage("Menu", "Menu", screen, resourceManager.getTexture("MenuBackground"));
    addMusic("MenuTheme", "Menu");
    addMusic("MapTheme", "Map");
    addButton("Continue", "Menu", {screen.w/2 - 250/2, screen.h/2 - 100/2, 170, 50}, resourceManager.getTexture("ContinueButton"),  [this]() { game.startGame(); });
    addButton("Quit", "Menu", {screen.w/2 - 250/2, screen.h/2 - 100/2 + 100, 100, 50}, resourceManager.getTexture("QuitButton"), [this]() {game.endGame(); });
}
