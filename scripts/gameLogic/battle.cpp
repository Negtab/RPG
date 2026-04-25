#include "battle.h"
#include <algorithm>
#include <chrono>

#include "../main/game.h"

const std::vector<Enemy> &Battle::getEnemies() const {
    return enemies;
}

BattleState Battle::getState() const {
    return state;
}

void Battle::setState(BattleState newState) {
    this->state = newState;
}

int Battle::getCurrentHeroIndex() const
{
    return currentHeroIndex;
}

bool Battle::isPlayerChoosing() const
{
    return state == BattleState::PlayerChoose;
}

bool Battle::canEscape() const
{
    return !escapeUsed && state != BattleState::Result;
}

void Battle::tryEscape()
{
    if (!canEscape())
        return;

    escapeUsed = true;

    int roll = rand() % 100;
    if (roll < escapeChance)
    {
        // успешный побег
        state = BattleState::EscapeResult;
    }
    else
    {
        // неудачный — бой продолжается
        // по правилам JRPG — обычно враги ходят
        startEnemyTurn();
    }
}


void Battle::startExecuteActions()
{
    currentTurn = TurnOwner::Player;
    state = BattleState::ExecuteActions;
    currentActionIndex = 0;
}

void Battle::startPlayerChoose()
{
    currentTurn = TurnOwner::Player;
    plannedActions.clear();
    currentHeroIndex = 0;
    state = BattleState::PlayerChoose;

    while (currentHeroIndex < player->getHeroes().size() &&
           player->getHeroes()[currentHeroIndex].getCurrentHp() <= 0)
        currentHeroIndex++;
}

void Battle::confirmAction(const Action& action)
{
    if (action.actorIndex != currentHeroIndex)
        return;

    plannedActions.push_back(action);

    // переход к следующему герою
    currentHeroIndex++;
    while (currentHeroIndex < player->getHeroes().size() &&
        player->getHeroes()[currentHeroIndex].getCurrentHp() <= 0)
        currentHeroIndex++;

    // все герои выбрали действие
    if (currentHeroIndex >= player->getHeroes().size())
    {
        // ПОКА просто переходим дальше
        // позже здесь будет ExecuteActions
        startExecuteActions();
    }
}

void Battle::updateExecuteActions()
{
    if (state != BattleState::ExecuteActions)
        return;

    // все действия исполнены
    if (currentActionIndex >= plannedActions.size())
    {
        plannedActions.clear();
        state = BattleState::Result;
        return;
    }

    Action& action = plannedActions[currentActionIndex];

    // атакующий мёртв — пропускаем
    if (player->getHeroes()[action.actorIndex].getCurrentHp() <= 0)
    {
        currentActionIndex++;
        return;
    }

    // исполняем действие
    executeAction(action);

    currentActionIndex++;

    // после каждого действия — анимация
    currentTurn = TurnOwner::Player;
    state = BattleState::Animation;
}

void Battle::executeItem(Hero& user, const std::vector<int>& targets, int itemId)
{
    if (!player->hasItem(itemId))
        return;

    Item& item = game->getItem(itemId);

    for (int idx : targets)
    {
        switch (item.effectType)
        {
            case ItemEffectType::Heal:
                player->getLinkTOHeroes()[idx].heal(item.power);
                break;

            case ItemEffectType::Damage:
                enemies[idx].takeDamage(item.power);
                break;

            case ItemEffectType::RestoreMana:
                player->getLinkTOHeroes()[idx].restoreMana(item.power);
                break;

            default:
                break;
        }
    }

    if (item.consumable)
        player->consumeItem(itemId);
}

void Battle::executeSkill(GamePerson& caster, int skillId, const std::vector<int>& targets, bool casterIsHero)
{
    const Skill& skill = game->getSkill(skillId);

    if (!caster.canCast(skill))
        return;

    caster.spendMana(skill.manaCost);

    for (int idx : targets)
    {
        switch (skill.effectType)
        {
            case SkillEffectType::Damage:
                if (casterIsHero)
                    enemies[idx].takeDamage(skill.power, skill.element);
                else
                    player->getLinkTOHeroes()[idx].takeDamage(skill.power, skill.element);
                break;

            case SkillEffectType::Heal:
                if (casterIsHero)
                    player->getLinkTOHeroes()[idx].heal(skill.power);
                else
                    enemies[idx].heal(skill.power);
                break;

            default:
                break;
        }
    }

    caster.setCooldown(skill.id, skill.maxCooldown);
}


