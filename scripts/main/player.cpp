#include "player.h"
#include "game.h"


Player::Player(std::string name) : name(std::move(name)) {
    heroes.emplace_back(Specialization(SpecializationNames::Archer));
    heroes.emplace_back(Specialization(SpecializationNames::Magician));
    heroes.emplace_back(Specialization(SpecializationNames::Thief));
    heroes.emplace_back(Specialization(SpecializationNames::Warrior));
}

uint32_t Player::getNetworkId() const noexcept
{
    return networkId;
}

void Player::setNetworkId(uint32_t id) noexcept
{
    networkId = id;
}

NetworkRole Player::getRole() const noexcept
{
    return role;
}

void Player::setRole(NetworkRole newRole) noexcept
{
    role = newRole;
}

int Player::getLiveHeroCount() const
{
    int count = 0;
    for (const auto& hero : heroes)
        if (hero.getIsAlive())
            count++;
    return count;

}


int Player::getAverageLevel() const
{
    int total = 0, count = 0;
    for (const auto& hero : heroes)
        total += hero.getLevel(), ++count;
    return (count > 0) ? total / count : 0;
}

const std::vector<Hero>& Player::getHeroes() const noexcept { return heroes; }
std::vector<Hero>& Player::getLinkTOHeroes() noexcept { return heroes; }
std::vector<Hero> Player::getCopyOfHeroes() const noexcept { return heroes; }


uint32_t Player::getPlayerGold() const noexcept { return gold; }
void Player::setPlayerGold(uint32_t g) noexcept { gold = g; }

int32_t Player::getPlayerSpeed() const noexcept { return speed; }
void Player::setPlayerSpeed(int32_t newSpeed) noexcept { speed = newSpeed; }

std::string Player::getPlayerName() const noexcept { return name; }
void Player::setPlayerName(std::string newName) noexcept { name = std::move(newName); }

Point Player::getPlayerCoords() const noexcept { return coords; }
void Player::setPlayerCoords(Point newCoords) noexcept { coords = newCoords; }

GameState Player::getLastGameState() const noexcept { return lastState; }


std::vector<std::string> Player::getDiscoveredEnemies() const { return discoveredEnemies; }

std::vector<int> Player::getAvailableItems() const
{
    std::vector<int> available;
    available.reserve(itemIds.size());
    for (const auto& [id, _] : itemIds)
        available.push_back(id);
    return available;
}

Location Player::getPlayerLocation(const Game& game) const
{
    Point playerCoords = getPlayerCoords();
   for (const auto& loc : game.getLocations())
        if (playerCoords.x > loc.coord.x && playerCoords.x < loc.coord.x + loc.w &&
            playerCoords.y > loc.coord.y && playerCoords.y < loc.coord.y + loc.h)
            return loc;
    return {};
}

void Player::addDiscoveredEnemy(const std::string& enemy) { discoveredEnemies.push_back(enemy); }
void Player::addItem(int itemId, uint8_t count) { itemIds[itemId] += count; }
bool Player::hasItem(int itemId) const
{
    auto it = itemIds.find(itemId);
    return it != itemIds.end() && it->second > 0;
}

bool Player::consumeItem(int itemId)
{
    auto it = itemIds.find(itemId);
    if (it == itemIds.end() || it->second == 0)
        return false;

    it->second--;

    if (it->second == 0)
        itemIds.erase(it);

    return true;
}

Direction Player::getDirection() const { return this->direction; }
void Player::setDirection(const Direction &newDirection) { this->direction = newDirection; }