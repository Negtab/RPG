#include "player.h"
#include "game.h"


Player::Player(std::string name) : name(std::move(name)) {}

int Player::getAverageLevel() const
{
    int total = 0, count = 0;
    for (const auto& hero : heroes)
        total += hero.getLevel(), ++count;
    return (count > 0) ? total / count : 0;
}

uint32_t Player::getPlayerGold() const noexcept { return gold; }
void Player::setPlayerGold(uint32_t g) noexcept { gold = g; }

int32_t Player::getPlayerSpeed() const noexcept { return speed; }
void Player::setPlayerSpeed(int32_t newSpeed) noexcept { speed = newSpeed; }

std::vector<EnemiesNames> Player::getDiscoveredEnemies() const { return discoveredEnemies; }

std::vector<int> Player::getAvailableItems() const
{
    std::vector<int> available;
    available.reserve(itemIds.size());
    for (const auto&
    [id, _] : itemIds)
        available.push_back(id);
    return available;
}

Location Player::getPlayerPlace(const Game& game) const
{
   /* for (const auto& loc : game.locations)
        if (coords.x > loc.coord.x && coords.x < loc.coord.x + loc.w &&
            coords.y > loc.coord.y && coords.y < loc.coord.y + loc.h)
            return loc;
    return {};*/
}

void Player::addDiscoveredEnemy(EnemiesNames enemy) { discoveredEnemies.push_back(enemy); }
void Player::addItem(int itemId, uint8_t count) { itemIds[itemId] += count; }

