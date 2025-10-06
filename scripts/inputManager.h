#ifndef PROJECT_NAME_INPUTMANAGER_H
#define PROJECT_NAME_INPUTMANAGER_H

#include <SDl.h>
#include <unordered_map>

class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    void processEvent(const SDL_Event& e);
    void update(); // вызывать в начале каждого кадра

    // --- Клавиатура ---
    bool isKeyPressed(SDL_Scancode key) const;   // нажата в этом кадре
    bool isKeyHeld(SDL_Scancode key) const;      // удерживается
    bool isKeyReleased(SDL_Scancode key) const;  // отпущена

    // --- Мышь ---
    bool isMousePressed(Uint8 button) const;
    bool isMouseHeld(Uint8 button) const;
    bool isMouseReleased(Uint8 button) const;
    int getMouseX() const { return mouseX; }
    int getMouseY() const { return mouseY; }

private:
    std::unordered_map<SDL_Scancode, bool> keyHeld;
    std::unordered_map<SDL_Scancode, bool> keyPressed;
    std::unordered_map<SDL_Scancode, bool> keyReleased;

    std::unordered_map<Uint8, bool> mouseHeld;
    std::unordered_map<Uint8, bool> mousePressed;
    std::unordered_map<Uint8, bool> mouseReleased;

    int mouseX = 0;
    int mouseY = 0;
};

#endif //PROJECT_NAME_INPUTMANAGER_H