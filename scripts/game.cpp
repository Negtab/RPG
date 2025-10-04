#include "game.h"

void Game::addPlayer(const Player& player){ players.push_back(player); }
void Game::addItem(const Item &item) { items[items.size()] = item; }
void Game::addQuest(const Quest &quest) { quests.push_back(quest); }

GameState Game::getGameState() const { return state; }
GameState Game::getPrevGameState() const { return prevState; }

void Game::setGameState(const GameState& state) { this->state = state; }
void Game::setPreviousGameState(const GameState& state) { this->prevState = state; }

Game::Game()
{
    this->setGameState(GameState::Menu);
    this->setPreviousGameState(GameState::Menu);
}

