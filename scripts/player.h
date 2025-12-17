#ifndef PROJECT_NAME_PLAYER_H
#define PROJECT_NAME_PLAYER_H

#include "types.h"
#include "gameLogic/gamePerson.h"
#include "gameLogic/hero.h"

class Game;

class Player
{
public:
    explicit Player(std::string name);
    ~Player() = default;

    [[nodiscard]] int getAverageLevel() const;
    [[nodiscard]] uint32_t getPlayerGold() const noexcept;
    void setPlayerGold(uint32_t g) noexcept;

    [[nodiscard]] int32_t getPlayerSpeed() const noexcept;
    void setPlayerSpeed(int32_t newSpeed) noexcept;

    [[nodiscard]] std::string getPlayerName() const noexcept;
    void setPlayerName(std::string newName) noexcept;

    [[nodiscard]] Point getPlayerCoords() const noexcept;
    void setPlayerCoords(Point newCoords) noexcept;

    [[nodiscard]] GameState getLastGameState() const noexcept;
    void setLastGameState(GameState newGameState);

    [[nodiscard]] std::vector<Hero> getHeroes() const noexcept;

    [[nodiscard]] std::vector<std::string> getDiscoveredEnemies() const;
    [[nodiscard]] std::vector<int> getAvailableItems() const;
    [[nodiscard]] Location getPlayerLocation(const Game& game) const;

    void addDiscoveredEnemy(const std::string& enemy);
    void addItem(int itemId, uint8_t count);

    bool hasItem(int itemId) const;
    bool consumeItem(int itemId);
    void removeItem(int itemId);

private:
    std::string name;
    uint32_t gold{0};
    Point coords{0, 0};
    int32_t speed{1};

    GameState lastState{GameState::Map};

    std::vector<Hero> heroes;
    std::vector<std::string> discoveredEnemies;
    std::map<int, uint8_t> itemIds;
};


#endif //PROJECT_NAME_PLAYER_H