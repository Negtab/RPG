#include "battle.h"
#include <algorithm>
#include <chrono>

#include "gameData.h"
#include "../main/game.h"

namespace {
// Обычный удар: атака минус половина защиты, разброс ±10%, но не меньше 1.
// Магия и предметы защиту не учитывают, как в FF1
uint32_t physicalDamage(uint32_t attack, uint32_t defence)
{
    const int64_t base = std::max<int64_t>(1, static_cast<int64_t>(attack) - defence / 2);
    const int64_t spread = 90 + rand() % 21;
    return static_cast<uint32_t>(std::max<int64_t>(1, base * spread / 100));
}
}

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
    ui->hideBattleResults();

    for (int i = 0; i < packet.enemyCount && i < 4; i++)
    {
        const int id = packet.enemies[i].enemyId;           // теперь это id из enemies.json
        const int32_t maxHp = packet.enemies[i].maxHp;
        if (!GameData::instance().enemy(id) || maxHp <= 0 || maxHp > 10000000)
            continue;                                         // неизвестный id или мусор из сети

        enemies.emplace_back(id, player->getAverageLevel());
        enemies.back().setMaxHp(static_cast<uint32_t>(maxHp));
        enemies.back().setCurrentHp(static_cast<uint32_t>(std::max<int32_t>(0, packet.enemies[i].currentHp)));
    }

    prepairUI();

    beginParticipants();

    currentTurn = TurnOwner::Player;
    state = BattleState::PlayerChoose;
    startPlayerChoose();
}

void Battle::confirmActionFromNetwork(const Action& action)
{
    if (action.playerId == game->localPlayerId)
        return;

    const auto& remotePlayers = game->getRemotePlayers();
    if (!remotePlayers.contains(action.playerId) || !participants.contains(action.playerId))
    {
        SDL_Log("Action from unknown or non-participant playerId=%u, skipping", action.playerId);
        return;
    }

    // payloadId пришёл по сети: не доверяем
    if (action.type == ActionType::Magic && !game->findSkill(action.payloadId)) return;
    if (action.type == ActionType::Item  && !game->findItem(action.payloadId))  return;

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
    if (packet.battleState > static_cast<uint8_t>(BattleState::WaitingForHost) ||
    packet.currentTurn > static_cast<uint8_t>(TurnOwner::Enemy))
        return;                                                    // мусор

    const int heroCount  = std::min<int>(packet.heroCount, 8);
    const int enemyCount = std::min<int>({packet.enemyCount, 4, static_cast<int>(enemies.size())});

    for (int i = 0; i < heroCount; i++)
    {
        const uint32_t owner = packet.heroOwner[i];

        int index = 0;                                   // какой по счёту герой этого владельца
        for (int j = 0; j < i; j++)
            if (packet.heroOwner[j] == owner)
                index++;

        std::vector<Hero>* party = nullptr;
        if (owner == game->localPlayerId)
            party = &player->getLinkTOHeroes();
        else
        {
            auto& remotes = game->getRemotePlayersMutable();
            auto it = remotes.find(owner);
            if (it != remotes.end())
                party = &it->second.getLinkTOHeroes();
        }

        if (party && index < static_cast<int>(party->size()))
        {
            (*party)[index].setCurrentHp(packet.heroes[i].currentHp);
            (*party)[index].setCurrentMp(packet.heroes[i].currentMana);
        }
    }

    for (int i = 0; i < enemyCount; i++)         // было: i < packet.enemyCount && i < enemies.size()
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

    skipDeadHeroes();                                  // павший первый герой ход не держит
}

