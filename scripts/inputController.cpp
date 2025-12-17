#include "inputController.h"
#include "types.h"
#include "game.h"

void InputController::chooseInput(SDL_Event event, Game& game, Battle& battle, Player& player, UIManager& uiManager)
{
    GameState state = game.getGameState();
    switch (state)
    {
        case GameState::Map:      mapInput(player); break;
        case GameState::Menu:       menuInput(event, game, uiManager); break;
        case GameState::Battle:     battleInput(event, game, battle, uiManager); break;
        case GameState::Inventory:  inventoryInput(player); break;
        default: ;
    }
}

void InputController::mapInput(Player& player)
{
    Point point = player.getPlayerCoords();
    const int speed = player.getPlayerSpeed();

    if (input.isKeyHeld(SDL_SCANCODE_W) || input.isKeyHeld(SDL_SCANCODE_UP)) point.y += speed;
    if (input.isKeyHeld(SDL_SCANCODE_S) || input.isKeyHeld(SDL_SCANCODE_DOWN)) point.y -= speed;
    if (input.isKeyHeld(SDL_SCANCODE_A) || input.isKeyHeld(SDL_SCANCODE_LEFT)) point.x += speed;
    if (input.isKeyHeld(SDL_SCANCODE_D) || input.isKeyHeld(SDL_SCANCODE_RIGHT)) point.x -= speed;

    if (input.isKeyPressed(SDL_SCANCODE_M))
    {
        // открыть меню или карта
    }

    player.setPlayerCoords(point);
}

void InputController::menuInput(SDL_Event event, Game& game, UIManager& manager)
{
    if (input.isMousePressed(SDL_BUTTON_LEFT))
        manager.handleClickEvent(event,gameStateToString(game.getGameState()));
    manager.handleHoverEvent(event, gameStateToString(game.getGameState()));

}

void InputController::battleInput(SDL_Event event, Game& game, Battle& battle,  UIManager& manager)
{
    if (input.isMousePressed(SDL_BUTTON_LEFT))
        manager.handleClickEvent(event,gameStateToString(game.getGameState()));

    if (manager.getState() == UIChooseState::ChooseTarget)
    {
        if (input.isKeyPressed(SDL_SCANCODE_SPACE) || input.isKeyPressed(SDL_SCANCODE_RETURN))
            manager.confirmTarget();
        if (input.isKeyPressed(SDL_SCANCODE_W) || input.isKeyPressed(SDL_SCANCODE_UP) ||
            input.isKeyPressed(SDL_SCANCODE_D) || input.isKeyPressed(SDL_SCANCODE_RIGHT))
            manager.moveSelectorToNext();
        if (input.isKeyPressed(SDL_SCANCODE_S) || input.isKeyPressed(SDL_SCANCODE_DOWN) ||
            input.isKeyPressed(SDL_SCANCODE_A) || input.isKeyPressed(SDL_SCANCODE_LEFT))
            manager.moveSelectorToPrevious();
    }

}

void InputController::inventoryInput(Player& player)
{
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        // выйти из инвентаря
    }
}
