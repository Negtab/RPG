#ifndef PROJECT_NAME_GAME_H
#define PROJECT_NAME_GAME_H

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "../controllers/resourceManager.h"
#include "../controllers/visualizer.h"

#include "../gameLogic/battle.h"
#include "../gameLogic/quest.h"
#include "../controllers/inputManager.h"
#include "../gameLogic/notGamePerson.h"

#include "player.h"
#include "types.h"
#include "uiManager.h"
#include "inputController.h"

#include "../online/client.h"
#include "../online/server.h"
#include "../online/networkPackets.h"

struct PlayerMovePacket;
class InputController;

class Game {
    public:
        explicit Game(SDL_Window* window, SDL_Renderer* renderer);
        ~Game() = default;

        void run();

        void addPlayer(Player&& player);
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

        Item &getItem(const int &index);
        Skill& getSkill(const int &id);

        void startGame();
        void endGame();

        void openOptions(const GameState &prevGame);
        void closeOptions();

        void openOnlineMenu();
        void closeOnlineMenu();

        void startRandomBattle();
        void endRandomBattle();

        float getDeltaTime();
        void updateFPS();
        [[nodiscard]] int getCurrentFPS() const;
        [[nodiscard]] std::string getCurrentTime() const;

        [[nodiscard]] std::string getIP() const;
        void connectToServer(const std::string &ip);
        void startServer();

        void sendLocalPlayerState();

        [[nodiscard]] const std::unordered_map<uint32_t, Player>& getRemotePlayers() const noexcept;
    private:

        void handleInput(const SDL_Event &event);
        void update(float dt);
        void render();

        void initItems();
        void initSkills();

        void updateNetwork();

        void processMovePacket(const PlayerMovePacket& packet);

        bool isRunning = false;

        Uint64 lastCounter = 0;     // для deltaTime
        Uint64 fpsLastCounter = 0;  // для подсчёта FPS
        int frameCount = 0;         // сколько кадров прошло с последнего замера
        int currentFPS = 0;         // реальный FPS для отображения
        float targetFPS = 120.0f;    // фиксированный FPS

        SDL_Rect screen = {0, 0, 900, 600};
        GameState state{GameState::CreatePlayer};
        GameState prevState{GameState::CreatePlayer};

        std::unordered_map<uint32_t, Player> remotePlayers;
        uint32_t localPlayerId {1};

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

        Client client;
        Server server;

        SDL_Window* window;
        SDL_Renderer* renderer;
    };

#endif // PROJECT_NAME_GAME_H