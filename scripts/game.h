#ifndef PROJECT_NAME_GAME_H
#define PROJECT_NAME_GAME_H

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "item.h"
#include "quest.h"
#include "gamePerson.h"
#include "player.h"
#include "resourceManager.h"
#include "visualizer.h"
#include "inputController.h"
#include "inputManager.h"
#include "types.h"
#include "uiManager.h"

class Game
{
public:
    explicit Game(SDL_Renderer* renderer);
    ~Game() = default;

    void run(); // главный игровой цикл

    void addPlayer(const Player& player);
    void addQuest(const Quest& quest);
    void addItem(const Item& item);

    void setGameState(GameState state);
    void setPreviousGameState(GameState state);
    [[nodiscard]] GameState getGameState() const;
    [[nodiscard]] GameState getPrevGameState() const;
private:
    void handleInput(const SDL_Event &event);
    void update();
    void render();

    GameState state{GameState::CreatePlayer};
    GameState prevState{GameState::CreatePlayer};

    std::map<int, Item> items;
    std::map<int, Skill> skills;
    std::vector<Location> locations;
    std::vector<Player> players;
    std::vector<NotGamePerson> npss;
    std::vector<Quest> quests;
    std::string pathToSave;

    // Менеджеры
    std::unique_ptr<ResourceManager> resourceManager;

    std::unique_ptr<Visualizer> visualizer;
    std::unique_ptr<UIManager> uiManager;

    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<InputController> inputController;

    SDL_Renderer* renderer;
};

#endif // PROJECT_NAME_GAME_H