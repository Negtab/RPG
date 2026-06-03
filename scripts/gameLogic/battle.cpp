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

int Battle::getCurrentActionIndex() const
{
    return currentActionIndex;
}

TurnOwner Battle::getCurrentTurn() const
{
    return currentTurn;
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

void Battle::runFromNetwork(const BattleStartPacket& packet)
{
    enemies.clear();
    rewards.clear();
    isWin = false;
    escapeUsed = false;

    for (int i = 0; i < packet.enemyCount && i < 4; i++)
    {
        auto name = static_cast<EnemyName>(packet.enemies[i].enemyName);
        enemies.emplace_back(name, player);
    }

    prepairUI();

    expectedPlayers = game->getPlayerCount();
    playersReady = 0;

    currentTurn = TurnOwner::Player;
    state = BattleState::PlayerChoose;
    startPlayerChoose();
}

void Battle::confirmActionFromNetwork(const Action& action)
{
    if (action.playerId == game->localPlayerId)
        return;

    const auto& remotePlayers = game->getRemotePlayers();
    if (!remotePlayers.contains(action.playerId))
    {
        SDL_Log("Unknown playerId=%d, skipping", action.playerId);
        return;
    }

    // защита от дублирования — игрок уже засчитан
    if (readyPlayers.contains(action.playerId))
        return;

    plannedActions.push_back(action);

    // считаем сколько действий пришло именно от этого игрока
    int actionsFromPlayer = 0;
    for (auto& a : plannedActions)
        if (a.playerId == action.playerId)
            actionsFromPlayer++;

    int heroesOfPlayer = remotePlayers.at(action.playerId).getLiveHeroCount();
    // getLiveHeroCount может вернуть 0 если герои ещё не загружены — защита
    if (heroesOfPlayer <= 0)
        heroesOfPlayer = 1;

    if (actionsFromPlayer >= heroesOfPlayer)
    {
        readyPlayers.insert(action.playerId);
        playersReady++;

        if (game->isHost() && playersReady >= expectedPlayers)
        {
            playersReady = 0;
            readyPlayers.clear();
            startExecuteActions();
        }
    }
}

void Battle::applySnapshot(const BattleSnapshotPacket& packet)
{
    for (int i = 0; i < packet.heroCount; i++)
    {
        if (packet.heroOwner[i] == game->localPlayerId)
        {
            int localIndex = 0;
            for (int j = 0; j < i; j++)
                if (packet.heroOwner[j] == game->localPlayerId)
                    localIndex++;

            if (localIndex < (int)player->getHeroes().size())
            {
                player->getLinkTOHeroes()[localIndex].setCurrentHp(packet.heroes[i].currentHp);
                player->getLinkTOHeroes()[localIndex].setCurrentMp(packet.heroes[i].currentMana);
            }
        }
        else
        {
            // ← был const ref, теперь mutable
            auto& remotePlayers = game->getRemotePlayersMutable();
            if (!remotePlayers.contains(packet.heroOwner[i]))
                continue;

            int remoteIndex = 0;
            for (int j = 0; j < i; j++)
                if (packet.heroOwner[j] == packet.heroOwner[i])
                    remoteIndex++;

            auto& remote = remotePlayers.at(packet.heroOwner[i]);
            if (remoteIndex < (int)remote.getHeroes().size())
            {
                remote.getLinkTOHeroes()[remoteIndex].setCurrentHp(packet.heroes[i].currentHp);
                remote.getLinkTOHeroes()[remoteIndex].setCurrentMp(packet.heroes[i].currentMana);
            }
        }
    }

    // остаток функции без изменений
    for (int i = 0; i < packet.enemyCount && i < (int)enemies.size(); i++)
        enemies[i].setCurrentHp(packet.enemies[i].currentHp);

    auto newState = static_cast<BattleState>(packet.battleState);

    currentActionIndex = packet.currentActionIndex;
    currentTurn        = static_cast<TurnOwner>(packet.currentTurn);

    if (newState == BattleState::ExecuteActions)
        return;

    if (newState == BattleState::Animation)
    {
        state = BattleState::Animation;
        return;
    }

    state = newState;
    currentHeroIndex = packet.currentHeroIndex;

    if (state == BattleState::PlayerChoose && !game->isHost())
        startPlayerChoose();
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
    readyPlayers.clear(); // ← добавить
    currentHeroIndex = 0;
    playersReady = 0;
    state = BattleState::PlayerChoose;

    while (currentHeroIndex < player->getHeroes().size() &&
           player->getHeroes()[currentHeroIndex].getCurrentHp() <= 0)
        currentHeroIndex++;
}

Hero* Battle::resolveActor(const Action& action)
{
    // локальный игрок (или офлайн)
    if (!game->isConnected() || action.playerId == game->localPlayerId)
    {
        auto& heroes = player->getLinkTOHeroes();
        if (action.actorIndex >= 0 && action.actorIndex < (int)heroes.size())
            return &heroes[action.actorIndex];
        return nullptr;
    }

    // удалённый игрок
    auto& remotePlayers = game->getRemotePlayersMutable();
    if (!remotePlayers.contains(action.playerId))
        return nullptr;

    auto& heroes = remotePlayers.at(action.playerId).getLinkTOHeroes();
    if (action.actorIndex >= 0 && action.actorIndex < (int)heroes.size())
        return &heroes[action.actorIndex];

    return nullptr;
}

void Battle::confirmAction(const Action& action)
{
    if (action.actorIndex != currentHeroIndex)
        return;

    Action actionWithId = action;
    actionWithId.playerId = game->localPlayerId;
    plannedActions.push_back(actionWithId);

    if (game->isConnected())
        game->sendBattleAction(actionWithId);

    currentHeroIndex++;
    while (currentHeroIndex < player->getHeroes().size() &&
           player->getHeroes()[currentHeroIndex].getCurrentHp() <= 0)
        currentHeroIndex++;

    if (currentHeroIndex >= player->getHeroes().size())
    {
        readyPlayers.insert(game->localPlayerId); // ← добавить
        playersReady++;
        state = BattleState::WaitingForHost;

        if ((game->isHost() && playersReady >= expectedPlayers) ||
            (!game->isHost() && !game->isConnected()))
        {
            playersReady = 0;
            readyPlayers.clear();
            startExecuteActions();
        }
    }
}

void Battle::updateExecuteActions()
{
    if (state != BattleState::ExecuteActions)
        return;

    if (game->isConnected() && !game->isHost())
        return;

    if (currentActionIndex >= (int)plannedActions.size())
    {
        plannedActions.clear();
        currentActionIndex = 0;
        state = BattleState::Result;
        if (game->isConnected()) game->sendBattleSnapshot();
        return;
    }

    const Action& action = plannedActions[currentActionIndex];

    Hero* actor = resolveActor(action);
    if (!actor || actor->getCurrentHp() <= 0)
    {
        currentActionIndex++;
        return;
    }

    executeAction(action);

    /*bool isLocal = (action.playerId == game->localPlayerId || !game->isConnected());
    std::string animPrefix = isLocal
        ? "Hero"
        : "RemoteHero";

    ui->playAnimation(
        animPrefix + std::to_string(action.actorIndex + 1) + "Animation",
        "Battle", true
    );*/

    currentActionIndex++;
    currentTurn = TurnOwner::Player;
    state = BattleState::Animation;

    if (game->isConnected()) game->sendBattleSnapshot();
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
    Hero* actor = resolveActor(action); // ← через resolveActor
    if (!actor || actor->getCurrentHp() <= 0)
        return;

    auto targets = resolveTargets(action);
    if (targets.empty())
        return;

    switch (action.type)
    {
        case ActionType::Attack:
            executeAttack(*actor, targets);
            break;
        case ActionType::Magic:
            executeSkill(*actor, action.payloadId, targets, true);
            break;
        case ActionType::Item:
            executeItem(*actor, targets, action.payloadId);
            break;
        case ActionType::Skip:
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

    // ✅ Клиент не считает — ждёт снапшот от хоста
    if (game->isConnected() && !game->isHost())
        return;

    if (currentEnemyIndex >= enemies.size())
    {
        state = BattleState::PlayerChoose;
        endRound();
        startPlayerChoose();
        game->sendBattleSnapshot(); // ✅ рассылаем после хода всех врагов
        return;
    }

    Enemy& enemy = enemies[currentEnemyIndex];

    if (enemy.getCurrentHp() <= 0)
    {
        currentEnemyIndex++;
        return;
    }

    int targetIndex = selectHeroTarget();
    if (targetIndex == -1)
    {
        state = BattleState::Result;
        return;
    }

    uint32_t damage = enemy.getAttackPower(nullptr);

    if (targetIndex >= 0)
    {
        // ✅ Локальный герой
        Hero& target = player->getLinkTOHeroes()[targetIndex];
        target.takeDamage(damage);
    }
    else
    {
        // ✅ Удалённый герой — декодируем индекс
        int encoded = -targetIndex - 1;
        uint32_t playerId = encoded / 100;
        int heroIndex = encoded % 100;

        auto& remotePlayers = game->getRemotePlayersMutable();
        if (remotePlayers.contains(playerId))
            remotePlayers.at(playerId).getLinkTOHeroes()[heroIndex].takeDamage(damage);
    }

    currentEnemyIndex++;
    currentTurn = TurnOwner::Enemy;
    state = BattleState::Animation;

    game->sendBattleSnapshot();
}

int Battle::selectHeroTarget() const
{
    // Собираем всех живых героев — локальных и удалённых
    int totalHeroes = 0;

    for (int i = 0; i < (int)player->getHeroes().size(); i++)
        if (player->getHeroes()[i].getCurrentHp() > 0)
            totalHeroes++;

    for (auto& [id, remote] : game->getRemotePlayers())
        for (int i = 0; i < (int)remote.getHeroes().size(); i++)
            if (remote.getHeroes()[i].getCurrentHp() > 0)
                totalHeroes++;

    if (totalHeroes == 0)
        return -1;

    // Случайная цель среди всех живых
    int roll = rand() % totalHeroes;
    int count = 0;

    for (int i = 0; i < (int)player->getHeroes().size(); i++)
    {
        if (player->getHeroes()[i].getCurrentHp() > 0)
        {
            if (count == roll)
                return i; // локальный герой
            count++;
        }
    }

    // Если цель — удалённый игрок, возвращаем отрицательный индекс
    // -1000 - playerId * 100 - heroIndex
    for (auto& [id, remote] : game->getRemotePlayers())
    {
        for (int i = 0; i < (int)remote.getHeroes().size(); i++)
        {
            if (remote.getHeroes()[i].getCurrentHp() > 0)
            {
                if (count == roll)
                    return -(int)(id * 100 + i); // закодированный индекс
                count++;
            }
        }
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

    // Восстанавливаем спрайт только если атаковал герой
    if (currentTurn == TurnOwner::Player && !plannedActions.empty())
    {
        const auto& action = plannedActions[currentActionIndex - 1];

        bool isMyAction = (action.playerId == game->localPlayerId
                           || !game->isConnected());
        std::string prefix = isMyAction ? "Hero" : "RemoteHero";

        const std::string heroId = prefix + std::to_string(action.actorIndex + 1);
        ui->setEnVI(heroId, "Battle", true); // восстанавливаем правильный спрайт
    }

    isAnimationsStarted = false;

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
                if (currentTurn == TurnOwner::Player && !plannedActions.empty() && currentActionIndex > 0
                    && currentActionIndex <= (int)plannedActions.size())
                {
                    const auto& action = plannedActions[currentActionIndex - 1];

                    // определяем чей герой — как в updateExecuteActions
                    bool isMyAction = (action.playerId == game->localPlayerId
                                       || !game->isConnected());
                    std::string prefix = isMyAction ? "Hero" : "RemoteHero";

                    const std::string heroId = prefix + std::to_string(action.actorIndex + 1);
                    ui->setEnVI(heroId, "Battle", false);
                    ui->playAnimation(heroId + "Animation", "Battle", true);
                }
                isAnimationsStarted = true;
            }

            onAnimationFinished();
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
        auto &heroes = player->getLinkTOHeroes();
        for (int i = 0; i < heroes.size(); i++)
            heroes.at(i).addXP(sumExp/heroes.size());
    }
    else {
        //sd
    }

    currentHeroIndex = 0;
    game->battleStarted = false;
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
    escapeChance = 10;

    spawnEnemies();
    prepairUI();
    determineFirstTurn();

    expectedPlayers = game->isConnected() ? 2 : 1;
    playersReady = 0;

    //if (firstTurnIsPlayer) {
        currentTurn = TurnOwner::Player;
        state = BattleState::PlayerChoose;
   /* } else {
        currentTurn = TurnOwner::Enemy;
        state = BattleState::EnemyTurn;
    }*/
}

void Battle::resetState()
{
    enemies.clear();
    plannedActions.clear();
    readyPlayers.clear();
    playersReady = 0;
    currentHeroIndex = 0;
    currentActionIndex = 0;
    currentEnemyIndex = 0;
    state = BattleState::PlayerChoose;
}