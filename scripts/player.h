#ifndef PROJECT_NAME_PLAYER_H
#define PROJECT_NAME_PLAYER_H

#include "types.h"
#include "gamePerson.h"

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

    [[nodiscard]] std::vector<EnemiesNames> getDiscoveredEnemies() const;
    [[nodiscard]] std::vector<int> getAvailableItems() const;
    [[nodiscard]] Location getPlayerLocation(const Game& game) const;

    void addDiscoveredEnemy(EnemiesNames enemy);
    void addItem(int itemId, uint8_t count);

private:
    std::string name;
    uint32_t gold{0};
    Point coords{};
    int32_t speed{0};
    std::vector<Hero> heroes;
    std::vector<EnemiesNames> discoveredEnemies;
    std::map<int, uint8_t> itemIds;
};


#endif //PROJECT_NAME_PLAYER_H