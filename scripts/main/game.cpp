#include "game.h"

#include <chrono>
#include <filesystem>

#include "inputController.h"

#include <SDL.h>
#include <ws2tcpip.h>


Game::Game(SDL_Window *window, SDL_Renderer* renderer) : window(window), renderer(renderer)
{
    players.emplace_back("Test");

    resourceManager = std::make_unique<ResourceManager>(*renderer);

    visualizer = std::make_unique<Visualizer>(renderer);

    audioManager = std::make_unique<AudioManager>();

    uiManager = std::make_unique<UIManager>(*visualizer, *audioManager, *resourceManager, *this, players.at(0));

    inputManager = std::make_unique<InputManager>();
    inputController = std::make_unique<InputController>(*inputManager);

    battle = std::make_unique<Battle>(&players.at(0), uiManager.get(), this);

    resourceManager->initialize();
    uiManager->initialize();

    initItems();
    initSkills();

    SDL_SetWindowIcon(window, ResourceManager::getImage("Sword"));

    this->isRunning = true;
    this->setGameState(GameState::Menu);
}

void Game::run()
{
    SDL_Event event;

    srand(static_cast<unsigned int>(time(nullptr)));

    pathToSave = std::filesystem::current_path().remove_filename().string() + "save.bin";
    if (resourceManager->isPlayerNew(pathToSave))
        uiManager->setEnVI("Continue", "Menu", false);
    else
    {
        uiManager->setEnVI("Continue", "Menu", true);
        resourceManager->loadPlayer(pathToSave, players.at(0));
    }

    while (this->isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                this->isRunning = false;
                break;
            }
            inputManager->processEvent(event);
            handleInput(event); // обработка клавиш и кликов
        }

        float dt = getDeltaTime();

        update(dt);   // логика
        render();   // отрисовка
        inputManager->update();

        updateFPS();
    }
    resourceManager->savePlayer(pathToSave, players.at(0));
}

float Game::getDeltaTime()
{
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    float dt = 0.0f;
    if (lastCounter != 0)
        dt = static_cast<float>(now - lastCounter) / freq;

    // Ограничение FPS
    float targetDt = 1.0f / targetFPS;
    if (dt < targetDt)
        SDL_Delay(static_cast<Uint32>((targetDt - dt) * 1000.0f)); // перевод в мс

    lastCounter = SDL_GetPerformanceCounter(); // обновляем после задержки
    dt = static_cast<float>(lastCounter - now) / freq; // пересчёт dt после Delay

    return dt;
}

void Game::updateFPS()
{
    frameCount++;

    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    if (now - fpsLastCounter >= freq) // прошло >=1 сек
    {
        currentFPS = frameCount;
        frameCount = 0;
        fpsLastCounter = now;
    }
}

int Game::getCurrentFPS() const { return currentFPS; }

std::string Game::getCurrentTime() {
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t now_time = system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &now_time);  // Windows
#else
    localtime_r(&now_time, &local_tm);  // Linux / macOS
#endif

    char buffer[6]; // HH:MM
    std::strftime(buffer, sizeof(buffer), "%H:%M", &local_tm);

    return std::string(buffer);
}

void Game::sendLocalPlayerState()
{
    if (!isConnected()) return;
    if (players.empty())
        return;

    Player& player = players[0];

    PlayerMovePacket packet{};

    packet.header.type = PacketType::PlayerMove;
    packet.header.size = sizeof(PlayerMovePacket);

    packet.playerId = localPlayerId;

    packet.x = player.getPlayerCoords().x;
    packet.y = player.getPlayerCoords().y;

    packet.direction = player.getDirection();

    client.send(reinterpret_cast<const char*>(&packet), sizeof(packet));
}

void Game::sendBattleEnd(bool isWin)
{
    if (!isConnected()) return;

    BattleEndPacket packet{};
    packet.header.type = PacketType::BattleEnd;
    packet.header.size = sizeof(BattleEndPacket);
    packet.isWin = isWin ? 1 : 0;

    client.send(reinterpret_cast<const char*>(&packet), sizeof(packet));
}