void Battle::skipDeadHeroes()
{
    while (currentHeroIndex < static_cast<int>(player->getHeroes().size()) &&
           player->getHeroes()[currentHeroIndex].getCurrentHp() <= 0)
        currentHeroIndex++;

    if (currentHeroIndex < static_cast<int>(player->getHeroes().size()))
        return;

    // Все свои уже выбрали или все пали
    if (game->isConnected())
    {
        bool sentAny = false;
        for (const auto& a : plannedActions)
            if (a.playerId == game->localPlayerId) { sentAny = true; break; }
        if (!sentAny)                                  // свои все пали: хост ждёт от нас хотя бы одно действие
        {
            Action skip{};
            skip.type = ActionType::Skip;
            skip.playerId = game->localPlayerId;
            plannedActions.push_back(skip);
            game->sendBattleAction(skip);
        }
    }

    readyPlayers.insert(game->localPlayerId);
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

Hero* Battle::resolveActor(const Action& action)
{
    auto* party = partyOf(action.playerId);
    if (!party || action.actorIndex < 0 || action.actorIndex >= static_cast<int>(party->size()))
        return nullptr;
    return &(*party)[action.actorIndex];
}

void Battle::confirmAction(const Action& action)
{
    if (action.actorIndex != currentHeroIndex)
        return;

    Action actionWithId = action;
    actionWithId.playerId = game->localPlayerId;
    plannedActions.push_back(actionWithId);

    // хост не знает инвентарь клиента, поэтому клиент списывает свой предмет сам
    if (action.type == ActionType::Item && game->isConnected() && !game->isHost())
        player->consumeItem(action.payloadId);

    if (game->isConnected())
        game->sendBattleAction(actionWithId);

    currentHeroIndex++;
    skipDeadHeroes();
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

void Battle::executeAttack(Hero& attacker, const std::vector<int>& targets)
{
    for (int idx : targets)
    {
        Enemy& target = enemies[idx];
        target.takeDamage(physicalDamage(attacker.getAttackPower(nullptr), target.getDefense()));
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
            executeSkill(*actor, action.payloadId, targets, true, action.playerId);
            break;
        case ActionType::Item:
            executeItem(*actor, targets, action.payloadId, action.playerId);
            break;
        case ActionType::Skip:
            break;
    }

    currentTurn = TurnOwner::Player;
    state = BattleState::Animation;
}

void Battle::executeSkill(GamePerson& caster, int skillId, const std::vector<int>& targets,
                          bool casterIsHero, uint32_t owner)
{
    const Skill* skill = game->findSkill(skillId);
    auto* party = partyOf(owner);
    if (!skill || !party || !caster.canCast(*skill))
        return;

    caster.spendMana(skill->manaCost);

    const auto validEnemy = [&](int i) { return i >= 0 && i < static_cast<int>(enemies.size()); };
    const auto validHero  = [&](int i) { return i >= 0 && i < static_cast<int>(party->size()); };

    for (int idx : targets)
    {
        switch (skill->effectType)
        {
            case SkillEffectType::Damage:
                if (casterIsHero) { if (validEnemy(idx)) enemies[idx].takeDamage(skill->power, skill->element); }
                else              { if (validHero(idx))  (*party)[idx].takeDamage(skill->power, skill->element); }
                break;

            case SkillEffectType::Heal:
                if (casterIsHero) { if (validHero(idx))  (*party)[idx].heal(skill->power); }
                else              { if (validEnemy(idx)) enemies[idx].heal(skill->power); }
                break;

            default:
                break;
        }
    }

    caster.setCooldown(skill->id, skill->maxCooldown);
}

void Battle::executeItem(Hero& /*user*/, const std::vector<int>& targets, int itemId, uint32_t owner)
{
    const Item* item = game->findItem(itemId);
    auto* party = partyOf(owner);
    if (!item || !party) return;

    // Инвентарь удалённого игрока хост не видит: свой предмет проверяется и списывается здесь,
    // а чужой списывает сам владелец в confirmAction.
    const bool mine = !game->isConnected() || owner == game->localPlayerId;
    if (mine && !player->hasItem(itemId)) return;

    for (int idx : targets)
    {
        switch (item->effectType)
        {
            case ItemEffectType::Heal:
                if (idx >= 0 && idx < static_cast<int>(party->size())) (*party)[idx].heal(item->power);
                break;
            case ItemEffectType::Damage:
                if (idx >= 0 && idx < static_cast<int>(enemies.size())) enemies[idx].takeDamage(item->power);
                break;
            case ItemEffectType::RestoreMana:
                if (idx >= 0 && idx < static_cast<int>(party->size())) (*party)[idx].restoreMana(item->power);
                break;
            default:
                break;
        }
    }

    if (mine && item->consumable)
        player->consumeItem(itemId);
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

    attacker.takeDamage(physicalDamage(target.getAttackPower(nullptr) / 2, attacker.getDefense()));
}

void Battle::startEnemyTurn()
{
    currentTurn = TurnOwner::Enemy;
    state = BattleState::EnemyTurn;
    currentEnemyIndex = 0;
}


void Battle::updateEnemyTurn()
{
    if (state != BattleState::EnemyTurn) return;
    if (game->isConnected() && !game->isHost()) return;     // клиент ждёт снапшот от хоста

    if (currentEnemyIndex >= static_cast<int>(enemies.size()))
    {
        endRound();
        if (aliveHeroes().empty())                          // герои погибли в конце хода врагов
        {
            state = BattleState::Result;
            game->sendBattleSnapshot();
            return;
        }
        startPlayerChoose();
        game->sendBattleSnapshot();
        return;
    }

    if (enemies[currentEnemyIndex].getCurrentHp() <= 0) { currentEnemyIndex++; return; }

    if (!performEnemyAction(currentEnemyIndex))
    {
        state = BattleState::Result;                        // целей нет
        game->sendBattleSnapshot();
        return;
    }

    currentEnemyIndex++;
    currentTurn = TurnOwner::Enemy;
    state = BattleState::Animation;
    game->sendBattleSnapshot();
}

bool Battle::performEnemyAction(int idx)
{
    Enemy& enemy = enemies[idx];
    const EnemyDef* def = GameData::instance().enemy(enemy.getEnemyId());
    if (!def) return !aliveHeroes().empty();

    std::vector<const EnemyAbility*> usable;                // обычный удар доступен всегда, умения только без кулдауна
    int total = 0;
    for (const auto& a : def->abilities)
        if (a.skillId < 0 || enemy.isSkillReady(a.skillId))
        {
            usable.push_back(&a);
            total += a.weight;
        }
    if (total <= 0) return enemyBasicAttack(enemy, def->hits);

    int roll = rand() % total;
    for (const EnemyAbility* a : usable)
    {
        if (roll < a->weight)
            return a->skillId < 0 ? enemyBasicAttack(enemy, def->hits) : enemyUseSkill(idx, a->skillId);
        roll -= a->weight;
    }
    return enemyBasicAttack(enemy, def->hits);
}

bool Battle::enemyBasicAttack(Enemy& enemy, int hits)
{
    for (int i = 0; i < std::max(1, hits); ++i)
    {
        const auto target = selectHeroTarget();
        if (!target) return i > 0;                          // героев нет вообще: false; погибли за время ударов: true
        if (Hero* hero = heroByRef(*target))
            hero->takeDamage(physicalDamage(enemy.getAttackPower(nullptr), hero->getDefense()));
    }
    return true;
}

bool Battle::enemyUseSkill(int idx, int skillId)
{
    const Skill* skill = game->findSkill(skillId);
    const auto heroes = aliveHeroes();
    if (!skill) return !heroes.empty();

    std::vector<int> aliveEnemies;
    for (int i = 0; i < static_cast<int>(enemies.size()); ++i)
        if (enemies[i].getCurrentHp() > 0) aliveEnemies.push_back(i);

    std::vector<HeroRef> heroTargets;
    std::vector<int> enemyTargets;

    switch (skill->targetType)                              // для врага «Enemy» значит «герои», «Ally» значит «другие враги»
    {
    case TargetType::Enemy:
        if (heroes.empty()) return false;
        heroTargets.push_back(heroes[rand() % heroes.size()]);
        break;
    case TargetType::AllEnemies:
        if (heroes.empty()) return false;
        heroTargets = heroes;
        break;
    case TargetType::Ally:
        if (!aliveEnemies.empty()) enemyTargets.push_back(aliveEnemies[rand() % aliveEnemies.size()]);
        break;
    case TargetType::AllAllies:
        enemyTargets = aliveEnemies;
        break;
    case TargetType::Self:
        enemyTargets.push_back(idx);
        break;
    }

    const uint32_t power = static_cast<uint32_t>(std::max(0, skill->power));
    switch (skill->effectType)
    {
    case SkillEffectType::Damage:
        for (const HeroRef& ref : heroTargets)
            if (Hero* hero = heroByRef(ref)) hero->takeDamage(power, skill->element);
        break;
    case SkillEffectType::Heal:
        for (int i : enemyTargets) enemies[i].heal(power);
        break;
    default:                                            // Buff и Debuff пока без эффекта
        break;
    }

    enemies[idx].setCooldown(skill->id, skill->maxCooldown);
    return true;
}

std::optional<HeroRef> Battle::selectHeroTarget() const
{
    const auto alive = aliveHeroes();
    if (alive.empty()) return std::nullopt;
    return alive[rand() % alive.size()];
}

int Battle::averageLevel() const
{
    int sum = 0, count = 0;
    for (const auto& h : player->getHeroes()) { sum += h.getLevel(); ++count; }
    for (const auto& [id, remote] : game->getRemotePlayers())
        for (const auto& h : remote.getHeroes()) { sum += h.getLevel(); ++count; }
    return count > 0 ? std::max(1, sum / count) : 1;
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
            auto* party = partyOf(action.playerId);
            if (party && action.targetIndex >= 0 &&
                action.targetIndex < static_cast<int>(party->size()) &&
                (*party)[action.targetIndex].getCurrentHp() > 0)
                result.push_back(action.targetIndex);
            break;
        }

        case TargetType::AllAllies:
        {
            if (auto* party = partyOf(action.playerId))
                for (int i = 0; i < static_cast<int>(party->size()); ++i)
                    if ((*party)[i].getCurrentHp() > 0)
                        result.push_back(i);
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
    }

    return result;
}


void Battle::onAnimationFinished()
{
    if (!ui->isAnimationFinished("Battle"))
        return;
    if (currentTurn == TurnOwner::Enemy && SDL_GetTicks() - animStartTicks < 700)
        return;                                        // иначе все враги бьют за пару кадров и числа урона сливаются

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
    const bool heroesAlive = !aliveHeroes().empty();
    bool enemiesAlive = false;

    for (const auto& e : enemies)
        if (e.getCurrentHp() > 0) { enemiesAlive = true; break; }

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
                animStartTicks = SDL_GetTicks();
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


void Battle::spawnEnemies()
{
    const auto& data = GameData::instance();
    const int avgLvl = averageLevel();

    for (int id : enemyFormation)                            // 1) готовая формация из encounters.json
    {
        if (static_cast<int>(enemies.size()) >= 4) break;    // в интерфейсе четыре слота
        if (data.enemy(id)) enemies.emplace_back(id, avgLvl);
    }
    if (!enemies.empty()) return;

    const auto& all = data.enemyIds();                       // 2) запасной вариант: случайные
    const int enemyCount = 1 + rand() % std::min(3, avgLvl + 1);
    for (int i = 0; i < enemyCount; i++)
    {
        int id = enemyPool.empty() ? all[rand() % all.size()] : enemyPool[rand() % enemyPool.size()];
        if (!data.enemy(id)) id = all[rand() % all.size()];
        enemies.emplace_back(id, avgLvl);
    }
}

std::vector<HeroRef> Battle::aliveHeroes() const
{
    std::vector<HeroRef> alive;

    const auto& mine = player->getHeroes();
    for (int i = 0; i < static_cast<int>(mine.size()); ++i)
        if (mine[i].getCurrentHp() > 0)
            alive.push_back({game->localPlayerId, i});

    for (const auto& [id, remote] : game->getRemotePlayers())
    {
        if (!participants.contains(id)) continue;
        const auto& heroes = remote.getHeroes();
        for (int i = 0; i < static_cast<int>(heroes.size()); ++i)
            if (heroes[i].getCurrentHp() > 0)
                alive.push_back({id, i});
    }
    return alive;
}


void Battle::determineFirstTurn()
{
    firstTurnIsPlayer = (rand() % 100) < 30;
}

void Battle::prepairUI()
{
    for (int i = 0; i < static_cast<int>(enemies.size()); i++)
        if (const EnemyDef* def = GameData::instance().enemy(enemies[i].getEnemyId()))
            ui->setSrcRect("Enemy" + std::to_string(i + 1), "Battle", def->srcRect);
}

void Battle::run(const std::vector<int>& formation, const std::vector<int>& pool)
{
    enemies.clear();
    rewards.clear();
    isWin = false;
    escapeChance = 10;
    ui->hideBattleResults();

    enemyFormation = formation;
    enemyPool = pool;
    spawnEnemies();
    prepairUI();
    determineFirstTurn();

    expectedPlayers = game->isConnected() ? 2 : 1;
    playersReady = 0;

    //if (firstTurnIsPlayer) {
        startPlayerChoose();                          // а не просто state = PlayerChoose: пропустит павших героев
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

void Battle::beginParticipants()
{
    participants.clear();
    participants.insert(game->localPlayerId);
    for (const auto& entry : game->getRemotePlayers())
        participants.insert(entry.first);
    expectedPlayers = static_cast<int>(participants.size());
    playersReady = 0;
}

std::vector<Hero>* Battle::partyOf(uint32_t owner)
{
    if (!game->isConnected() || owner == game->localPlayerId)
        return &player->getLinkTOHeroes();

    auto& remotes = game->getRemotePlayersMutable();
    auto it = remotes.find(owner);
    return it != remotes.end() ? &it->second.getLinkTOHeroes() : nullptr;
}

Hero* Battle::heroByRef(const HeroRef& ref)
{
    auto* party = partyOf(ref.owner);
    if (!party || ref.index < 0 || ref.index >= static_cast<int>(party->size()))
        return nullptr;
    return &(*party)[ref.index];
}

void Battle::onPlayerLeft(uint32_t id)
{
    participants.erase(id);
    expectedPlayers = std::max(1, static_cast<int>(participants.size()));

    if (readyPlayers.erase(id) > 0 && playersReady > 0)
        --playersReady;

    // Пока выбирают действия, его выбор не нужен. Во время выполнения plannedActions не трогаем
    // (currentActionIndex), его действия пропустит resolveActor.
    if (state == BattleState::PlayerChoose || state == BattleState::WaitingForHost)
        std::erase_if(plannedActions, [&](const Action& a) { return a.playerId == id; });

    // Все оставшиеся уже готовы: хост запускает выполнение
    if (game->isHost() && state == BattleState::WaitingForHost && playersReady >= expectedPlayers)
    {
        playersReady = 0;
        readyPlayers.clear();
        startExecuteActions();
    }
}

void Battle::finishBattle(const bool& win)
{
    isWin = win;
    if (!win)                                  // поражение или побег: сразу на карту
    {
        leaveBattle();
        return;
    }

    report = BattleReport{};
    uint32_t totalExp = 0, totalGold = 0;
    for (const auto& e : enemies) { totalExp += e.getExperience(); totalGold += e.getGold(); }

    const size_t living  = std::max<size_t>(1, aliveHeroes().size());     // опыт делят живые герои всех игроков
    const size_t sharers = std::max<size_t>(1, participants.size());      // золото делят игроки

    report.exp     = totalExp;
    report.expEach = static_cast<uint32_t>(totalExp / living);
    report.gold    = static_cast<uint32_t>(totalGold / sharers);

    for (auto& h : player->getLinkTOHeroes())
        if (h.getCurrentHp() > 0)
        {
            const int before = h.getLevel();
            h.addXP(report.expEach);
            if (h.getLevel() > before)
                report.levelUps.push_back({h.getName(), h.getLevel()});
        }
    player->setPlayerGold(player->getPlayerGold() + report.gold);

    // Трофеи: за каждого врага каждая строка таблицы бросается отдельно; у каждого игрока свои
    std::map<int, int> found;
    for (const auto& e : enemies)
        for (const Drop& d : GameData::instance().drops(e.getEnemyId()))
            if (rand() % 100 < d.chance) ++found[d.itemId];
    for (const auto& [id, count] : found)
    {
        player->addItem(id, static_cast<uint8_t>(count));
        const Item* item = game->findItem(id);
        report.items.push_back((item ? item->name : "Item") + (count > 1 ? " x" + std::to_string(count) : ""));
    }

    game->sendPartyInfo();
    state = BattleState::Victory;              // ждём подтверждения
    ui->showBattleResults(report);
}

void Battle::closeResults()
{
    if (state != BattleState::Victory) return;
    ui->hideBattleResults();
    leaveBattle();
}

void Battle::leaveBattle()
{
    currentHeroIndex = 0;
    game->battleStarted = false;
    game->sendPartyInfo();                     // новые уровни и HP видны напарнику
    game->setGameState(GameState::Map);
}