void Battle::executeAttack(Hero& attacker, const std::vector<int>& targets)
{
    for (int idx : targets)
    {
        Enemy& target = enemies[idx];
        uint32_t damage = attacker.getAttackPower(nullptr);
        target.takeDamage(damage);
        tryCounterAttack(attacker, target);
    }
}


void Battle::executeAction(const Action& action)
{
    Hero& actor = player->getLinkTOHeroes()[action.actorIndex];

    if (actor.getCurrentHp() <= 0)
        return;

    auto targets = resolveTargets(action);
    if (targets.empty())
        return;

    switch (action.type)
    {
        case ActionType::Attack:
            executeAttack(actor, targets);
            break;

        case ActionType::Magic:
            executeSkill(actor, action.payloadId, targets, true);
            break;

        case ActionType::Item:
            executeItem(actor, targets, action.payloadId);
            break;

        case ActionType::Skip:
            // ничего не делаем, но ход считается
            break;
    }

    currentTurn = TurnOwner::Player;
    state = BattleState::Animation;
}


void Battle::tryCounterAttack(Hero& attacker, Enemy& target)
{
    if (target.getCurrentHp() <= 0)
        return;

    float speedRatio = static_cast<float>(target.getSpeed()) / static_cast<float>(attacker.getSpeed());

    // коэффициент "сильно быстрее"
    const float SPEED_COEFF = 1.5f;

    if (speedRatio < SPEED_COEFF)
        return;

    // шанс контрудара (можно вынести в параметр)
    int chance = static_cast<int>((speedRatio - 1.0f) * 50); // пример
    chance = std::min(chance, 80); // ограничение

    int roll = rand() % 100;
    if (roll >= chance)
        return;

    uint32_t counterDamage = target.getAttackPower(nullptr) / 2;
    attacker.takeDamage(counterDamage);
}

void Battle::startEnemyTurn()
{
    currentTurn = TurnOwner::Enemy;
    state = BattleState::EnemyTurn;
    currentEnemyIndex = 0;
}


void Battle::updateEnemyTurn()
{
    if (state != BattleState::EnemyTurn)
        return;

    // все враги сходили
    if (currentEnemyIndex >= enemies.size())
    {
        state = BattleState::PlayerChoose;
        endRound();
        startPlayerChoose();
        return;
    }

    Enemy& enemy = enemies[currentEnemyIndex];

    // мёртвый враг — пропускаем
    if (enemy.getCurrentHp() < 0)
    {
        currentEnemyIndex++;
        return;
    }

    // выбираем цель
    int targetIndex = selectHeroTarget();

    // живых героев нет — бой закончится в Result
    if (targetIndex == -1)
    {
        state = BattleState::Result;
        return;
    }

    Hero& target = player->getLinkTOHeroes()[targetIndex];

    uint32_t damage = enemy.getAttackPower(nullptr);
    target.takeDamage(damage);

    currentEnemyIndex++;

    // после каждой атаки — анимация
    currentTurn = TurnOwner::Enemy;
    state = BattleState::Animation;
}

int Battle::selectHeroTarget() const
{
    for (int i = 0; i < player->getHeroes().size(); ++i)
    {
        if (player->getHeroes()[i].getCurrentHp() > 0)
            return i;
    }
    return -1;
}

std::vector<int> Battle::resolveTargets(const Action& action)
{
    std::vector<int> result;

    switch (action.targetType)
    {
        case TargetType::Self:
        {
            // сам кастер (герой)
            result.push_back(action.actorIndex);
            break;
        }

        case TargetType::Enemy:
        {
            // один конкретный враг
            if (action.targetIndex >= 0 &&
                action.targetIndex < static_cast<int>(enemies.size()) &&
                enemies[action.targetIndex].getCurrentHp() > 0)
            {
                result.push_back(action.targetIndex);
            }
            break;
        }

        case TargetType::Ally:
        {
            // один конкретный союзник
            if (action.targetIndex >= 0 &&
                action.targetIndex < static_cast<int>(player->getHeroes().size()) &&
                player->getHeroes()[action.targetIndex].getCurrentHp() > 0)
            {
                result.push_back(action.targetIndex);
            }
            break;
        }

        case TargetType::AllEnemies:
        {
            for (int i = 0; i < static_cast<int>(enemies.size()); ++i)
            {
                if (enemies[i].getCurrentHp() > 0)
                    result.push_back(i);
            }
            break;
        }

        case TargetType::AllAllies:
        {
            for (int i = 0; i < static_cast<int>(player->getHeroes().size()); ++i)
            {
                if (player->getHeroes()[i].getCurrentHp() > 0)
                    result.push_back(i);
            }
            break;
        }
    }

    return result;
}