void Game::processMovePacket(const PlayerMovePacket& packet)
{
    if (packet.playerId == localPlayerId)
        return;

    if (!remotePlayers.contains(packet.playerId))
    {
        Player player("RemotePlayer");
        player.setRole(NetworkRole::Remote);
        player.setNetworkId(packet.playerId);

        // ✅ Инициализируем анимации
        player.animPlayer.animations["MoveLeftAnimation"]  = { 0.2f, ResourceManager::getAnimation("MoveLeftAnimation"),  {}, {0,0,45,45}, 0, 0.f, false, AnimPlayMode::Loop };
        player.animPlayer.animations["MoveRightAnimation"] = { 0.2f, ResourceManager::getAnimation("MoveRightAnimation"), {}, {0,0,45,45}, 0, 0.f, false, AnimPlayMode::Loop };
        player.animPlayer.animations["MoveUpAnimation"]    = { 0.2f, ResourceManager::getAnimation("MoveUpAnimation"),    {}, {0,0,45,45}, 0, 0.f, false, AnimPlayMode::Loop };
        player.animPlayer.animations["MoveDownAnimation"]  = { 0.2f, ResourceManager::getAnimation("MoveDownAnimation"),  {}, {0,0,45,45}, 0, 0.f, false, AnimPlayMode::Loop };

        remotePlayers.emplace(packet.playerId, std::move(player));
    }

    Player& remote = remotePlayers.at(packet.playerId);
    remote.setPlayerCoords({ packet.x, packet.y });
    remote.setDirection(packet.direction);
}

void Game::processPacket(const char* data)
{
    auto* header = reinterpret_cast<const PacketHeader*>(data);

    switch (header->type)
    {
        case PacketType::Connect:
        {
            auto* packet = reinterpret_cast<const ConnectPacket*>(data);
            localPlayerId = packet->assignedId;
            SDL_Log("Player connect");
            break;
        }

        case PacketType::PlayerMove:
        {
            auto* packet = reinterpret_cast<const PlayerMovePacket*>(data);
            processMovePacket(*packet);
            break;
        }

        case PacketType::PlayerJoined:
        {
            auto* packet = reinterpret_cast<const PlayerJoinedPacket*>(data);
            SDL_Log("Player joined: %d", packet->playerId);
            break;
        }

        case PacketType::Disconnect:
        {
            auto* packet = reinterpret_cast<const DisconnectPacket*>(data);
            remotePlayers.erase(packet->playerId);
            SDL_Log("Player disconnected: %d", packet->playerId);
            break;
        }

        case PacketType::BattleStart:
        {
            auto* packet = reinterpret_cast<const BattleStartPacket*>(data);

            if (!battleStarted)
            {
                battleStarted = true;
                battle->runFromNetwork(*packet);

                setPreviousGameState(GameState::Map);
                setGameState(GameState::Battle);

                uiManager->addEnemys();
                uiManager->addCharacters();
            }
            break;
        }

        case PacketType::BattleAction:
        {
            auto* packet = reinterpret_cast<const BattleActionPacket*>(data);

            if (packet->playerId == localPlayerId)
                break;

            Action action{};
            action.type        = static_cast<ActionType>(packet->actionType);
            action.targetType  = static_cast<TargetType>(packet->targetType);
            action.actorIndex  = packet->actorIndex;
            action.targetIndex = packet->targetIndex;
            action.payloadId   = packet->payloadId;
            action.playerId    = packet->playerId;

            battle->confirmActionFromNetwork(action);
            break;
        }

        case PacketType::BattleSnapshot:
        {
            auto* packet = reinterpret_cast<const BattleSnapshotPacket*>(data);
            battle->applySnapshot(*packet);
            break;
        }

        case PacketType::BattleEnd:
        {
            auto* packet = reinterpret_cast<const BattleEndPacket*>(data);

            battleStarted = false;
            battle->resetState();
            setGameState(getPrevGameState());
            setPreviousGameState(GameState::Battle);

            SDL_Log("BattleEnd received: isWin=%d", packet->isWin);
            break;
        }

        default:
            SDL_Log("Unknown packet type: %d", (int)header->type);
            break;
    }
}

