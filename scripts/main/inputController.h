#ifndef PROJECT_NAME_INPUTCONTROLLER_H
#define PROJECT_NAME_INPUTCONTROLLER_H

#include "player.h"
#include "uiManager.h"

#include "../gameLogic/battle.h"

#include "../controllers/inputManager.h"

class Battle;
class UIManager;

class InputController
{
public:
    explicit InputController(InputManager& input) : input(input) {}

    void mapMove(Game &game, Player& player);
    void chooseInput(SDL_Event even, Game& game, Battle &battle, Player& player,  UIManager& uiManager);
private:
    InputManager& input;

    void onlineInput(SDL_Event event, Game &game, UIManager& manager);
    void mapInput(Game& game);
    void menuInput(SDL_Event event, Game& game, UIManager& manager);
    void battleInput(SDL_Event event, Game& game, Battle& battle, UIManager& uiManager);
    void optionsInput(SDL_Event event, Game& game, UIManager& uiManager);
    void inventoryInput(Player& player);
};

#endif