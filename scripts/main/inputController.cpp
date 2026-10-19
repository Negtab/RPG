#include "inputController.h"
#include "types.h"
#include "game.h"

void InputController::chooseInput(SDL_Event event, Game& game, Battle& battle, Player& player, UIManager& uiManager)
{
    if (uiManager.handleTextEvent(event, gameStateString.at(game.getGameState())))
        return;                                       // печатаем в поле: игровые клавиши не должны срабатывать
    GameState state = game.getGameState();
    switch (state)
    {
        case GameState::Map:        mapInput(game); break;
        case GameState::Menu:       menuInput(event, game, uiManager); break;
        case GameState::Battle:     battleInput(event, game, battle, uiManager); break;
        case GameState::Inventory:  inventoryInput(player); break;
        case GameState::Options:    optionsInput(event, game, uiManager); break;
        case GameState::Online:     onlineInput(event, game, uiManager); break;
        case GameState::CreatePlayer: createInput(event, game, uiManager); break;
        default: ;
    }
}

void InputController::createInput(SDL_Event event, Game& game, UIManager& ui)
{
    ui.handleClickEvent(event, "CreatePlayer");
    ui.handleHoverEvent(event, "CreatePlayer");
    if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        game.closePartyCreation();                    // сюда доходит, только если поле не поглотило Esc
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
    if (battle.getState() == BattleState::Victory)           // окно результатов: любое подтверждение закрывает его
    {
        const bool confirm =
            (event.type == SDL_KEYDOWN && (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                                           event.key.keysym.scancode == SDL_SCANCODE_SPACE)) ||
            event.type == SDL_MOUSEBUTTONDOWN;
        if (confirm) battle.closeResults();
        return;
    }

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

void InputController::onlineInput(SDL_Event event, Game& game, UIManager& manager)
{
    manager.handleClickEvent(event, "Online");
    manager.handleHoverEvent(event, "Online");
    if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        game.closeOnlineMenu();        // Esc в поле только снимает фокус, второй Esc выходит из меню
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
