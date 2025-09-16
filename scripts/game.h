#ifndef PROJECT_NAME_GAME_H
#define PROJECT_NAME_GAME_H

#include "item.h"
#include "quest.h"
#include "gamePerson.h"
#include "player.h"

class Game
{
public:
    Game() = default;
    ~Game() = default;

    void addPlayer(const Player& player);

    GameState state{GameState::CreatePlayer};
    std::map<int, Item> items;
    std::map<int, Skill> skills;
    std::vector<Location> locations;
    std::vector<Player> players;
    std::vector<NotGamePerson> npss;
    std::vector<Quest> quests;
    std::string pathToSave;
};


#endif //PROJECT_NAME_GAME_H