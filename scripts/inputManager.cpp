#include "inputManager.h"
#include "types.h"

void InputManager::moveInput(Player player)
{
    SDL_PollEvent( & event);
    if(event.type == SDL_KEYDOWN)
    {
        switch (event.key.keysym.sym)
        {
            case SDLK_w:
            case SDLK_UP:
            {
                Point point = player.getPlayerCoords();
                int speed = player.getPlayerSpeed();
                point.y += speed;
                player.setPlayerCoords(point);
                break;
            }
            case SDLK_s:
            case SDLK_DOWN:
            {
                Point point = player.getPlayerCoords();
                int speed = player.getPlayerSpeed();
                point.y -= speed;
                player.setPlayerCoords(point);
                break;
            }
            case SDLK_a:
            case SDLK_LEFT:
            {
                Point point = player.getPlayerCoords();
                int speed = player.getPlayerSpeed();
                point.x -= speed;
                player.setPlayerCoords(point);
                break;
            }
            case SDLK_d:
            case SDLK_RIGHT:
            {
                Point point = player.getPlayerCoords();
                int speed = player.getPlayerSpeed();
                point.x += speed;
                player.setPlayerCoords(point);
                break;
            }
        }
    }
}