#ifndef PROJECT_NAME_INPUTCONTROLLER_H
#define PROJECT_NAME_INPUTCONTROLLER_H

#include "player.h"
#include "../ui/uiManager.h"

#include "../gameLogic/battle.h"

#include "../controllers/inputManager.h"

class Battle;
class UIManager;

class InputController
{
public:
    explicit InputController(InputManager& input) : input(input) {}

    Direction readMoveDirection();
    void chooseInput(SDL_Event even, Game& game, Battle &battle, Player& player,  UIManager& uiManager);
private:
    InputManager& input;

    Direction lastMoveDir{Direction::Idle};
    void onlineInput(SDL_Event event, Game &game, UIManager& manager);
    void mapInput(Game& game);
    void menuInput(SDL_Event event, Game& game, UIManager& manager);
    void battleInput(SDL_Event event, Game& game, Battle& battle, UIManager& uiManager);
    void optionsInput(SDL_Event event, Game& game, UIManager& uiManager);
    void inventoryInput(Player& player);
};

#endif