void Game::updateNetwork()
{
    char chunk[2048];
    int received = client.receive(chunk, sizeof(chunk));

    if (received > 0)
        netRecvBuffer.insert(netRecvBuffer.end(), chunk, chunk + received);

    int offset = 0;
    const int total = (int)netRecvBuffer.size();

    while (offset + (int)sizeof(PacketHeader) <= total)
    {
        auto* header = reinterpret_cast<PacketHeader*>(netRecvBuffer.data() + offset);

        if (header->size < sizeof(PacketHeader) || header->size > 4096)
        {
            SDL_Log("Invalid packet size: %d — clearing buffer", header->size);
            netRecvBuffer.clear();
            return;
        }

        // пакет ещё не пришёл целиком — ждём следующего кадра
        if (offset + (int)header->size > total)
            break;

        // обрабатываем пакет
        processPacket(netRecvBuffer.data() + offset);
        offset += header->size;
    }

    // убираем обработанные байты
    if (offset > 0)
        netRecvBuffer.erase(netRecvBuffer.begin(), netRecvBuffer.begin() + offset);
}

const std::unordered_map<uint32_t, Player>& Game::getRemotePlayers() const noexcept
{
    return remotePlayers;
}

void Game::startGame()
{
    this->setPreviousGameState(GameState::Menu);
    this->setGameState(GameState::Map);
}

void Game::endGame()
{
    this->isRunning = false;
}


void Game::handleInput(const SDL_Event &event)
{
    inputController->chooseInput(event, *this, *this->getBattle(), players.at(0), *this->uiManager);
}

void Game::update(float dt)
{
    if (state == GameState::Map) {
        inputController->mapMove(*this, players.at(0));
        startRandomBattle();

        if (localPlayerId != 0)
            sendLocalPlayerState();

        for (auto& [id, remote] : remotePlayers)
        {
            uiManager->updateRemoteAnimation(remote.animPlayer, remote);
            remote.animPlayer.update(dt);
        }
    }

    if (state == GameState::Battle)
        battle->update();

    updateNetwork();
    uiManager->update(dt);
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    uiManager->drawScene(gameStateString.at(this->getGameState()));
    SDL_RenderPresent(renderer);
}

void Game::startRandomBattle()
{
    if (isConnected() && !isHost())
        return;

    int randomNumber = rand() % 1000;
    if (randomNumber == 1)
    {
        setPreviousGameState(getGameState());
        setGameState(GameState::Battle);
        battle->run();
        uiManager->addEnemys();
        uiManager->addCharacters();

        battleStarted = true;

        // ✅ Уведомляем других игроков
        if (localPlayerId != 0)
            sendBattleStart();
    }
}

void Game::endRandomBattle()
{
    int randomNumber = rand() / 100;
    if (randomNumber > 30)
    {
        if (isHost())
            sendBattleEnd(false); // побег — не победа

        this->setGameState(this->getPrevGameState());
        this->setPreviousGameState(GameState::Battle);
        battleStarted = false;
    }
}

void Game::openOptions(const GameState &preGame) {
    this->setGameState(GameState::Options);
    this->setPreviousGameState(preGame);
}

void Game::closeOptions() {
    this->setGameState(this->getPrevGameState());
    this->setPreviousGameState(GameState::Options);
}

void Game::openOnlineMenu() {
    this->setGameState(GameState::Online);
    this->setPreviousGameState(GameState::Menu);
}

void Game::closeOnlineMenu() {
    this->setGameState(GameState::Menu);
    this->setPreviousGameState(GameState::Online);
}

