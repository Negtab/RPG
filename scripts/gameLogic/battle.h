#ifndef PROJECT_NAME_BATTLE_H
#define PROJECT_NAME_BATTLE_H

#include <optional>
#include <unordered_set>
#include <vector>

#include "enemy.h"
#include "action.h"

#include "../main/player.h"
#include "../ui/uiManager.h"

class Game;
class UIManager;

struct HeroRef { uint32_t owner; int index; };

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

    void run(const std::vector<int>& formation = {}, const std::vector<int>& pool = {});
    void update();

    [[nodiscard]] BattleState getState() const;
    void setState(BattleState state);

    [[nodiscard]] const std::vector<Enemy>& getEnemies() const;
    [[nodiscard]] int  getCurrentHeroIndex() const;
    [[nodiscard]] bool isPlayerChoosing() const;

    void startPlayerChoose();
    void skipDeadHeroes();                             // к следующему живому герою; если таких нет, выбор окончен
    void confirmAction(const Action& action);

    void tryEscape();
    [[nodiscard]] bool canEscape() const;

    void startExecuteActions();
    void updateExecuteActions();

    void startEnemyTurn();
    void updateEnemyTurn();

    void onAnimationFinished();

    void runFromNetwork(const BattleStartPacket& packet);
    void confirmActionFromNetwork(const Action& action);
    void applySnapshot(const BattleSnapshotPacket& packet);

    void resetState();

    TurnOwner getCurrentTurn() const;
    int getCurrentActionIndex() const;

    void onPlayerLeft(uint32_t playerId);
    [[nodiscard]] bool isParticipant(uint32_t id) const { return participants.contains(id); }

    void closeResults();                               // игрок подтвердил окно
    [[nodiscard]] const BattleReport& getReport() const { return report; }

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

    std::vector<int> resolveTargets(const Action& action);

    void checkBattleResult();
    void finishBattle(const bool &isWin);

    int averageLevel() const;

    Hero* resolveActor(const Action& action);

    void leaveBattle();
    BattleReport report;

    Player *player;
    UIManager *ui;
    Game *game;

    BattleState state = BattleState::Start;
    TurnOwner currentTurn;


    bool firstTurnIsPlayer = false;
    bool isWin = false;
    bool isAnimationsStarted = false;
    uint32_t animStartTicks = 0;                       // когда начался показ действия: у врагов анимации нет, держим паузу

    bool escapeUsed = false;
    int escapeChance = 40;

    std::vector<Action> plannedActions;
    int currentHeroIndex = 0;
    int currentEnemyIndex = 0;
    int currentActionIndex = 0;

    int playersReady = 0;
    int expectedPlayers = 1;

    std::vector<Enemy> enemies;
    std::vector<Item> rewards;

    std::unordered_set<uint32_t> readyPlayers;

    std::vector<int> enemyFormation, enemyPool;                  // enemyPool уже был
    std::vector<HeroRef> aliveHeroes() const;
    bool performEnemyAction(int enemyIndex);                     // false: целей нет, все герои мертвы
    bool enemyBasicAttack(Enemy& enemy, int hits);
    bool enemyUseSkill(int enemyIndex, int skillId);

    std::optional<HeroRef> selectHeroTarget() const;    // было: int selectHeroTarget() const
    std::vector<Hero>* partyOf(uint32_t owner);
    Hero* heroByRef(const HeroRef& ref);
    void beginParticipants();
    void executeSkill(GamePerson& caster, int skillId, const std::vector<int>& targets,
                      bool casterIsHero, uint32_t owner);
    void executeItem(Hero& user, const std::vector<int>& targets, int itemId, uint32_t owner);

    std::unordered_set<uint32_t> participants;          // кто был в бою на момент старта
};

#endif // PROJECT_NAME_BATTLE_H
