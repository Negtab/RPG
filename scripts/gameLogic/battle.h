#ifndef PROJECT_NAME_BATTLE_H
#define PROJECT_NAME_BATTLE_H
#include "enemy.h"
#include "item.h"
#include "../player.h"
#include "../uiManager.h"

class Battle
{
public:
    explicit Battle(Player *player, UIManager *ui) : player(player), ui(ui) {}
    ~Battle() = default;

    void run();

    [[nodiscard]] const std::vector<Enemy> &getEnemies() const;

    void getResult() const;

    void setState(BattleState state);
    [[nodiscard]] BattleState getState() const;

    void setChoose(bool isChoose);
    [[nodiscard]] bool getChoose() const;

    void finishChoose();
private:
    void setOrder();
    void processAttack();
    void startPlayerTurn();
    void finishBattle();

    void enemyTurn();
    void heroTurn();

    void startBattle();


    bool isWin {false}, isLose {false}, isChoose {false};
    int currentSkill;
    std::unique_ptr<Player> player;
    std::unique_ptr<UIManager> ui;
    BattleState state = BattleState::SelectAction;
    std::vector<Enemy> enemies;
    std::vector<std::pair<int,int>> order;
    std::vector<Item> rewards;
};


#endif //PROJECT_NAME_BATTLE_H