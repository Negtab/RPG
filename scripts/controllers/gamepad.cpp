#include "gamepad.h"

namespace {
    constexpr int kStickOn = 16000, kStickOff = 10000;     // разный порог: у границы стик не дребезжит
    constexpr Uint32 kRepeatDelay = 350, kRepeatStep = 120;

    bool isArrow(SDL_Scancode key)
    {
        return key == SDL_SCANCODE_UP || key == SDL_SCANCODE_DOWN ||
               key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT;
    }

    void pushKey(SDL_Scancode key, bool down, bool repeat)
    {
        SDL_Event e{};
        e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
        e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
        e.key.repeat = repeat ? 1 : 0;
        e.key.keysym.scancode = key;
        e.key.keysym.sym = SDL_GetKeyFromScancode(key);
        SDL_PushEvent(&e);
    }

    // Названия кнопок SDL — по положению, как на Xbox: A снизу, B справа
    SDL_Scancode keyForButton(Uint8 button)
    {
        switch (button)
        {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:       return SDL_SCANCODE_UP;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return SDL_SCANCODE_DOWN;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return SDL_SCANCODE_LEFT;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return SDL_SCANCODE_RIGHT;
            case SDL_CONTROLLER_BUTTON_A:             return SDL_SCANCODE_RETURN;   // подтвердить, заговорить
            case SDL_CONTROLLER_BUTTON_B:             return SDL_SCANCODE_ESCAPE;   // назад
            case SDL_CONTROLLER_BUTTON_START:         return SDL_SCANCODE_ESCAPE;   // меню
            case SDL_CONTROLLER_BUTTON_X:             return SDL_SCANCODE_E;        // посадить дирижабль
            case SDL_CONTROLLER_BUTTON_Y:             return SDL_SCANCODE_V;        // сменить героя, удалить сохранение
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return SDL_SCANCODE_M;        // мини-карта
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return SDL_SCANCODE_B;        // бестиарий в бою
            case SDL_CONTROLLER_BUTTON_BACK:          return SDL_SCANCODE_H;        // случайные бои
            default:                                  return SDL_SCANCODE_UNKNOWN;
        }
    }
}

Gamepad::~Gamepad()
{
    if (pad) SDL_GameControllerClose(pad);
}

void Gamepad::open(int deviceIndex)
{
    if (pad) return;                                       // играем одним геймпадом: первым подключённым
    pad = SDL_GameControllerOpen(deviceIndex);
    if (!pad)
    {
        SDL_Log("Cannot open gamepad: %s", SDL_GetError());
        return;
    }
    padId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
    SDL_Log("Gamepad connected: %s", SDL_GameControllerName(pad));
}

void Gamepad::close()
{
    for (const auto& [key, count] : held)                  // иначе герой пойдёт дальше сам
        if (count > 0) pushKey(key, false, false);
    held.clear();
    stickX = stickY = 0;
    repeatKey = SDL_SCANCODE_UNKNOWN;
    SDL_GameControllerClose(pad);
    pad = nullptr;
    padId = -1;
}

void Gamepad::press(SDL_Scancode key)
{
    if (key == SDL_SCANCODE_UNKNOWN || held[key]++ > 0) return;
    pushKey(key, true, false);
    if (isArrow(key))
    {
        repeatKey = key;
        nextRepeat = SDL_GetTicks() + kRepeatDelay;
    }
}

void Gamepad::release(SDL_Scancode key)
{
    if (key == SDL_SCANCODE_UNKNOWN) return;
    const auto it = held.find(key);
    if (it == held.end() || it->second == 0 || --it->second > 0) return;
    pushKey(key, false, false);
    if (key == repeatKey) repeatKey = SDL_SCANCODE_UNKNOWN;
}

void Gamepad::setAxis(int& dir, Sint16 value, SDL_Scancode negative, SDL_Scancode positive)
{
    int next = dir;
    if (dir == 0 || (dir < 0 ? value > -kStickOff : value < kStickOff)) next = 0;
    if (value <= -kStickOn) next = -1;
    if (value >= kStickOn)  next = 1;
    if (next == dir) return;

    if (dir != 0) release(dir < 0 ? negative : positive);
    if (next != 0) press(next < 0 ? negative : positive);
    dir = next;
}

void Gamepad::processEvent(const SDL_Event& e)
{
    switch (e.type)
    {
        case SDL_CONTROLLERDEVICEADDED:                    // приходит и за уже подключённые при запуске
            open(e.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (pad && e.cdevice.which == padId) close();
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            if (pad && e.cbutton.which == padId) press(keyForButton(e.cbutton.button));
            break;
        case SDL_CONTROLLERBUTTONUP:
            if (pad && e.cbutton.which == padId) release(keyForButton(e.cbutton.button));
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (!pad || e.caxis.which != padId) break;
            if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
                setAxis(stickX, e.caxis.value, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
            if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
                setAxis(stickY, e.caxis.value, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
            break;
        default: ;
    }
}

void Gamepad::update()
{
    if (repeatKey == SDL_SCANCODE_UNKNOWN || SDL_TICKS_PASSED(SDL_GetTicks(), nextRepeat) == SDL_FALSE) return;
    pushKey(repeatKey, true, true);
    nextRepeat = SDL_GetTicks() + kRepeatStep;
}
