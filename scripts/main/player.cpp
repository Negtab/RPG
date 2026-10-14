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