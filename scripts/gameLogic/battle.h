#ifndef PROJECT_NAME_BATTLE_H
#define PROJECT_NAME_BATTLE_H
#include "enemy.h"
#include "item.h"


class Battle
{
    std::vector<Enemy> enemies;
    std::vector<int> order;
    bool isWin;
    std::vector<Item> rewards;
};


#endif //PROJECT_NAME_BATTLE_H