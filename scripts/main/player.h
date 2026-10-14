#ifndef PROJECT_NAME_PLAYER_H
#define PROJECT_NAME_PLAYER_H

#include "movement.h"
#include "types.h"
#include "../ui/animation/animPlayer.h"
#include "../gameLogic/hero.h"
#include "../online/networkRole.h"
#include "../online/server.h"

enum class NetworkRole;

class Game;

class Player
{
public:
    explicit Player(std::string name);

    Player(Player&& other) noexcept = default;
    Player& operator=(Player&& other) noexcept = default;


    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    ~Player() = default;


    [[nodiscard]] int getLiveHeroCount() const;

    [[nodiscard]] int getAverageLevel() const;
    [[nodiscard]] uint32_t getPlayerGold() const noexcept;
    void setPlayerGold(uint32_t g) noexcept;

    [[nodiscard]] std::string getPlayerName() const noexcept;
    void setPlayerName(std::string newName) noexcept;


    [[nodiscard]] GameState getLastGameState() const noexcept;
    //void setLastGameState(GameState newGameState);

    [[nodiscard]] std::vector<Hero> getCopyOfHeroes() const noexcept;
    std::vector<Hero>& getLinkTOHeroes() noexcept;
    [[nodiscard]] const std::vector<Hero>& getHeroes() const noexcept;

    [[nodiscard]] std::vector<std::string> getDiscoveredEnemies() const;
    [[nodiscard]] std::vector<int> getAvailableItems() const;

    void addDiscoveredEnemy(const std::string& enemy);
    void addItem(int itemId, uint8_t count);

    [[nodiscard]] bool hasItem(int itemId) const;
    bool consumeItem(int itemId);
    //void removeItem(int itemId);


    [[nodiscard]] uint32_t getNetworkId() const noexcept;
    void setNetworkId(uint32_t id) noexcept;

    [[nodiscard]] NetworkRole getRole() const noexcept;
    void setRole(NetworkRole newRole) noexcept;

    [[nodiscard]] const std::map<int, uint8_t>& getItemCounts() const noexcept { return itemIds; }

    [[nodiscard]] MoveState& getMove() noexcept { return move; }
    [[nodiscard]] const MoveState& getMove() const noexcept { return move; }

    [[nodiscard]] Direction getDirection() const { return move.moving ? move.dir : Direction::Idle; }
    void setDirection(const Direction& d) { if (d != Direction::Idle) move.dir = d; }
    [[nodiscard]] Point getPlayerCoords() const noexcept { return move.cell; }
    void setPlayerCoords(Point p) noexcept { move.cell = move.target = p; move.moving = false; move.progress = 0.f; }

    AnimPlayer animPlayer;

private:
    uint32_t networkId {0};
    NetworkRole role {NetworkRole::Local};

    std::string name;
    uint32_t gold{0};
    MoveState move;

    GameState lastState{GameState::Map};
    Direction direction {Direction::Idle};

    std::vector<Hero> heroes;
    std::vector<std::string> discoveredEnemies;
    std::map<int, uint8_t> itemIds;
};


#endif //PROJECT_NAME_PLAYER_H