void Battle::onAnimationFinished()
{
    if (!ui->isAnimationFinished("Battle"))
        return;

    isAnimationsStarted = false;
    plannedActions.clear();

    if (currentTurn == TurnOwner::Player)
        state = BattleState::ExecuteActions;
    else
        state = BattleState::EnemyTurn;
}


void Battle::checkBattleResult()
{
    bool heroesAlive = false;
    bool enemiesAlive = false;

    for (auto& h : player->getHeroes())
        if (h.getCurrentHp() > 0) { heroesAlive = true; break;}

    for (auto& e : enemies)
        if (e.getCurrentHp() > 0) { enemiesAlive = true; break;}

    if (!heroesAlive && enemiesAlive) {
        finishBattle(false);
        return;
    }
    if (heroesAlive && !enemiesAlive)
    {
        finishBattle(true);
        return;
    }

    startEnemyTurn();
}

void Battle::endRound()
{
    for (auto& hero : player->getLinkTOHeroes())
        hero.tickCooldowns();

    for (auto& enemy : enemies)
        enemy.tickCooldowns();
}


void Battle::update()
{
    switch (state)
    {
        case BattleState::EnemyTurn:
            updateEnemyTurn();
            break;

        case BattleState::ExecuteActions:
            updateExecuteActions();
            break;

        case BattleState::Animation:
            if (!isAnimationsStarted)
            {

                for (const auto& plannedAction : plannedActions)
                {
                    ui->playAnimation(
                        "Hero" + std::to_string(plannedAction.actorIndex + 1) + "Animation",
                        "Battle",
                        false
                    );
                }

                isAnimationsStarted = true;
            }

            onAnimationFinished(); // ← ТОЛЬКО ПРОВЕРКА
            break;

        case BattleState::Result:
            checkBattleResult();
            break;

        case BattleState::EscapeResult:
            finishBattle(false);
            break;


        default:
            break;
    }
}


void Battle::finishBattle(const bool &isWin)
{
    if (isWin) {
        uint32_t sumExp = 0, sumGold = 0;
        for (auto e : enemies)
        {
            sumExp += e.getExperience();
            sumGold += e.getGold();
        }
        player->setPlayerGold(player->getPlayerGold() + sumGold);
        std::vector<Hero> heroes = player->getLinkTOHeroes();
        for (int i = 0; i < heroes.size(); i++)
            heroes.at(i).addXP(sumExp/heroes.size());
    }
    else {
        //sd
    }

    game->setGameState(GameState::Map);
}

void Battle::spawnEnemies() {
    int avgLvl = player->getAverageLevel();

    int enemyCount = 1 + rand() % std::min(3, avgLvl + 1);

    for (int i = 0; i < enemyCount; i++)
    {
        int t = rand() % 4;
        auto name = static_cast<EnemyName>(t);
        enemies.emplace_back(name, player);
    }
}

void Battle::determineFirstTurn()
{
    firstTurnIsPlayer = (rand() % 100) < 30;
}

void Battle::prepairUI()
{
    for (int i = 0; i < enemies.size(); i++)
    {
        SDL_Rect enemyRect = enemiesSize.at(enemies.at(i).getEnemyName());
        ui->setSrcRect("Enemy" + std::to_string(i + 1), "Battle", enemyRect);
    }
}

void Battle::run()
{
    enemies.clear();
    rewards.clear();
    isWin = false;
    escapeChance = false;

    spawnEnemies();
    prepairUI();
    determineFirstTurn();


    if (firstTurnIsPlayer) {
        currentTurn = TurnOwner::Player;
        state = BattleState::PlayerChoose;
    } else {
        currentTurn = TurnOwner::Enemy;
        state = BattleState::EnemyTurn;
    }
}
