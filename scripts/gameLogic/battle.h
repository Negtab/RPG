#ifndef PROJECT_NAME_BATTLE_H
#define PROJECT_NAME_BATTLE_H
#include "enemy.h"
#include "item.h"


class Battle
{
public:
    explicit Battle(Player &player) : player(player){}
    ~Battle() = default;
    [[nodiscard]] std::vector<Enemy> getEnemies() const;
    void getResult() const;

    void setOrder();
    void processAttack();
private:
    bool isWin {false};
    Player &player;
    std::vector<Enemy> enemies;
    std::vector<int> order;
    std::vector<Item> rewards;
};


#endif //PROJECT_NAME_BATTLE_H