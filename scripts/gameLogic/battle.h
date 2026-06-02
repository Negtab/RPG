#ifndef PROJECT_NAME_BATTLE_H
#define PROJECT_NAME_BATTLE_H

#include <vector>

#include "enemy.h"
#include "action.h"

#include "../main/player.h"
#include "../ui/uiManager.h"

class Game;
class UIManager;

struct BattleTarget
{
    bool isHero;
    int index;
};

class Battle
{
public:
    Battle(Player* player, UIManager* ui, Game* game) : player(player), ui(ui), game(game) {};
    ~Battle() = default;

    void run();
    void update();

    [[nodiscard]] BattleState getState() const;
    void setState(BattleState state);

    [[nodiscard]] const std::vector<Enemy>& getEnemies() const;
    [[nodiscard]] int  getCurrentHeroIndex() const;
    [[nodiscard]] bool isPlayerChoosing() const;

    void startPlayerChoose();
    void confirmAction(const Action& action);

    void tryEscape();
    [[nodiscard]] bool canEscape() const;

    void startExecuteActions();
    void updateExecuteActions();

    void startEnemyTurn();
    void updateEnemyTurn();

    void onAnimationFinished();

private:
    void prepairUI();
    void spawnEnemies();
    void determineFirstTurn();
    void endRound();

    void executeAction(const Action& action);
    void executeAttack(Hero& attacker, const std::vector<int>& targets);
    void executeSkill(GamePerson& caster,int skillId,const std::vector<int>& targets,bool casterIsHero);
    void executeItem(Hero& user, const std::vector<int>& targets, int itemId);

    void tryCounterAttack(Hero& attacker, Enemy& target);

    [[nodiscard]] int selectHeroTarget() const;
    std::vector<int> resolveTargets(const Action& action);

    void checkBattleResult();
    void finishBattle(const bool &isWin);

    void startNextHeroAnimation();

    Player *player;
    UIManager *ui;
    Game *game;

    BattleState state = BattleState::Start;
    TurnOwner currentTurn;

    bool firstTurnIsPlayer = false;
    bool isWin = false;
    bool isAnimationsStarted = false;

    bool escapeUsed = false;
    int escapeChance = 40;

    std::vector<Action> plannedActions;
    int currentHeroIndex = 0;
    int currentEnemyIndex = 0;
    int currentActionIndex = 0;

    std::vector<Enemy> enemies;
    std::vector<Item> rewards;
};

#endif // PROJECT_NAME_BATTLE_H
