#ifndef PROJECT_NAME_INPUTCONTROLLER_H
#define PROJECT_NAME_INPUTCONTROLLER_H

#include "inputManager.h"
#include "player.h"

class InputController
{
public:
    explicit InputController(InputManager& input) : input(input) {}

    void chooseInput(SDL_Event even, Game& game, Player& player);

private:
    InputManager& input;

    void mapInput(Player& player);
    void menuInput(Game& game);
    void battleInput(Player& player);
    void inventoryInput(Player& player);
};

#endif