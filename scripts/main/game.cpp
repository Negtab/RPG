#include "game.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <SDL.h>
#include <ws2tcpip.h>

#include "inputController.h"
#include "mapLoader.h"
#include "terrain.h"
#include <SDL_image.h>
#include "../gameLogic/gameData.h"


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

    // Сохранение грузится по «Continue» или с экрана загрузки, а не при запуске
    saveDir = std::filesystem::current_path().remove_filename() / "saves";
    migrateOldSave();
    refreshLatestSave();
    loadSettings();

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
    if (gameInProgress)                                      // из главного меню без игры автосейв не перезаписываем
        saveToSlot(0);
}

// ─────────────────────────────────────────
// Сохранения
// ─────────────────────────────────────────

std::string Game::slotPath(int slot) const
{
    return (saveDir / (slot == 0 ? std::string("auto.bin") : "slot" + std::to_string(slot) + ".bin")).string();
}

void Game::migrateOldSave()
{
    // Раньше было одно сохранение save.bin рядом с ресурсами: становится автосохранением
    const auto old = saveDir.parent_path() / "save.bin";
    std::error_code ec;
    if (!std::filesystem::is_regular_file(old, ec) || std::filesystem::exists(slotPath(0), ec)) return;
    std::filesystem::create_directories(saveDir, ec);
    std::filesystem::rename(old, slotPath(0), ec);
    if (ec) SDL_Log("Cannot move save.bin to saves/: %s", ec.message().c_str());
}

void Game::refreshLatestSave()
{
    latestSaveSlot = -1;
    int64_t latest = -1;
    for (int i = 0; i < kSaveSlots; ++i)
    {
        const auto s = getSlotSummary(i);
        if (!s || s->corrupted) continue;
        if (s->savedAt > latest) { latest = s->savedAt; latestSaveSlot = i; }   // старые файлы без даты: 0, но тоже годятся
    }
}

std::optional<SaveSummary> Game::getSlotSummary(int slot) const
{
    if (slot < 0 || slot >= kSaveSlots) return std::nullopt;
    return resourceManager->readSaveSummary(slotPath(slot));
}

std::string Game::currentLocation() const
{
    const Point c = players.at(0).getMove().cell;
    if (const Zone* z = currentMap->zoneAt(c.x, c.y); z && !z->name.empty())
        return z->name;
    std::string name = currentMap->name.empty() ? "World" : currentMap->name;
    name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    return name;
}

bool Game::canSave() const
{
    // Клиент в кооперативе стоит на карте хоста: такую позицию без хоста не восстановить
    return gameInProgress && state != GameState::Battle && !(isConnected() && !isHost());
}

bool Game::canLoad() const
{
    return latestSaveSlot >= 0 && !isConnected();             // загрузка посреди коопа рассинхронизирует отряды
}

bool Game::saveToSlot(int slot)
{
    if (slot < 0 || slot >= kSaveSlots) return false;
    const SaveMeta meta{currentLocation(), static_cast<uint32_t>(playSeconds)};
    const bool ok = resourceManager->savePlayer(slotPath(slot), players.at(0), meta);
    if (ok) refreshLatestSave();
    return ok;
}

bool Game::loadFromSlot(int slot)
{
    if (slot < 0 || slot >= kSaveSlots || isConnected()) return false;
    try
    {
        SaveMeta meta;
        const bool positionRestored = resourceManager->loadPlayer(slotPath(slot), players.at(0), &meta);
        playSeconds = static_cast<float>(meta.playSeconds);
        if (!meta.hasVehicles) resetVehicles();
        placePlayerAfterLoad(positionRestored);
    }
    catch (const std::exception& e)
    {
        SDL_Log("Cannot load slot %d: %s", slot, e.what());
        return false;
    }
    startGame();
    return true;
}

void Game::placePlayerAfterLoad(bool positionRestored)
{
    Player& me = players.at(0);
    if (me.getHeroes().empty())                               // сохранения до версии 3 без отряда: даём классический
        for (SpecializationNames cls : {SpecializationNames::Warrior, SpecializationNames::Thief,
                                        SpecializationNames::Archer, SpecializationNames::Magician})
        {
            me.getLinkTOHeroes().emplace_back(Specialization(cls));
            me.getLinkTOHeroes().back().equipStarting();
        }

    const Point c = me.getPlayerCoords();
    const auto mapIt = maps.find(me.getMove().mapId);
    const bool validCell = mapIt != maps.end() &&
        (me.getRiding() == Vehicle::None ? mapIt->second.isWalkable(c.x, c.y)
                                         : vehicleCanMove(mapIt->second, me.getRiding(), c.x, c.y));
    if (positionRestored && validCell)
        currentMap = &mapIt->second;                         // продолжаем там, где сохранились (город, подземелье)
    else
    {
        currentMap = &maps.at(0);
        me.getMove().mapId = 0;
        me.setPlayerCoords(currentMap->spawn);
        me.setRiding(Vehicle::None);                       // корабль посреди суши не оставляем
    }
    stepsSinceBattle = 0;
    stepFrom = me.getPlayerCoords();
}