void Game::connectToServer(const std::string &ip)
{
    if (this->isConnected())
    {
        uiManager->setText("NotifyLabel", "Online", "You are already connected!");
        return;
    }
    if (this->isHost())
    {
        uiManager->setText("NotifyLabel", "Online", "You are host");
        return;
    }

    if (client.connectTo(ip, 54000))
        uiManager->setText("NotifyLabel", "Online", "You are connected");
    else
        uiManager->setText("NotifyLabel", "Online", "You are not connected");
}

void Game::startServer() {
    if (this->isConnected())
    {
        uiManager->setText("NotifyLabel", "Online", "You are already connected!");
        return;
    }
    if (this->isHost())
    {
        uiManager->setText("NotifyLabel", "Online", "You are already host");
        return;
    }

    if (server.start() && client.connectTo("127.0.0.1", 54000))
        uiManager->setText("NotifyLabel","Online", "Server started");
    else
        uiManager->setText("NotifyLabel","Online", "Server not started. Something happend");
}

std::string Game::getIP()
{
    WSADATA wsaData;

    // Инициализация WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return "ERROR";

    char hostName[256];

    // Получаем имя компьютера
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
    {
        WSACleanup();
        return "ERROR";
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;       // Только IPv4
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;

    // Получаем список адресов
    if (getaddrinfo(hostName, nullptr, &hints, &result) != 0)
    {
        WSACleanup();
        return "ERROR";
    }

    std::string ipStr;

    // Проходим по найденным адресам
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        auto* ipv4 = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);

        char ipBuffer[INET_ADDRSTRLEN];

        // Перевод бинарного IP в строку
        inet_ntop(AF_INET,
                  &(ipv4->sin_addr),
                  ipBuffer,
                  INET_ADDRSTRLEN);

        ipStr = ipBuffer;

        // Берем первый найденный IP
        break;
    }

    freeaddrinfo(result);
    WSACleanup();

    return ipStr;
}

void Game::sendBattleStart()
{
    if (!isConnected()) return;
    const auto& enemies = battle->getEnemies();

    BattleStartPacket packet{};
    packet.header.type = PacketType::BattleStart;
    packet.header.size = sizeof(BattleStartPacket);
    packet.enemyCount = static_cast<uint8_t>(enemies.size());

    for (int i = 0; i < (int)enemies.size() && i < 4; i++)
    {
        packet.enemies[i].enemyName  = static_cast<uint8_t>(enemies[i].getEnemyName());
        packet.enemies[i].currentHp  = enemies[i].getCurrentHp();
        packet.enemies[i].maxHp      = enemies[i].getMaxHp();
    }

    client.send(reinterpret_cast<const char*>(&packet), sizeof(packet));
}

void Game::sendBattleAction(const Action& action)
{
    if (!isConnected()) return;
    SDL_Log("sendBattleAction: localPlayerId=%d", localPlayerId);
    BattleActionPacket packet{};
    packet.header.type = PacketType::BattleAction;
    packet.header.size = sizeof(BattleActionPacket);
    packet.playerId    = localPlayerId;
    packet.actionType  = static_cast<uint8_t>(action.type);
    packet.targetType  = static_cast<uint8_t>(action.targetType);
    packet.actorIndex  = action.actorIndex;
    packet.targetIndex = action.targetIndex;
    packet.payloadId   = action.payloadId;

    client.send(reinterpret_cast<const char*>(&packet), sizeof(packet));
}

