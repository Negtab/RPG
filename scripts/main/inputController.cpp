#include "inputController.h"
#include "types.h"
#include "game.h"

void InputController::chooseInput(SDL_Event event, Game& game, Battle& battle, Player& player, UIManager& uiManager)
{
    GameState state = game.getGameState();
    switch (state)
    {
        case GameState::Map:        mapInput(game); break;
        case GameState::Menu:       menuInput(event, game, uiManager); break;
        case GameState::Battle:     battleInput(event, game, battle, uiManager); break;
        case GameState::Inventory:  inventoryInput(player); break;
        case GameState::Options:    optionsInput(event, game, uiManager); break;
        case GameState::Online:     onlineInput(event, game, uiManager); break;
        default: ;
    }
}

void InputController::mapMove(Game &game, Player& player)
{
    Point point = player.getPlayerCoords();
    const int speed = player.getPlayerSpeed();
    bool moved = false;

    if (input.isKeyHeld(SDL_SCANCODE_W) || input.isKeyHeld(SDL_SCANCODE_UP)) {
        moved = true;
        point.y += speed;
        player.setDirection(Direction::Up);
    }
    if (input.isKeyHeld(SDL_SCANCODE_S) || input.isKeyHeld(SDL_SCANCODE_DOWN)) {
        moved = true;
        point.y -= speed;
        player.setDirection(Direction::Down);
    }
    if (input.isKeyHeld(SDL_SCANCODE_A) || input.isKeyHeld(SDL_SCANCODE_LEFT)) {
        moved = true;
        point.x += speed;
        player.setDirection(Direction::Left);
    }
    if (input.isKeyHeld(SDL_SCANCODE_D) || input.isKeyHeld(SDL_SCANCODE_RIGHT)) {
        moved = true;
        point.x -= speed;
        player.setDirection(Direction::Right);
    }

    if (!moved)
        player.setDirection(Direction::Idle);

    player.setPlayerCoords(point);
}

void InputController::mapInput(Game& game)
{
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        game.setPreviousGameState(GameState::Map);
        game.setGameState(GameState::Menu);
    }
}

void InputController::menuInput(SDL_Event event, Game& game, UIManager& manager)
{
    if (input.isMousePressed(SDL_BUTTON_LEFT))
        manager.handleClickEvent(event,gameStateString.at(game.getGameState()));
    manager.handleHoverEvent(event, gameStateString.at(game.getGameState()));

}

void InputController::battleInput(SDL_Event event, Game& game, Battle& battle,  UIManager& manager)
{
    if (input.isMousePressed(SDL_BUTTON_LEFT))
        manager.handleClickEvent(event,gameStateString.at(game.getGameState()));

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

void InputController::onlineInput(SDL_Event event, Game &game, UIManager& manager)
{
    if (input.isMousePressed(SDL_BUTTON_LEFT))
        manager.handleClickEvent(event,gameStateString.at(game.getGameState()));
    if (event.type != SDL_KEYDOWN && event.type != SDL_MOUSEBUTTONDOWN)
        return;
    if (input.isKeyPressed(SDL_SCANCODE_RETURN) || input.isKeyPressed(SDL_SCANCODE_KP_ENTER) || input.isKeyPressed(SDL_SCANCODE_RETURN2))
        manager.setFocus("IPEdit","Online", false);

    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE))
    {
        game.setPreviousGameState(GameState::Online);
        game.setGameState(GameState::Menu);
    }

    if (manager.getFocus("IPEdit", "Online"))
    {
        if (input.isKeyPressed(SDL_SCANCODE_PERIOD)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), ".") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1); }
        if (input.isKeyPressed(SDL_SCANCODE_0)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "0") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1); }
        if (input.isKeyPressed(SDL_SCANCODE_1)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "1") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1); }
        if (input.isKeyPressed(SDL_SCANCODE_2)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "2") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_3)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "3") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_4)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "4") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_5)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "5") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_6)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "6") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_7)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "7") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_8)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "8") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}
        if (input.isKeyPressed(SDL_SCANCODE_9)) { manager.setText("IPEdit", "Online",manager.getText("IPEdit", "Online").insert(manager.getPos("IPEdit", "Online"), "9") ); manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") + 1);}

        if (input.isKeyPressed(SDL_SCANCODE_BACKSPACE))
        {
            if (manager.getPos("IPEdit", "Online") == 0)
                return;
            manager.setText("IPEdit", "Online", manager.getText("IPEdit", "Online").erase(manager.getPos("IPEdit", "Online") - 1, 1));
            manager.setPos("IPEdit", "Online", manager.getPos("IPEdit", "Online") - 1);
        }
        if (input.isKeyPressed(SDL_SCANCODE_LEFT)) { manager.setPos("IPEdit","Online", manager.getPos("IPEdit","Online") - 1); }
        if (input.isKeyPressed(SDL_SCANCODE_RIGHT)) {  manager.setPos("IPEdit","Online", manager.getPos("IPEdit","Online") + 1); }
    }
}


void InputController::optionsInput(SDL_Event event, Game &game, UIManager &uiManager) {
    if (input.isKeyReleased(SDL_SCANCODE_ESCAPE))
        game.closeOptions();
}


void InputController::inventoryInput(Player& player)
{
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        // выйти из инвентаря
    }
}
