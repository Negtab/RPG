#ifndef PROJECT_NAME_INPUTCONTROLLER_H
#define PROJECT_NAME_INPUTCONTROLLER_H

#include "uiManager.h"
#include "inputManager.h"
#include "player.h"

class InputController
{
public:
    explicit InputController(InputManager& input) : input(input) {}

    void chooseInput(SDL_Event even, Game& game, Player& player,  UIManager& uiManager);

private:
    InputManager& input;

    void mapInput(Player& player);
    void menuInput(SDL_Event event, Game& game, UIManager& manager);
    void battleInput(Player& player);
    void inventoryInput(Player& player);
};

#endif