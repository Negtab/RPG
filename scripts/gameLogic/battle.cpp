#include "battle.h"
#include <algorithm>
#include <chrono>

std::vector<Enemy> Battle::getEnemies() const
{
    return enemies;
}

void Battle::startPlayerTurn() {
    state = BattleState::SelectAction;
    ui->setEnVI("ActionPanel","Battle", true);
    ui->setEnVI("MagicPanel","Battle", false);
    ui->setEnVI("InventoryPanel","Battle", false);
}



/*void Battle::onChooseTarget(int enemyId) {
    state = BattleState::Animation;
    ui->playAttackAnimation(player, enemies[enemyId]);
    applySkill(enemyId);

    state = BattleState::Result;
    ui->showBattleLog(...);

    if (isWin || !isLose) finishBattle();
    else enemyTurn();
}*/

BattleState Battle::getState() const {
    return state;
}

void Battle::setState(BattleState state) {
    this->state = state;
}

bool Battle::getChoose() const {
    return isChoose;
}

void Battle::setChoose(bool isChoose) {
    this->isChoose = isChoose;
}


void Battle::setOrder()
{
    order.clear();

    bool surpriseAttack = (rand() % 100) < 30; // 30% что враги начинают

    // heroes
    auto heroes = player->getHeroes();
    for (int i = 0; i < (int)heroes.size(); i++)
        order.emplace_back(heroes[i].getSpeed(), i); // скорость + индекс (из позитива)

    // enemies (отрицательный индекс)
    for (int i = 0; i < (int)enemies.size(); i++)
        order.emplace_back(enemies[i].getSpeed(), -(i+1));

    std::sort(order.begin(), order.end(), [](auto& a, auto& b){
        return a.first > b.first;
    });

    if (surpriseAttack)
        std::reverse(order.begin(), order.end());

    for (auto& u : order)
        order.push_back(u.second);
}

void Battle::processAttack()
{
    for (auto unit : order)
    {
        if (unit.first >= 0)
        {
            // Ход героя
            auto heroes = player->getHeroes();
            Hero& h = heroes[unit.first];

            if (h.getCurrentHp() == 0) continue;

            // выбираем случайного живого врага
            Enemy* target = nullptr;
            for (auto& e : enemies)
                if (e.getCurrentHp() > 0) { target = &e; break; }

            if (!target) { isWin = true; return; }

            uint32_t dmg = h.getAttackPower(nullptr) - target->getDefense();
            if (dmg < 1) dmg = 1;

            target->setCurrentHp(target->getCurrentHp() - dmg);
            if (target->getCurrentHp() == 0)
                target->killPerson();
        }
        else
        {
            // Ход врага
            int idx = -unit.first - 1;
            Enemy& e = enemies[idx];
            if (e.getCurrentHp() == 0) continue;

            auto heroes = player->getHeroes();
            Hero* target = nullptr;
            for (auto& h : heroes)
                if (h.getCurrentHp() > 0) { target = &h; break; }

            if (!target)
            {
                isWin = false;
                return;
            }

            uint32_t dmg = e.getAttackPower(nullptr) - target->getDefense();
            if (dmg < 1) dmg = 1;

            target->setCurrentHp(target->getCurrentHp() - dmg);
            if (target->getCurrentHp() == 0)
                target->killPerson();
        }
    }
}


/*void Battle::WinBattle()
{
    int sumExp = 0, sumGold = 0;
    for (auto e : enemies)
    {
        sumExp += e.getExperience();
        sumGold += e.getGold();
    }
    player->setPlayerGold(player->getPlayerGold() + sumGold);
    std::vector<Hero> heroes = player->getHeroes();
    for (int i = 0; i < heroes.size(); i++)
        heroes.at(i).addXP(sumExp/heroes.size());
}*/

void Battle::enemyTurn()
{

}

void Battle::heroTurn()
{

    while (!isChoose);

}

void Battle::startBattle()
{
    for (auto u : order)
        if (u.second <= 0)
            heroTurn();
        else
            enemyTurn();
}

void Battle::run()
{
    enemies.clear();
    rewards.clear();
    isWin = false;

    int avgLvl = player->getAverageLevel();

    int enemyCount = 1 + rand() % std::min(3, avgLvl + 1);

    for (int i = 0; i < enemyCount; i++)
    {
        int t = rand() % 4;
        EnemyName name = static_cast<EnemyName>(t);
        enemies.emplace_back(name, player.get());
    }

    setOrder();
    startBattle();
}