void Game::sendBattleSnapshot()
{
    if (!isConnected()) return;
    BattleSnapshotPacket packet{};
    packet.header.type = PacketType::BattleSnapshot;
    packet.header.size = sizeof(BattleSnapshotPacket);

    const auto& heroes  = players.at(0).getHeroes();
    const auto& enemies = battle->getEnemies();

    packet.enemyCount = static_cast<uint8_t>(enemies.size());

    // ✅ Сначала герои хоста
    int heroIndex = 0;
    for (int i = 0; i < (int)heroes.size() && heroIndex < 8; i++, heroIndex++)
    {
        packet.heroes[heroIndex].currentHp   = heroes[i].getCurrentHp();
        packet.heroes[heroIndex].currentMana = heroes[i].getCurrentMp();
        packet.heroOwner[heroIndex] = localPlayerId; // ← чьи герои
    }

    // ✅ Потом герои удалённых игроков
    for (auto& [id, remote] : remotePlayers)
    {
        for (int i = 0; i < (int)remote.getHeroes().size() && heroIndex < 8; i++, heroIndex++)
        {
            packet.heroes[heroIndex].currentHp   = remote.getHeroes()[i].getCurrentHp();
            packet.heroes[heroIndex].currentMana = remote.getHeroes()[i].getCurrentMp();
            packet.heroOwner[heroIndex] = id; // ← чьи герои
        }
    }

    packet.heroCount = static_cast<uint8_t>(heroIndex);

    for (int i = 0; i < (int)enemies.size() && i < 4; i++)
    {
        packet.enemies[i].currentHp   = enemies[i].getCurrentHp();
        packet.enemies[i].currentMana = 0;
    }

    packet.battleState      = static_cast<uint8_t>(battle->getState());
    packet.currentHeroIndex = static_cast<uint8_t>(battle->getCurrentHeroIndex());

    packet.currentActionIndex = battle->getCurrentActionIndex();
    packet.currentTurn        = static_cast<uint8_t>(battle->getCurrentTurn());

    client.send(reinterpret_cast<const char*>(&packet), sizeof(packet));
}


void Game::addPlayer(Player&& player) { players.push_back(std::move(player)); }
void Game::addQuest(const Quest& quest) { quests.push_back(quest); }
void Game::addItem(const Item& item) { items[item.id] = item; }

void Game::setGameState(GameState s) { state = s; }
void Game::setPreviousGameState(GameState s) { prevState = s; }
GameState Game::getGameState() const { return state; }
GameState Game::getPrevGameState() const { return prevState; }

void Game::addLocation(const Location &location) { locations.push_back(location); }
std::vector<Location> Game::getLocations() const { return locations; }

void Game::setScreenRect(const SDL_Rect *rect) {this->screen = *rect;}

SDL_Rect Game::getScreenRect() const { return screen; }

Battle *Game::getBattle() const { return battle.get(); }

Item &Game::getItem(const int &index) {
    auto it = items.find(index);
    if (it == items.end()) {
        SDL_Log("Item not found: %d", index);
        return items.at(0);
    }

    return it->second;
}

Skill &Game::getSkill(const int &id) {
    auto it = skills.find(id);
    if (it == skills.end()) {
        SDL_Log("Skill not found: %d", id);
        return skills.at(0);
    }

    return it->second;
}

void Game::initItems()
{
    items.clear();

    // === Лечебное зелье ===
    {
        Item potion;
        potion.id = 1;
        potion.cost = 5;
        potion.name = "Health Potion";
        potion.power = 30;
        potion.consumable = true;
        potion.effectType = ItemEffectType::Heal;
        potion.targetType = TargetType::Self;

        items.emplace(potion.id, potion);
    }

    // === Зелье маны ===
    {
        Item manaPotion;
        manaPotion.id = 2;
        manaPotion.cost = 5;
        manaPotion.name = "Mana Potion";
        manaPotion.power = 20;
        manaPotion.consumable = true;
        manaPotion.effectType = ItemEffectType::RestoreMana;
        manaPotion.targetType = TargetType::Self;

        items.emplace(manaPotion.id, manaPotion);
    }

    // === Бомба (урон по врагу) ===
    {
        Item bomb;
        bomb.id = 3;
        bomb.cost = 10;
        bomb.name = "Bomb";
        bomb.power = 25;
        bomb.consumable = true;
        bomb.effectType = ItemEffectType::Damage;
        bomb.targetType = TargetType::Enemy;

        items.emplace(bomb.id, bomb);
    }

    // === Усиление атаки ===
    {
        Item attackBuff;
        attackBuff.id = 4;
        attackBuff.cost = 8;
        attackBuff.name = "Attack Scroll";
        attackBuff.power = 10;
        attackBuff.consumable = true;
        attackBuff.effectType = ItemEffectType::Buff;
        attackBuff.targetType = TargetType::Self;

        items.emplace(attackBuff.id, attackBuff);
    }

    // === Ослабление врага ===
    {
        Item weaken;
        weaken.id = 5;
        weaken.cost = 8;
        weaken.name = "Weaken Scroll";
        weaken.power = 10;
        weaken.consumable = true;
        weaken.effectType = ItemEffectType::Debuff;
        weaken.targetType = TargetType::Enemy;

        items.emplace(weaken.id, weaken);
    }
}

