#include "game.h"
#include "inputController.h"

#include <SDL.h>


Game::Game(SDL_Renderer* renderer) : renderer(renderer)
{
    players.emplace_back("Test");

    resourceManager = std::make_unique<ResourceManager>(*renderer);

    visualizer = std::make_unique<Visualizer>(renderer);
    uiManager = std::make_unique<UIManager>(*visualizer, *resourceManager, *this, players.at(0));

    inputManager = std::make_unique<InputManager>();
    inputController = std::make_unique<InputController>(*inputManager);

    battle = std::make_unique<Battle>(&players.at(0), uiManager.get(), this);
}

void Game::run()
{
    this->isRunning = true;
    SDL_Event event;
    this->setGameState(GameState::Menu);
    resourceManager->initialize();
    uiManager->initialize();
    srand(static_cast<unsigned int>(time(nullptr)));

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

        update();   // логика
        render();   // отрисовка
        inputManager->update();

        SDL_Delay(16); // ~60 FPS
    }
}

void Game::startGame()
{
    this->setPreviousGameState(GameState::Menu);
    this->setGameState(players.at(players.size() - 1).getLastGameState());
}

void Game::endGame()
{
    this->isRunning = false;
}


void Game::handleInput(const SDL_Event &event)
{
    inputController->chooseInput(event, *this, *this->getBattle(), players.at(0), *this->uiManager);
}


void Game::update()
{
    if (state == GameState::Map)
        startRandomBattle();

    if (state == GameState::Battle)
        battle->update();
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    uiManager->drawScene(gameStateToString(this->getGameState()));
    SDL_RenderPresent(renderer);
}

void Game::startRandomBattle()
{
    int randomNumber = rand()/100;
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

void Game::addPlayer(const Player& player) { players.push_back(player); }
void Game::addQuest(const Quest& quest) { quests.push_back(quest); }
void Game::addItem(const Item& item) { items[item.getId()] = item; }

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