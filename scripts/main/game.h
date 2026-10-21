#ifndef PROJECT_NAME_GAME_H
#define PROJECT_NAME_GAME_H

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
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
#include "../ui/uiManager.h"
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

        [[nodiscard]] Battle *getBattle() const;

        void setScreenRect(const SDL_Rect *rect);
        [[nodiscard]] SDL_Rect getScreenRect() const;

        Item &getItem(const int &index);
        Skill& getSkill(const int &id);

        void startGame();
        void continueGame();                          // вернуться в игру или загрузить последнее сохранение
        void endGame();

        // ── Игровое меню (Esc на карте) ──
        void openGameMenu();
        void closeGameMenu();
        void returnToTitle();                         // автосохранение и выход на стартовый экран
        [[nodiscard]] std::string currentLocation() const;
        [[nodiscard]] uint32_t getPlaySeconds() const { return static_cast<uint32_t>(playSeconds); }
        [[nodiscard]] const Player& getLocalPlayer() const { return players.at(0); }
        void loadSettings();                          // settings.ini: громкость, бои, мини-карта
        void saveSettings() const;

        [[nodiscard]] bool encountersEnabled() const { return encounters; }
        void setEncountersEnabled(bool on) { encounters = on; }
        // Мини-карта: 0 — выключена, 1..3 — приближение (1 — вся карта)
        static constexpr int kMinimapZoomMax = 3;
        [[nodiscard]] bool minimapEnabled() const { return minimapZoom > 0; }
        [[nodiscard]] int getMinimapZoom() const { return minimapZoom; }
        void setMinimapZoom(int zoom) { minimapZoom = std::clamp(zoom, 0, kMinimapZoomMax); }
        void cycleMinimap();                          // M: Off → x1 → x2 → x3 → Off
        void toggleEncounters();                      // H: с подсказкой на карте

        // ── Сохранения: слот 0 — автосохранение при выходе, 1..4 — ручные ──
        static constexpr int kSaveSlots = 5;
        void openSaveScreen(bool saving);
        void closeSaveScreen();
        bool saveToSlot(int slot);
        bool loadFromSlot(int slot);
        [[nodiscard]] std::optional<SaveSummary> getSlotSummary(int slot) const;
        [[nodiscard]] bool canSave() const;
        [[nodiscard]] bool canLoad() const;
        [[nodiscard]] bool canContinue() const { return gameInProgress || latestSaveSlot >= 0; }
        [[nodiscard]] bool isGameInProgress() const { return gameInProgress; }

        void openOptions(const GameState &prevGame);
        void closeOptions();

        void openOnlineMenu();
        void closeOnlineMenu();

        void endRandomBattle();

        float getDeltaTime();
        void updateFPS();
        [[nodiscard]] int getCurrentFPS() const;
        static std::string getCurrentTime() ;

        static std::string getIP() ;
        void connectToServer(const std::string &ip);
        void startServer();

        void sendLocalPlayerState();

        [[nodiscard]] const std::unordered_map<uint32_t, Player>& getRemotePlayers() const noexcept;

        void sendBattleAction(const Action& action);
        void sendBattleStart();
        void sendBattleSnapshot();

        bool isHost() const { return server.isRunning(); }
        bool isConnected() const { return client.isConnected(); }
        int getPlayerCount() const { return 1 + remotePlayers.size(); }

        std::unordered_map<uint32_t, Player>& getRemotePlayersMutable() { return remotePlayers; }

        bool battleStarted = false;
        uint32_t localPlayerId {0};

        [[nodiscard]] const Item*  findItem(int id) const;
        [[nodiscard]] const Skill* findSkill(int id) const;

        [[nodiscard]] const MapData& getMap() const { return *currentMap; }

        // ── Переход между картами: затемнение, смена, осветление ──
        [[nodiscard]] float getFadeAlpha() const;     // 0 — ничего, 1 — экран чёрный
        void startTransition(std::function<void()> change);   // change вызывается в самый тёмный момент

        // ── Транспорт ──
        void vehicleAction();                         // E на карте: посадить дирижабль
        [[nodiscard]] bool canLandAirship() const;

        void sendPartyInfo();
        void openPartyCreation();
        void closePartyCreation();
        void createParty(const std::vector<SpecializationNames>& classes, const std::vector<std::string>& names);
        // private:
        void buildParty(const std::vector<SpecializationNames>& classes, const std::vector<std::string>& names);

    private:

        void loadMaps();
        [[nodiscard]] std::string slotPath(int slot) const;
        void placePlayerAfterLoad(bool positionRestored);
        void refreshLatestSave();
        void migrateOldSave();

        bool encounters = true;                       // случайные бои (выключаются в Config)
        int minimapZoom = 1;
        bool gameInProgress = false;                  // отряд на карте: можно сохраняться, «Continue» возвращает в игру
        float playSeconds = 0.0f;                     // время игры текущего прохождения
        int latestSaveSlot = -1;                      // самый свежий слот для «Continue», -1 — сохранений нет
        bool applyMapChange(int mapId, int x, int y);
        void followPortal(const Portal& portal, Point from);
        void exitBack();                              // шаг за край карты с exit = "back"
        [[nodiscard]] const MapData* mapByName(const std::string& name) const;
        [[nodiscard]] const MapData* mapWithEntrance(const std::string& portalName) const;
        enum class Fade { None, Out, In };
        Fade fadePhase = Fade::None;
        float fadeT = 0.0f;                           // 0..1 внутри фазы
        std::function<void()> fadeChange;
        void updateFade(float dt);
        struct ReturnPoint { int mapId; Point cell; };
        std::vector<ReturnPoint> returnStack;         // откуда пришли: выход из города или дома ведёт сюда
        void sendMapChange(int mapId, int x, int y);
        void gatherPartyAtHost();

        void handleInput(const SDL_Event &event);
        void update(float dt);
        void render();

        void initItems();
        void initSkills();

        void updateNetwork();

        void onStepFinished(Point from);
        bool updateVehicleAfterStep(Point from);     // посадка и высадка; true, если что-то сменилось
        void computeWater(MapData& m);
        void findRivers(MapData& m);                  // узкая вода вдали от моря — реки для каноэ
        [[nodiscard]] std::string battleBackground(const Zone* zone, const std::string& group) const;
        void resetVehicles();                         // транспорт на стоянки: маркеры Tiled или ближайшее подходящее место
        void parkVehicle(Point at, uint16_t mapId);   // слезть с транспорта, оставив его в клетке at
        Point stepFrom{};                             // откуда начался текущий шаг: туда ставим корабль при высадке
        void beginBattle(const Zone* zone = nullptr);
        [[nodiscard]] std::string nearestLandmark() const;   // имя ближайшего портала: по нему выбирается группа встреч
        [[nodiscard]] std::string encounterTerrain() const;  // land, sea или river — по транспорту
        Player& ensureRemote(uint32_t id);
        int stepsSinceBattle = 0;

        void processMovePacket(const PlayerMovePacket& packet);
        void processPacket(const char* data);

        void sendBattleEnd(bool isWin);

        void applyPartyInfo(const PartyInfoPacket& packet);

        void onConnectionLost(const std::string& reason);
        Uint32 connectTicks = 0;      // когда подключились: ждём Connect-пакет не дольше 3 секунд




        bool isRunning = false;

        std::map<int, MapData> maps;                  // mapId -> карта (узлы std::map не двигаются, указатель безопасен)
        std::map<std::string, int> mapIdByName;
        const MapData *currentMap = nullptr;

        Uint64 lastCounter = 0;     // для deltaTime
        Uint64 fpsLastCounter = 0;  // для подсчёта FPS
        int frameCount = 0;         // сколько кадров прошло с последнего замера
        int currentFPS = 0;         // реальный FPS для отображения
        float targetFPS = 120.0f;    // фиксированный FPS

        SDL_Rect screen = {0, 0, 900, 600};
        GameState state{GameState::CreatePlayer};
        GameState prevState{GameState::CreatePlayer};

        std::unordered_map<uint32_t, Player> remotePlayers;

        std::vector<char> netRecvBuffer;


        std::map<int, Item> items;
        std::map<int, Skill> skills;
        std::vector<Player> players;
        std::vector<NotGamePerson> npss;
        std::vector<Quest> quests;
        std::filesystem::path saveDir;

        std::unique_ptr<Battle> battle;
        std::unique_ptr<AudioManager> audioManager;
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