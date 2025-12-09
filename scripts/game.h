#ifndef PROJECT_NAME_GAME_H
#define PROJECT_NAME_GAME_H

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "gameLogic/battle.h"
#include "gameLogic/item.h"
#include "gameLogic/quest.h"
#include "player.h"
#include "controllers/resourceManager.h"
#include "controllers/visualizer.h"
#include "inputController.h"
#include "controllers/inputManager.h"
#include "gameLogic/notGamePerson.h"
#include "types.h"
#include "uiManager.h"

    class Game {
    public:
        explicit Game(SDL_Renderer* renderer);
        ~Game() = default;

        void run();

        void addPlayer(const Player& player);
        void addQuest(const Quest& quest);
        void addItem(const Item& item);

        void setGameState(GameState state);
        void setPreviousGameState(GameState state);
        [[nodiscard]] GameState getGameState() const;
        [[nodiscard]] GameState getPrevGameState() const;

        void addLocation(const Location &location);
        [[nodiscard]] std::vector<Location> getLocations() const;

        [[nodiscard]] Battle *getBattle() const;

        void setScreenRect(const SDL_Rect *rect);
        [[nodiscard]] SDL_Rect getScreenRect() const;

        void startGame();
        void endGame();

        void startRandomBattle();
        void endRandomBattle();
    private:
        void handleInput(const SDL_Event &event);
        void update();
        void render();



        bool isRunning = false;
        SDL_Rect screen = {0, 0, 900, 600};
        GameState state{GameState::CreatePlayer};
        GameState prevState{GameState::CreatePlayer};

        std::map<int, Item> items;
        std::map<int, Skill> skills;
        std::vector<Location> locations;
        std::vector<Player> players;
        std::vector<NotGamePerson> npss;
        std::vector<Quest> quests;
        std::string pathToSave;

        std::unique_ptr<Battle> battle;
        std::unique_ptr<ResourceManager> resourceManager;
        std::unique_ptr<Visualizer> visualizer;
        std::unique_ptr<UIManager> uiManager;
        std::unique_ptr<InputManager> inputManager;
        std::unique_ptr<InputController> inputController;

        SDL_Renderer* renderer;
    };

#endif // PROJECT_NAME_GAME_H