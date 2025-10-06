
#ifndef PROJECT_NAME_INPUTMANAGER_H
#define PROJECT_NAME_INPUTMANAGER_H
#include <SDl.h>
#include "player.h"

class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    void moveInput(Player player);
private:
    SDL_Event event;
};

#endif //PROJECT_NAME_INPUTMANAGER_H