#include "player.h"
#include "game.h"


Player::Player(std::string name) : name(std::move(name)) {}

void Player::resetProgress()
{
    heroes.clear();
    gold = 0;
    itemIds.clear();
    discoveredEnemies.clear();
    move = MoveState{};
    riding = Vehicle::None;
    vehicles = {};
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

std::string Player::getPlayerName() const noexcept { return name; }
void Player::setPlayerName(std::string newName) noexcept { name = std::move(newName); }

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


void Player::addDiscoveredEnemy(const std::string& enemy)
{
    if (std::find(discoveredEnemies.begin(), discoveredEnemies.end(), enemy) == discoveredEnemies.end())
        discoveredEnemies.push_back(enemy);
}

bool Player::isEnemyDiscovered(int enemyId) const
{
    const std::string key = std::to_string(enemyId);
    return std::find(discoveredEnemies.begin(), discoveredEnemies.end(), key) != discoveredEnemies.end();
}
void Player::addItem(int itemId, uint8_t count)
{
    if (itemId == 0 || count == 0) return;
    auto& n = itemIds[itemId];
    n = static_cast<uint8_t>(std::min(99, n + count));      // как в FF: не больше 99 штук, и uint8_t не переполнится
}
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