// ─────────────────────────────────────────
// Транспорт
// ─────────────────────────────────────────

void Game::computeWater(MapData& m)
{
    if (m.backgroundPath.empty()) return;
    SDL_Surface* raw = IMG_Load(m.backgroundPath.c_str());
    if (!raw) { SDL_Log("Map '%s': no image for water: %s", m.name.c_str(), IMG_GetError()); return; }
    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!rgba) return;

    int cw = 0, ch = 0;
    const auto coverage = waterCoverage(rgba, TILE_PX, cw, ch);
    SDL_FreeSurface(rgba);

    // Хотя бы половина клетки — вода: так корабль подходит вплотную к берегу
    m.water.assign(static_cast<size_t>(m.w) * m.h, 0);
    for (int y = 0; y < std::min(ch, m.h); ++y)
        for (int x = 0; x < std::min(cw, m.w); ++x)
            m.water[static_cast<size_t>(y) * m.w + x] = coverage[static_cast<size_t>(y) * cw + x] >= TILE_PX * TILE_PX / 2;
}

void Game::resetVehicles()
{
    auto& vehicles = players.at(0).getVehicles();
    vehicles = {};
    const MapData& world = maps.at(0);

    auto nearLand = [&](int x, int y) {
        return world.isWalkable(x + 1, y) || world.isWalkable(x - 1, y) ||
               world.isWalkable(x, y + 1) || world.isWalkable(x, y - 1);
    };
    auto taken = [&](int x, int y) {
        for (const auto& v : vehicles) if (v.available && v.cell.x == x && v.cell.y == y) return true;
        return x == world.spawn.x && y == world.spawn.y;
    };
    // Запасное место, если маркера в Tiled нет: ближайшая к точке появления подходящая клетка
    auto nearest = [&](auto&& fits) -> std::optional<Point> {
        std::optional<Point> best;
        int bestD = INT32_MAX;
        for (int y = 0; y < world.h; ++y)
            for (int x = 0; x < world.w; ++x)
            {
                const int d = (x - world.spawn.x) * (x - world.spawn.x) + (y - world.spawn.y) * (y - world.spawn.y);
                if (d < bestD && !taken(x, y) && fits(x, y)) { bestD = d; best = Point{x, y}; }
            }
        return best;
    };

    for (Vehicle v : {Vehicle::Ship, Vehicle::Canoe, Vehicle::Airship})
    {
        std::optional<Point> at = world.vehicleMarkers[vehicleIndex(v)];
        if (!at)
        {
            if (v == Vehicle::Airship)
                at = nearest([&](int x, int y) { return world.isWalkable(x, y) && !world.portalAt(x, y); });
            else if (v == Vehicle::Ship || world.hasRivers())        // каноэ без размеченных рек ставить некуда
                at = nearest([&](int x, int y) { return vehicleCanMove(world, v, x, y) && nearLand(x, y); });
        }
        if (!at) { SDL_Log("No place for the %s on the world map", vehicleName(v)); continue; }
        vehicles[vehicleIndex(v)] = {true, static_cast<uint16_t>(world.mapId), *at, Direction::Down};
    }
}

void Game::parkVehicle(Point at, uint16_t mapId)
{
    Player& me = players.at(0);
    const Vehicle riding = me.getRiding();
    if (riding == Vehicle::None) return;
    auto& v = me.getVehicles()[vehicleIndex(riding)];
    v.available = true;
    v.mapId = mapId;
    v.cell = at;
    v.dir = me.getMove().dir;
    me.setRiding(Vehicle::None);
}

bool Game::updateVehicleAfterStep(Point from)
{
    Player& me = players.at(0);
    const MoveState& m = me.getMove();
    const Vehicle riding = me.getRiding();

    if (riding == Vehicle::Ship || riding == Vehicle::Canoe)
    {
        if (vehicleCanMove(*currentMap, riding, m.cell.x, m.cell.y)) return false;
        parkVehicle(from, m.mapId);                          // шагнули на берег: лодка остаётся у берега
        return true;
    }
    if (riding != Vehicle::None) return false;

    for (int i = 0; i < kVehicleKinds; ++i)                  // зашли на стоящий транспорт: садимся
    {
        const VehicleState& v = me.getVehicles()[i];
        if (v.available && v.mapId == m.mapId && v.cell.x == m.cell.x && v.cell.y == m.cell.y)
        {
            me.setRiding(static_cast<Vehicle>(i + 1));
            return true;
        }
    }
    return false;
}

