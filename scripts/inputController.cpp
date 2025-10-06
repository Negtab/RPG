#include "inputController.h"
#include "types.h"
#include "game.h"

void InputController::chooseInput(SDL_Event event, Game& game, Player& player)
{

    GameState state = game.getGameState();
    switch (state)
    {
        case GameState::Map:      mapInput(player); break;
        case GameState::Menu:       menuInput(game); break;
        case GameState::Battle:     battleInput(player); break;
        case GameState::Inventory:  inventoryInput(player); break;
        default: ;
    }
}

void InputController::mapInput(Player& player)
{
    Point point = player.getPlayerCoords();
    const int speed = player.getPlayerSpeed();

    if (input.isKeyHeld(SDL_SCANCODE_W)) point.y -= speed;
    if (input.isKeyHeld(SDL_SCANCODE_S)) point.y += speed;
    if (input.isKeyHeld(SDL_SCANCODE_A)) point.x -= speed;
    if (input.isKeyHeld(SDL_SCANCODE_D)) point.x += speed;

    if (input.isKeyPressed(SDL_SCANCODE_M)) {
        // открыть меню или карта
    }

    player.setPlayerCoords(point);
}

void InputController::menuInput(Game& game)
{
    if (input.isMousePressed(SDL_BUTTON_LEFT)) {
        // обработка клика по кнопкам меню
    }
}

void InputController::battleInput(Player& player)
{
    if (input.isKeyPressed(SDL_SCANCODE_SPACE)) {
        // атаковать
    }
}

void InputController::inventoryInput(Player& player)
{
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        // выйти из инвентаря
    }
}
