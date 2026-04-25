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
    uiManager = std::make_unique<UIManager>(*visualizer, *resourceManager, *this, players.at(0));

    inputManager = std::make_unique<InputManager>();
    inputController = std::make_unique<InputController>(*inputManager);

    battle = std::make_unique<Battle>(&players.at(0), uiManager.get(), this);

    resourceManager->initialize();
    uiManager->initialize();

    initItems();
    initSkills();

    SDL_SetWindowIcon(window, resourceManager->getImage("Sword"));

    this->isRunning = true;
    this->setGameState(GameState::Menu);
}

void Game::run()
{
    SDL_Event event;

    srand(static_cast<unsigned int>(time(nullptr)));

    pathToSave = std::filesystem::current_path().remove_filename().string() + "save.bin";
    if (resourceManager->isPlayerNew(pathToSave))
    {
        uiManager->setEnVI("Continue", "Menu", false);
    }
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

std::string Game::getCurrentTime() const{
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
        inputController->mapMove(players.at(0));
        startRandomBattle();
    }

    if (state == GameState::Battle)
        battle->update();

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
    int randomNumber = rand()%1000;
    if (randomNumber == 1)
    {
        this->setPreviousGameState(this->getGameState());
        this->setGameState(GameState::Battle);
        battle->run();
        this->uiManager->addEnemys();
        this->uiManager->addCharacters();
    }
}

void Game::endRandomBattle()
{
    int randomNumber = rand()/100;
    if (randomNumber > 30)
    {
        this->setGameState(this->getPrevGameState());
        this->setPreviousGameState(GameState::Battle);
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

std::string Game::getIP() const {
    WSADATA wsaData;
    std::string ipStr;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return "ERROR";

    char hostName[256];
    gethostname(hostName, sizeof(hostName));

    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostName, NULL, &hints, &res) == 0) {
        struct addrinfo* p = res;
        while (p) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), LPSTR(ipStr.c_str()), INET_ADDRSTRLEN);
            p = p->ai_next;
        }
        freeaddrinfo(res);
    }
    WSACleanup();
    return ipStr;
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