bool Game::canLandAirship() const
{
    const Player& me = players.at(0);
    if (state != GameState::Map || me.getRiding() != Vehicle::Airship || me.getMove().moving) return false;
    const Point c = me.getMove().cell;
    if (!currentMap->isWalkable(c.x, c.y) || currentMap->portalAt(c.x, c.y)) return false;   // не в город и не на вход
    for (int i = 0; i < kVehicleKinds; ++i)                  // на другой стоящий транспорт не садимся
    {
        const VehicleState& v = me.getVehicles()[i];
        if (i != vehicleIndex(Vehicle::Airship) && v.available && v.mapId == me.getMove().mapId &&
            v.cell.x == c.x && v.cell.y == c.y)
            return false;
    }
    return true;
}

void Game::vehicleAction()
{
    if (!canLandAirship()) return;
    const MoveState& m = players.at(0).getMove();
    parkVehicle(m.cell, m.mapId);
    stepsSinceBattle = 0;
    if (localPlayerId != 0) sendLocalPlayerState();
}

void Game::continueGame()
{
    if (gameInProgress) { startGame(); return; }
    if (latestSaveSlot >= 0 && loadFromSlot(latestSaveSlot)) return;
    refreshLatestSave();                                      // слот испортился с момента проверки: кнопка погаснет
}

void Game::openSaveScreen(bool saving)
{
    if (saving ? !canSave() : !canLoad()) return;
    setPreviousGameState(state);                              // назад вернёмся туда, откуда пришли: титул или игровое меню
    setGameState(GameState::Saves);
    uiManager->openSaveScreen(saving);
}

void Game::closeSaveScreen()
{
    setGameState(prevState == GameState::GameMenu ? GameState::GameMenu : GameState::Menu);
}

// ─────────────────────────────────────────
// Игровое меню
// ─────────────────────────────────────────

void Game::openGameMenu()
{
    settle(players.at(0).getMove());                          // не уходим в меню посреди шага между клетками
    if (localPlayerId != 0) sendLocalPlayerState();
    setPreviousGameState(GameState::Map);
    setGameState(GameState::GameMenu);
    uiManager->openGameMenu();
}

void Game::closeGameMenu()
{
    setGameState(GameState::Map);
}

void Game::returnToTitle()
{
    saveToSlot(0);                                            // как при выходе из игры: прогресс не теряется
    if (isConnected() || isHost())
        onConnectionLost("Returned to title");               // выходим из коопа тем же путём, что и при обрыве
    gameInProgress = false;
    setPreviousGameState(GameState::GameMenu);
    setGameState(GameState::Menu);
}

void Game::loadSettings()
{
    // Формат «ключ=значение» по строке: файл можно поправить руками. Нет файла — всё по умолчанию
    std::ifstream in((saveDir.parent_path() / "settings.ini").string());
    std::string line;
    int music = AudioManager::getMusicVolume(), sound = AudioManager::getSoundVolume();
    while (std::getline(in, line))
    {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        int value = 0;
        if (!(std::istringstream(line.substr(eq + 1)) >> value)) continue;
        if (key == "music")      music = value;
        if (key == "sound")      sound = value;
        if (key == "encounters") encounters = value != 0;
        if (key == "minimap")    setMinimapZoom(value);       // старый файл: 1 = вкл, это и есть x1
    }
    AudioManager::setMusicVolume(music);                      // заодно применяем и зажимаем в 0..10
    AudioManager::setSoundVolume(sound);
}

void Game::cycleMinimap()
{
    setMinimapZoom((minimapZoom + 1) % (kMinimapZoomMax + 1));
    saveSettings();
}

void Game::toggleEncounters()
{
    encounters = !encounters;
    saveSettings();
    uiManager->showMapNotice(encounters ? "Encounters: On" : "Encounters: Off");
}

