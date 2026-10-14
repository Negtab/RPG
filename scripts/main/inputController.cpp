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
static bool dirHeld(const InputManager& in, Direction d) {
    switch (d) {
    case Direction::Up:    return in.isKeyHeld(SDL_SCANCODE_W) || in.isKeyHeld(SDL_SCANCODE_UP);
    case Direction::Down:  return in.isKeyHeld(SDL_SCANCODE_S) || in.isKeyHeld(SDL_SCANCODE_DOWN);
    case Direction::Left:  return in.isKeyHeld(SDL_SCANCODE_A) || in.isKeyHeld(SDL_SCANCODE_LEFT);
    case Direction::Right: return in.isKeyHeld(SDL_SCANCODE_D) || in.isKeyHeld(SDL_SCANCODE_RIGHT);
    default: return false;
    }
}
static bool dirPressed(const InputManager& in, Direction d) {
    switch (d) {
    case Direction::Up:    return in.isKeyPressed(SDL_SCANCODE_W) || in.isKeyPressed(SDL_SCANCODE_UP);
    case Direction::Down:  return in.isKeyPressed(SDL_SCANCODE_S) || in.isKeyPressed(SDL_SCANCODE_DOWN);
    case Direction::Left:  return in.isKeyPressed(SDL_SCANCODE_A) || in.isKeyPressed(SDL_SCANCODE_LEFT);
    case Direction::Right: return in.isKeyPressed(SDL_SCANCODE_D) || in.isKeyPressed(SDL_SCANCODE_RIGHT);
    default: return false;
    }
}

Direction InputController::readMoveDirection() {       // последняя нажатая клавиша, без диагоналей
    static constexpr Direction all[] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    for (Direction d : all) if (dirPressed(input, d)) lastMoveDir = d;
    if (lastMoveDir != Direction::Idle && dirHeld(input, lastMoveDir)) return lastMoveDir;
    for (Direction d : all) if (dirHeld(input, d)) return lastMoveDir = d;
    return lastMoveDir = Direction::Idle;
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
