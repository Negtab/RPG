#ifndef PROJECT_NAME_GAMEPAD_H
#define PROJECT_NAME_GAMEPAD_H

#include <SDL.h>
#include <unordered_map>

// Геймпад притворяется клавиатурой: кнопки и стик превращаются в обычные события клавиш,
// поэтому экранам не нужно знать о нём. Раскладка — в gamepad.cpp
class Gamepad
{
public:
    Gamepad() = default;
    ~Gamepad();

    void processEvent(const SDL_Event& e);
    void update();                                    // вызывать раз в кадр: автоповтор стрелок при удержании

private:
    void open(int deviceIndex);
    void close();
    void press(SDL_Scancode key);
    void release(SDL_Scancode key);
    void setAxis(int& dir, Sint16 value, SDL_Scancode negative, SDL_Scancode positive);

    SDL_GameController* pad = nullptr;
    SDL_JoystickID padId = -1;
    std::unordered_map<SDL_Scancode, int> held;       // сколько источников держат клавишу: крестовина и стик
    int stickX = 0, stickY = 0;                       // -1, 0, 1

    SDL_Scancode repeatKey = SDL_SCANCODE_UNKNOWN;    // какая стрелка повторяется
    Uint32 nextRepeat = 0;
};

#endif //PROJECT_NAME_GAMEPAD_H