void Game::initSkills()
{
    skills.clear();

    // === Базовая атака ===
    {
        Skill attack;
        attack.id = 1;
        attack.name = "Attack";
        attack.isDamaging = true;

        attack.manaCost = 0;
        attack.maxCooldown = 0;

        attack.targetCount = 1;
        attack.targetType = TargetType::Enemy;

        attack.effectType = SkillEffectType::Damage;
        attack.element = Elemental::None;

        attack.power = 10;

        skills.emplace(attack.id, attack);
    }

    // === Огненный шар ===
    {
        Skill fireball;
        fireball.id = 2;
        fireball.name = "Fireball";
        fireball.isDamaging = true;

        fireball.manaCost = 12;
        fireball.maxCooldown = 2;

        fireball.targetCount = 1;
        fireball.targetType = TargetType::Enemy;

        fireball.effectType = SkillEffectType::Damage;
        fireball.element = Elemental::Fire;

        fireball.power = 25;

        skills.emplace(fireball.id, fireball);
    }

    // === Лечение ===
    {
        Skill heal;
        heal.id = 3;
        heal.name = "Heal";
        heal.isDamaging = false;

        heal.manaCost = 10;
        heal.maxCooldown = 1;

        heal.targetCount = 1;
        heal.targetType = TargetType::Ally;

        heal.effectType = SkillEffectType::Heal;
        heal.element = Elemental::None;

        heal.power = 20;

        skills.emplace(heal.id, heal);
    }

    // === Удар молнией (игнор брони / магический урон) ===
    {
        Skill lightning;
        lightning.id = 4;
        lightning.name = "Lightning Strike";
        lightning.isDamaging = true;

        lightning.manaCost = 15;
        lightning.maxCooldown = 3;

        lightning.targetCount = 1;
        lightning.targetType = TargetType::Enemy;

        lightning.effectType = SkillEffectType::Damage;
        lightning.element = Elemental::Lightning;

        lightning.power = 30;

        skills.emplace(lightning.id, lightning);
    }

    // === Усиление защиты ===
    {
        Skill defenseBuff;
        defenseBuff.id = 5;
        defenseBuff.name = "Defense Up";
        defenseBuff.isDamaging = false;

        defenseBuff.manaCost = 8;
        defenseBuff.maxCooldown = 3;

        defenseBuff.targetCount = 1;
        defenseBuff.targetType = TargetType::Self;

        defenseBuff.effectType = SkillEffectType::Buff;
        defenseBuff.element = Elemental::None;

        defenseBuff.power = 5;

        skills.emplace(defenseBuff.id, defenseBuff);
    }

    // === Ослабление врагов (по всем) ===
    {
        Skill weaken;
        weaken.id = 6;
        weaken.name = "Weaken";
        weaken.isDamaging = false;

        weaken.manaCost = 14;
        weaken.maxCooldown = 4;

        weaken.targetCount = 0; // 0 = все цели данного типа
        weaken.targetType = TargetType::Enemy;

        weaken.effectType = SkillEffectType::Debuff;
        weaken.element = Elemental::None;

        weaken.power = 5;

        skills.emplace(weaken.id, weaken);
    }
}

