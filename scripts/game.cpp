#include "game.h"
#include <SDL.h>

#include "inputController.h"

Game::Game(SDL_Renderer* renderer) : renderer(renderer)
{
    players.push_back(Player("Test"));
    resourceManager = std::make_unique<ResourceManager>(*renderer);
    visualizer = std::make_unique<Visualizer>(renderer);
    uiManager = std::make_unique<UIManager>(*visualizer, *resourceManager, *this, players.at(0));

    inputManager = std::make_unique<InputManager>();
    inputController = std::make_unique<InputController>(*inputManager);

}

void Game::run()
{
    this->isRunning = true;
    SDL_Event event;
    this->setGameState(GameState::Menu);
    resourceManager->initialize();
    uiManager->initialize();

    players.emplace_back("Artem");
    while (this->isRunning)
    {
        inputManager->update();
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
    inputController->chooseInput(event, *this, players.at(0), *this->uiManager);
}

void Game::update()
{
    // логика игры, обновление состояния
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    uiManager->drawScene(gameStateToString(this->getGameState()));
    SDL_RenderPresent(renderer);
}

void Game::addPlayer(const Player& player) { players.push_back(player); }
void Game::addQuest(const Quest& quest) { quests.push_back(quest); }
void Game::addItem(const Item& item) { items[item.getId()] = item; }

void Game::setGameState(GameState s) { state = s; }
void Game::setPreviousGameState(GameState s) { prevState = s; }
GameState Game::getGameState() const { return state; }
GameState Game::getPrevGameState() const { return prevState; }

std::vector<Location> Game::getLocations() const { return locations; }