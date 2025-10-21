#include "InputManager.h"

void InputManager::update()
{
    // очищаем флаги для нового кадра
    keyPressed.clear();
    keyReleased.clear();
    mousePressed.clear();
    mouseReleased.clear();
}

void InputManager::processEvent(const SDL_Event& e)
{
    switch (e.type)
    {
        case SDL_KEYDOWN:
            if (!e.key.repeat) { // важно: пропускаем авто-повтор
                keyHeld[e.key.keysym.scancode] = true;
                keyPressed[e.key.keysym.scancode] = true;
            }
            break;

        case SDL_KEYUP:
            keyHeld[e.key.keysym.scancode] = false;
            keyReleased[e.key.keysym.scancode] = true;
            break;

        case SDL_MOUSEBUTTONDOWN:
            mouseHeld[e.button.button] = true;
            mousePressed[e.button.button] = true;
            break;

        case SDL_MOUSEBUTTONUP:
            mouseHeld[e.button.button] = false;
            mouseReleased[e.button.button] = true;
            break;

        case SDL_MOUSEMOTION:
            mouseX = e.motion.x;
            mouseY = e.motion.y;
            break;
        default: ;
    }
}

bool InputManager::isKeyPressed(SDL_Scancode key) const {
    auto it = keyPressed.find(key);
    return it != keyPressed.end() && it->second;
}

bool InputManager::isKeyHeld(SDL_Scancode key) const {
    auto it = keyHeld.find(key);
    return it != keyHeld.end() && it->second;
}

bool InputManager::isKeyReleased(SDL_Scancode key) const {
    auto it = keyReleased.find(key);
    return it != keyReleased.end() && it->second;
}

bool InputManager::isMousePressed(Uint8 button) const {
    auto it = mousePressed.find(button);
    return it != mousePressed.end() && it->second;
}

bool InputManager::isMouseHeld(Uint8 button) const {
    auto it = mouseHeld.find(button);
    return it != mouseHeld.end() && it->second;
}

bool InputManager::isMouseReleased(Uint8 button) const {
    auto it = mouseReleased.find(button);
    return it != mouseReleased.end() && it->second;
}
