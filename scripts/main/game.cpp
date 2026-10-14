#include "game.h"

#include <chrono>
#include <filesystem>

#include "inputController.h"
#include "mapLoader.h"
#include "../gameLogic/gameData.h"

#include <SDL.h>
#include <ws2tcpip.h>

static void initWalkAnimations(Player& p)
{
    for (const char* name : {"MoveLeftAnimation", "MoveRightAnimation", "MoveUpAnimation", "MoveDownAnimation"})
        p.animPlayer.animations[name] =
    { 0.2f, ResourceManager::getAnimation(name), {}, {0, 0, 0, 0}, 0, 0.f, false, AnimPlayMode::Loop };
}

namespace { constexpr int kDefaultEncounterRate = 8; }

Game::Game(SDL_Window *window, SDL_Renderer* renderer) : window(window), renderer(renderer)
{
    players.emplace_back("Test");

    loadMaps();

    resourceManager = std::make_unique<ResourceManager>(*renderer);

    visualizer = std::make_unique<Visualizer>(renderer);

    audioManager = std::make_unique<AudioManager>();

    uiManager = std::make_unique<UIManager>(*visualizer, *audioManager, *resourceManager, *this, players.at(0));

    inputManager = std::make_unique<InputManager>();
    inputController = std::make_unique<InputController>(*inputManager);

    battle = std::make_unique<Battle>(&players.at(0), uiManager.get(), this);

    resourceManager->initialize();
    initWalkAnimations(players.at(0));
    uiManager->initialize();

    items  = GameData::instance().items();
    skills = GameData::instance().skills();

    SDL_SetWindowIcon(window, ResourceManager::getImage("Sword"));

    this->isRunning = true;
    this->setGameState(GameState::Menu);
}

