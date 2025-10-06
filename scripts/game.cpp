#include "game.h"
#include <SDL.h>

#include "inputController.h"

Game::Game(SDL_Renderer* renderer) : renderer(renderer)
{
    resourceManager = std::make_unique<ResourceManager>();
    visualizer = std::make_unique<Visualizer>(renderer);
    uiManager = std::make_unique<UIManager>(*visualizer);

    inputManager = std::make_unique<InputManager>();
    inputController = std::make_unique<InputController>(*inputManager);
}

void Game::run()
{
    bool running = true;
    SDL_Event event;

    players.emplace_back("Artem");
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
                break;
            }
            else
                handleInput(event); // обработка клавиш и кликов
        }

        update();   // логика
        render();   // отрисовка

        SDL_Delay(16); // ~60 FPS
    }
}

void Game::handleInput(const SDL_Event &event)
{
    inputController->chooseInput(event, *this, players.at(0));
}

void Game::update()
{
    // логика игры, обновление состояния
}

void Game::render()
{
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void Game::addPlayer(const Player& player) { players.push_back(player); }
void Game::addQuest(const Quest& quest) { quests.push_back(quest); }
void Game::addItem(const Item& item) { items[item.getId()] = item; }

void Game::setGameState(GameState s) { state = s; }
void Game::setPreviousGameState(GameState s) { prevState = s; }
GameState Game::getGameState() const { return state; }
GameState Game::getPrevGameState() const { return prevState; }