void Game::saveSettings() const
{
    const std::string path = (saveDir.parent_path() / "settings.ini").string();
    std::ofstream out(path, std::ios::trunc);
    out << "music=" << AudioManager::getMusicVolume() << "\n"
        << "sound=" << AudioManager::getSoundVolume() << "\n"
        << "encounters=" << (encounters ? 1 : 0) << "\n"
        << "minimap=" << minimapZoom << "\n";
    if (!out) SDL_Log("Cannot write settings to %s", path.c_str());
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
            computeWater(m);
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

    // Через портал идут пешком: транспорт остаётся там, где стоял игрок
    Player& me = players.at(0);
    if (me.getRiding() != Vehicle::None)
        parkVehicle(me.getMove().moving ? me.getMove().target : me.getMove().cell, me.getMove().mapId);
    for (auto& entry : remotePlayers) entry.second.setRiding(Vehicle::None);

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

void Game::sendPartyInfo()
{
    if (!isConnected() || localPlayerId == 0 || players.empty()) return;

    PartyInfoPacket p{};
    p.header.type = PacketType::PartyInfo;
    p.header.size = sizeof(PartyInfoPacket);
    p.playerId = localPlayerId;

    const auto& heroes = players[0].getHeroes();
    p.heroCount = static_cast<uint8_t>(std::min<size_t>(heroes.size(), 4));
    for (int i = 0; i < p.heroCount; ++i)
    {
        p.heroes[i].classId   = static_cast<uint8_t>(heroes[i].getClass());
        p.heroes[i].level     = heroes[i].getLevel();
        p.heroes[i].currentHp = static_cast<int32_t>(heroes[i].getCurrentHp());
        p.heroes[i].currentMp = static_cast<int32_t>(heroes[i].getCurrentMp());
        std::strncpy(p.heroes[i].name, heroes[i].getName().c_str(), 12);
        p.heroes[i].name[12] = '\0';
        for (int s = 0; s < kEquipSlots; ++s)
            p.heroes[i].equipment[s] = heroes[i].getEquipped(static_cast<EquipSlot>(s));
    }
    client.send(reinterpret_cast<const char*>(&p), sizeof(p));
}

void Game::applyPartyInfo(const PartyInfoPacket& p)
{
    if (p.playerId == localPlayerId || p.heroCount > 4) return;
    for (int i = 0; i < p.heroCount; ++i)                        // данные из сети: проверяем
        if (p.heroes[i].classId > static_cast<uint8_t>(SpecializationNames::Thief) ||
            p.heroes[i].level < 1 || p.heroes[i].level > 99 || std::memchr(p.heroes[i].name, 0, 13) == nullptr)
            return;

    // Посреди хода хоста HP и MP считает только хост: чужие значения не принимаем
    const bool applyVitals = !(state == GameState::Battle && isHost() &&
                               battle->getState() != BattleState::PlayerChoose);

    auto& heroes = ensureRemote(p.playerId).getLinkTOHeroes();
    std::vector<Hero> rebuilt;
    for (int i = 0; i < p.heroCount; ++i)
    {
        Hero hero{Specialization(static_cast<SpecializationNames>(p.heroes[i].classId))};
        hero.setLevel(p.heroes[i].level);
        if (applyVitals)
        {
            hero.setCurrentHp(static_cast<uint32_t>(std::max(0, p.heroes[i].currentHp)));
            hero.setCurrentMp(static_cast<uint32_t>(std::max(0, p.heroes[i].currentMp)));
        }
        else if (i < static_cast<int>(heroes.size()))
        {
            hero.setCurrentHp(heroes[i].getCurrentHp());        // сохраняем то, что считал хост
            hero.setCurrentMp(heroes[i].getCurrentMp());
        }
        const std::string cleaned = sanitizeHeroName(p.heroes[i].name);
        if (!cleaned.empty()) hero.setName(cleaned);
        for (int s = 0; s < kEquipSlots; ++s)                    // equip сам отбросит чужой слот, класс и неизвестный id
            hero.equip(static_cast<EquipSlot>(s), p.heroes[i].equipment[s]);
        rebuilt.push_back(std::move(hero));
    }
    heroes = std::move(rebuilt);

    SDL_Log("Party of player %u: %d heroes, first level %d", p.playerId, (int)p.heroCount,
            p.heroCount ? (int)p.heroes[0].level : 0);
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
    p.vehicle   = static_cast<uint8_t>(players[0].getRiding());
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
    if (p.vehicle > static_cast<uint8_t>(Vehicle::Airship)) return;
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
    ensureRemote(p.playerId).setRiding(static_cast<Vehicle>(p.vehicle));
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
            sendPartyInfo();
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

        case PacketType::PartyInfo:
            applyPartyInfo(*reinterpret_cast<const PartyInfoPacket*>(data));
            break;

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
                sendPartyInfo();
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
    if (state == GameState::Map || state == GameState::Battle || state == GameState::Inventory)
    {
        gameInProgress = true;                           // сюда попадают и из «New game», и из загрузки, и из коопа
        playSeconds += dt;                               // меню и экраны сохранений время не тратят
    }

    if (state == GameState::Battle)
        battle->update();

    if (state == GameState::Map) {
        Player& me = players.at(0);
        MoveState& m = me.getMove();
        const Direction oldDir = m.dir;

        // Проходимость зависит от транспорта: пешком суша, на корабле море, в воздухе всё
        const Vehicle riding = me.getRiding();
        auto passable = [&](Point t) { return canEnter(*currentMap, riding, t.x, t.y, me.getVehicles(), m.mapId); };
        const Point finishedFrom = stepFrom;           // новый шаг может начаться в этом же кадре и затереть его
        const MoveResult r = updateLocal(m, passable, inputController->readMoveDirection(), dt, vehicleSpeed(riding));
        if (r.stepStarted) stepFrom = m.cell;
        uiManager->updateRemoteAnimation(me.animPlayer, me);   // название историческое: работает для любого игрока
        me.animPlayer.update(dt);

        if (localPlayerId != 0 && (r.stepStarted || r.stopped || m.dir != oldDir))
            sendLocalPlayerState();                       // только по событиям, не каждый кадр

        for (auto& [id, remote] : remotePlayers) {
            advance(remote.getMove(), dt, vehicleSpeed(remote.getRiding()));
            uiManager->updateRemoteAnimation(remote.animPlayer, remote);
            remote.animPlayer.update(dt);
        }

        if (r.stepFinished) onStepFinished(finishedFrom); // последним: может сменить состояние на Battle
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

void Game::onStepFinished(Point from)
{
    if (updateVehicleAfterStep(from) && localPlayerId != 0)
        sendLocalPlayerState();                          // напарник должен увидеть корабль, а не героя

    if (isConnected() && !isHost()) return;              // порталы и бои ведёт хост (или офлайн)

    const Point c = players.at(0).getMove().cell;
    const Vehicle riding = players.at(0).getRiding();
    if (riding == Vehicle::Airship) return;              // в воздухе ни порталов, ни боёв

    if (const Portal* portal = currentMap->portalAt(c.x, c.y); portal && riding == Vehicle::None)
    {
        followPortal(*portal);
        return;
    }

    const Zone* zone = currentMap->zoneAt(c.x, c.y);
    if (zone && zone->safe) { stepsSinceBattle = 0; return; }

    if (!encounters) return;                             // бои выключены в Config
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

    if (localPlayerId != 0)
    {
        sendBattleStart();
        sendPartyInfo();
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
    saveSettings();                                           // громкость переживает перезапуск
    this->setGameState(this->getPrevGameState());
    this->setPreviousGameState(GameState::Options);
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

void Game::openPartyCreation()
{
    setPreviousGameState(GameState::Menu);
    setGameState(GameState::CreatePlayer);
    uiManager->resetPartyCreation();
}

void Game::closePartyCreation()
{
    setGameState(GameState::Menu);
    setPreviousGameState(GameState::CreatePlayer);
}

void Game::buildParty(const std::vector<SpecializationNames>& classes, const std::vector<std::string>& names)
{
    Player& me = players.at(0);
    me.resetProgress();
    resetVehicles();
    playSeconds = 0.0f;
    currentMap = &maps.at(0);
    me.setPlayerCoords(currentMap->spawn);
    me.getMove().mapId = 0;

    for (size_t i = 0; i < classes.size() && i < 4; ++i)
    {
        Hero hero{Specialization(classes[i])};
        if (i < names.size())
        {
            const std::string name = sanitizeHeroName(names[i]);
            if (!name.empty()) hero.setName(name);
        }
        hero.equipStarting();
        me.getLinkTOHeroes().push_back(std::move(hero));
    }

    for (const auto& [id, item] : GameData::instance().items())        // стартовые зелья из items.json
        if (item.startCount > 0)
            me.addItem(id, static_cast<uint8_t>(item.startCount));
}

void Game::createParty(const std::vector<SpecializationNames>& classes, const std::vector<std::string>& names)
{
    buildParty(classes, names);
    startGame();
}

void Game::openOnlineMenu()
{
    if (players.at(0).getHeroes().empty())               // без отряда в кооператив не зайти: берём классический
        buildParty({SpecializationNames::Warrior, SpecializationNames::Thief,
                    SpecializationNames::Archer, SpecializationNames::Magician}, {});
    this->setGameState(GameState::Online);
    this->setPreviousGameState(GameState::Menu);
}