void Game::run()
{
    SDL_Event event;

    srand(static_cast<unsigned int>(time(nullptr)));

    pathToSave = std::filesystem::current_path().remove_filename().string() + "save.bin";
    bool positionRestored = false;
    if (resourceManager->isPlayerNew(pathToSave))
        uiManager->setEnVI("Continue", "Menu", false);
    else
    {
        try
        {
            positionRestored = resourceManager->loadPlayer(pathToSave, players.at(0));
            uiManager->setEnVI("Continue", "Menu", true);
        }
        catch (const std::exception& e)
        {
            SDL_Log("Save is unreadable (%s), starting a new game", e.what());
            std::error_code ec;
            std::filesystem::rename(pathToSave, pathToSave + ".bad", ec);   // не удаляем: можно посмотреть
            uiManager->setEnVI("Continue", "Menu", false);
        }
    }

    Player& me = players.at(0);
    const Point c = me.getPlayerCoords();
    const auto mapIt = maps.find(me.getMove().mapId);
    if (positionRestored && mapIt != maps.end() && mapIt->second.isWalkable(c.x, c.y))
        currentMap = &mapIt->second;                         // продолжаем там, где сохранились (город, подземелье)
    else
    {
        currentMap = &maps.at(0);
        me.getMove().mapId = 0;
        me.setPlayerCoords(currentMap->spawn);
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

void Game::loadMaps()
{
    const auto dir = std::filesystem::current_path().remove_filename() / "resources" / "Maps";

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (entry.path().extension() != ".tmj") continue;
        try
        {
            MapData m = loadMap(entry.path().string());
            if (maps.contains(m.mapId))
            {
                SDL_Log("Map '%s': mapId %d already used, skipped", m.name.c_str(), m.mapId);
                continue;
            }
            mapIdByName[m.name] = m.mapId;
            maps.emplace(m.mapId, std::move(m));
        }
        catch (const std::exception& e)
        {
            SDL_Log("Map '%s' failed to load: %s", entry.path().filename().string().c_str(), e.what());
        }
    }

    if (!maps.contains(0))                               // нет оверворлда: открытое поле, чтобы игра запустилась
    {
        SDL_Log("No overworld map, using an open field");
        mapIdByName["overworld"] = 0;
        maps.emplace(0, MapData::openField(256, 277));
    }

    for (const auto& [id, m] : maps)
    {
        // проверка порталов
        for (const Portal& p : m.portals)
        {
            const auto it = mapIdByName.find(p.targetMap);
            if (it == mapIdByName.end())
            {
                SDL_Log("Map '%s': portal at (%d,%d) leads to unknown map '%s'",
                        m.name.c_str(), p.x, p.y, p.targetMap.c_str());
                continue;
            }
            if (!maps.at(it->second).isWalkable(p.targetX, p.targetY))
                SDL_Log("Map '%s': portal at (%d,%d) targets (%d,%d), a wall in '%s'",
                        m.name.c_str(), p.x, p.y, p.targetX, p.targetY, p.targetMap.c_str());
        }
        for (const Zone& z : m.zones)
            if (!z.encounter.empty() && !GameData::instance().hasEncounter(z.encounter))
                SDL_Log("Map '%s': zone '%s' uses unknown encounter group '%s'",
                        m.name.c_str(), z.name.c_str(), z.encounter.c_str());
    }

    currentMap = &maps.at(0);
}

bool Game::applyMapChange(int mapId, int x, int y)
{
    const auto it = maps.find(mapId);
    if (it == maps.end() || !it->second.isWalkable(x, y))
    {
        SDL_Log("applyMapChange: bad target map %d (%d,%d)", mapId, x, y);
        return false;
    }

    currentMap = &it->second;

    auto place = [&](MoveState& m) {
        settle(m);
        m.cell = m.target = {x, y};
        m.mapId = static_cast<uint16_t>(mapId);
    };
    place(players.at(0).getMove());
    for (auto& entry : remotePlayers)                    // остальные тоже встают сюда до их собственных пакетов
        place(entry.second.getMove());

    stepsSinceBattle = 0;
    if (localPlayerId != 0) sendLocalPlayerState();
    return true;
}

void Game::followPortal(const Portal& portal)
{
    const Portal p = portal;                             // копия: currentMap сейчас сменится
    const auto it = mapIdByName.find(p.targetMap);
    if (it == mapIdByName.end())
    {
        SDL_Log("Portal leads to unknown map '%s'", p.targetMap.c_str());
        return;
    }
    if (applyMapChange(it->second, p.targetX, p.targetY))
        sendMapChange(it->second, p.targetX, p.targetY);
}

void Game::sendMapChange(int mapId, int x, int y)
{
    if (!isConnected() || !isHost()) return;

    MapChangePacket p{};
    p.header.type = PacketType::MapChange;
    p.header.size = sizeof(MapChangePacket);
    p.mapId = static_cast<uint16_t>(mapId);
    p.cellX = static_cast<int16_t>(x);
    p.cellY = static_cast<int16_t>(y);
    client.send(reinterpret_cast<const char*>(&p), sizeof(p));
}

void Game::gatherPartyAtHost()                           // новичок присоединился: все встают в клетку хоста
{
    MoveState& m = players.at(0).getMove();
    settle(m);
    const Point c = m.cell;
    const int id = currentMap->mapId;
    if (applyMapChange(id, c.x, c.y))
        sendMapChange(id, c.x, c.y);
}

void Game::sendLocalPlayerState() {
    if (!isConnected() || localPlayerId == 0 || players.empty()) return;
    const MoveState& m = players[0].getMove();

    PlayerMovePacket p{};
    p.header.type = PacketType::PlayerMove;
    p.header.size = sizeof(PlayerMovePacket);
    p.playerId  = localPlayerId;
    p.mapId     = m.mapId;
    p.cellX     = static_cast<int16_t>(m.cell.x);   p.cellY   = static_cast<int16_t>(m.cell.y);
    p.targetX   = static_cast<int16_t>(m.target.x); p.targetY = static_cast<int16_t>(m.target.y);
    p.direction = static_cast<uint8_t>(m.dir);
    p.moving    = m.moving ? 1 : 0;
    client.send(reinterpret_cast<const char*>(&p), sizeof(p));
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

Player& Game::ensureRemote(uint32_t id) {
    auto it = remotePlayers.find(id);
    if (it != remotePlayers.end()) return it->second;

    Player player("RemotePlayer");
    player.setRole(NetworkRole::Remote);
    player.setNetworkId(id);
    initWalkAnimations(player);
    return remotePlayers.emplace(id, std::move(player)).first->second;
}

void Game::processMovePacket(const PlayerMovePacket& p) {
    if (p.playerId == localPlayerId) return;
    if (p.direction > static_cast<uint8_t>(Direction::Idle)) return;           // мусорное значение
    const int dx = p.targetX - p.cellX, dy = p.targetY - p.cellY;
    if (p.moving && std::abs(dx) + std::abs(dy) != 1) return;                  // шаг ровно на одну клетку

    MoveState& m = ensureRemote(p.playerId).getMove();
    m.cell   = {p.cellX, p.cellY};
    m.target = {p.moving ? p.targetX : p.cellX, p.moving ? p.targetY : p.cellY};
    m.dir    = p.direction == static_cast<uint8_t>(Direction::Idle) ? Direction::Down
                                                                     : static_cast<Direction>(p.direction);
    m.moving = p.moving != 0;
    m.progress = 0.f;
    m.mapId  = p.mapId;
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
            sendLocalPlayerState();
            SDL_Log("Player connect");
            break;
        }

        case PacketType::PlayerMove:
        {
            auto* packet = reinterpret_cast<const PlayerMovePacket*>(data);
            processMovePacket(*packet);
            break;
        }

        case PacketType::MapChange:
        {
            const auto* p = reinterpret_cast<const MapChangePacket*>(data);
            if (!isHost())                                   // хост применил смену сам
                applyMapChange(p->mapId, p->cellX, p->cellY);
            break;
        }

        case PacketType::PlayerJoined:
        {
            auto* packet = reinterpret_cast<const PlayerJoinedPacket*>(data);
            if (packet->playerId != localPlayerId)
                ensureRemote(packet->playerId);
            if (isHost())
                gatherPartyAtHost();
            SDL_Log("Player joined: %d", packet->playerId);
            break;
        }

        case PacketType::Disconnect:
        {
            auto* packet = reinterpret_cast<const DisconnectPacket*>(data);
            remotePlayers.erase(packet->playerId);
            if (state == GameState::Battle)
                battle->onPlayerLeft(packet->playerId);

                                                    // enum из сети: значение вне диапазона
            SDL_Log("Player disconnected: %d", packet->playerId);
            break;
        }

        case PacketType::BattleStart:
        {
            auto* packet = reinterpret_cast<const BattleStartPacket*>(data);

            if (!battleStarted)
            {
                battleStarted = true;
                settle(players.at(0).getMove());
                sendLocalPlayerState();
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

                // case PacketType::BattleAction: в начале, после проверки playerId == localPlayerId
            if (packet->actionType > static_cast<uint8_t>(ActionType::Skip) ||
                packet->targetType > static_cast<uint8_t>(TargetType::AllAllies))
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
    if (!client.isConnected()) return;

    // Сервер, который принял соединение, но не прислал Connect (например, на порту чужая программа)
    if (localPlayerId == 0 && SDL_GetTicks() - connectTicks > 3000)
    {
        onConnectionLost("handshake timeout");
        return;
    }

    char chunk[2048];
    for (int i = 0; i < 16; ++i)                            // читаем всё накопившееся, но не бесконечно
    {
        const RecvResult r = client.receive(chunk, sizeof(chunk));

        if (r.status == RecvStatus::Data)
            netRecvBuffer.insert(netRecvBuffer.end(), chunk, chunk + r.bytes);
        else if (r.status == RecvStatus::NoData)
            break;
        else
        {
            onConnectionLost(r.status == RecvStatus::Closed ? "connection closed" : "network error");
            return;
        }
    }

    size_t offset = 0;
    while (netRecvBuffer.size() - offset >= sizeof(PacketHeader))
    {
        auto* header = reinterpret_cast<PacketHeader*>(netRecvBuffer.data() + offset);

        const uint32_t expected = expectedSize(header->type);
        if (expected == 0 || header->size != expected)
        {
            // В TCP границы пакетов не восстановить: единственный безопасный выход это разрыв
            onConnectionLost("protocol error");
            return;
        }

        if (netRecvBuffer.size() - offset < header->size)
            break;                                          // пакет ещё не пришёл целиком

        processPacket(netRecvBuffer.data() + offset);
        offset += header->size;

        if (!client.isConnected()) return;                 // processPacket могла разорвать соединение
    }

    netRecvBuffer.erase(netRecvBuffer.begin(), netRecvBuffer.begin() + offset);

    if (!client.flush())
        onConnectionLost("send failed");
}

void Game::onConnectionLost(const std::string& reason)
{
    SDL_Log("Connection lost: %s", reason.c_str());

    const bool wasHost = isHost();
    client.disconnect();
    if (wasHost) server.stop();            // если хост сам потерял связь, закрываем и сервер

    netRecvBuffer.clear();
    remotePlayers.clear();
    localPlayerId = 0;

    if (state == GameState::Battle)        // как в обработчике BattleEnd
    {
        battle->resetState();
        battleStarted = false;
        setGameState(getPrevGameState());
        setPreviousGameState(GameState::Battle);
    }

    uiManager->setText("NotifyLabel", "Online", "Disconnected: " + reason);
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
    if (state == GameState::Battle)
        battle->update();

    if (state == GameState::Map) {
        Player& me = players.at(0);
        MoveState& m = me.getMove();
        const Direction oldDir = m.dir;

        const MoveResult r = updateLocal(m, *currentMap, inputController->readMoveDirection(), dt);
        uiManager->updateRemoteAnimation(me.animPlayer, me);   // название историческое: работает для любого игрока
        me.animPlayer.update(dt);

        if (localPlayerId != 0 && (r.stepStarted || r.stopped || m.dir != oldDir))
            sendLocalPlayerState();                       // только по событиям, не каждый кадр

        for (auto& [id, remote] : remotePlayers) {
            advance(remote.getMove(), dt);
            uiManager->updateRemoteAnimation(remote.animPlayer, remote);
            remote.animPlayer.update(dt);
        }

        if (r.stepFinished) onStepFinished();             // последним: может сменить состояние на Battle
    }

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

void Game::onStepFinished()
{
    if (isConnected() && !isHost()) return;              // порталы и бои ведёт хост (или офлайн)

    const Point c = players.at(0).getMove().cell;

    if (const Portal* portal = currentMap->portalAt(c.x, c.y))
    {
        followPortal(*portal);
        return;
    }

    const Zone* zone = currentMap->zoneAt(c.x, c.y);
    if (zone && zone->safe) { stepsSinceBattle = 0; return; }

    if (++stepsSinceBattle < 6) return;
    if (rand() % 100 >= (zone ? zone->encounterRate : kDefaultEncounterRate)) return;

    stepsSinceBattle = 0;
    beginBattle(zone);
}

void Game::beginBattle(const Zone* zone)
{
    settle(players.at(0).getMove());
    if (localPlayerId != 0) sendLocalPlayerState();

    std::vector<int> formation, pool;
    if (zone)
    {
        pool = zone->enemies;
        if (!zone->encounter.empty())
            formation = GameData::instance().pickFormation(zone->encounter);
    }

    setPreviousGameState(getGameState());
    setGameState(GameState::Battle);
    battle->run(formation, pool);
    uiManager->addEnemys();
    uiManager->addCharacters();
    battleStarted = true;

    if (localPlayerId != 0) sendBattleStart();
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
    {
        netRecvBuffer.clear();
        connectTicks = SDL_GetTicks();
        uiManager->setText("NotifyLabel", "Online", "You are connected");
    }
    else
        uiManager->setText("NotifyLabel", "Online", "You are not connected");
}

void Game::startServer()
{
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

    if (!server.start())
    {
        uiManager->setText("NotifyLabel", "Online", "Server not started (port busy?)");
        return;
    }
    if (!client.connectTo("127.0.0.1", 54000))
    {
        server.stop();
        uiManager->setText("NotifyLabel", "Online", "Server not started. Something happened");
        return;
    }

    netRecvBuffer.clear();
    connectTicks = SDL_GetTicks();
    uiManager->setText("NotifyLabel", "Online", "Server started");
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

    const int n = std::min<int>(static_cast<int>(enemies.size()), 4);
    packet.enemyCount = static_cast<uint8_t>(n);
    for (int i = 0; i < n; i++)
    {
        packet.enemies[i].enemyId   = static_cast<uint8_t>(enemies[i].getEnemyId());
        packet.enemies[i].currentHp = static_cast<int32_t>(enemies[i].getCurrentHp());
        packet.enemies[i].maxHp     = static_cast<int32_t>(enemies[i].getMaxHp());
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

    constexpr int kMaxHeroes  = 8;   // размер packet.heroes и heroOwner
    constexpr int kMaxEnemies = 4;   // размер packet.enemies

    // Сначала герои хоста
    int heroIndex = 0;
    for (int i = 0; i < static_cast<int>(heroes.size()) && heroIndex < kMaxHeroes; i++, heroIndex++)
    {
        packet.heroes[heroIndex].currentHp   = heroes[i].getCurrentHp();
        packet.heroes[heroIndex].currentMana = heroes[i].getCurrentMp();
        packet.heroOwner[heroIndex]          = localPlayerId;   // чьи герои
    }

    // Потом герои удалённых игроков, но только тех, кто участвует в этом бою
    for (const auto& [id, remote] : remotePlayers)
    {
        if (!battle->isParticipant(id)) continue;   // зашёл посреди боя: героев в бою у него нет

        const auto& remoteHeroes = remote.getHeroes();
        for (int i = 0; i < static_cast<int>(remoteHeroes.size()) && heroIndex < kMaxHeroes; i++, heroIndex++)
        {
            packet.heroes[heroIndex].currentHp   = remoteHeroes[i].getCurrentHp();
            packet.heroes[heroIndex].currentMana = remoteHeroes[i].getCurrentMp();
            packet.heroOwner[heroIndex]          = id;
        }
    }

    packet.heroCount = static_cast<uint8_t>(heroIndex);

    const int enemyCount = std::min<int>(static_cast<int>(enemies.size()), kMaxEnemies);
    packet.enemyCount = static_cast<uint8_t>(enemyCount);

    for (int i = 0; i < enemyCount; i++)
    {
        packet.enemies[i].currentHp   = enemies[i].getCurrentHp();
        packet.enemies[i].currentMana = 0;
    }

    packet.battleState        = static_cast<uint8_t>(battle->getState());
    packet.currentHeroIndex   = static_cast<uint8_t>(battle->getCurrentHeroIndex());
    packet.currentActionIndex = static_cast<uint8_t>(battle->getCurrentActionIndex());
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

void Game::setScreenRect(const SDL_Rect *rect) {this->screen = *rect;}

SDL_Rect Game::getScreenRect() const { return screen; }

Battle *Game::getBattle() const { return battle.get(); }

const Item* Game::findItem(int id) const {
    auto it = items.find(id);
    return it != items.end() ? &it->second : nullptr;
}

const Skill* Game::findSkill(int id) const {
    auto it = skills.find(id);
    return it != skills.end() ? &it->second : nullptr;
}

Item &Game::getItem(const int &index) {
    static Item none{};                       // id 0, эффект None: пустышка вместо падения
    auto it = items.find(index);
    if (it == items.end()) { SDL_Log("Item not found: %d", index); return none; }
    return it->second;
}

Skill &Game::getSkill(const int &id) {
    static Skill none = [] { Skill s; s.effectType = SkillEffectType::None; s.name = "?"; return s; }();
    auto it = skills.find(id);
    if (it == skills.end()) { SDL_Log("Skill not found: %d", id); return none; }
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
        potion.power = 15;
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
        attack.element = Elemental::Physical;

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
        heal.element = Elemental::Physical;

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
        defenseBuff.element = Elemental::Physical;

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
        weaken.element = Elemental::Physical;

        weaken.power = 5;

        skills.emplace(weaken.id, weaken);
    }
}

