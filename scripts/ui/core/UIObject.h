
#ifndef PROJECT_NAME_UIOBJECT_H
#define PROJECT_NAME_UIOBJECT_H
#include <string>

#include "SDL_rect.h"
#include "SDL_render.h"

class Visualizer;

class UIObject {
public:
    virtual ~UIObject() = default;
    SDL_Rect rect{0,0,0,0};
    SDL_Texture* texture = nullptr;
    bool isVisible{true};
    bool isEnabled{true};

    virtual void setOnClick(std::function<void()> onClick) {}
    virtual void setOnHover(std::function<void()> onHover) {}

    virtual void  setText(const std::string&) {}
    [[nodiscard]] virtual const std::string& getText() const { static std::string s; return s; }

    virtual void  setFocus(bool) {}
    [[nodiscard]] virtual bool  getFocus() const { return false; }

    virtual void  setPos(int) {}
    [[nodiscard]] virtual int   getPos() const { return 0; }

    virtual void  setSrcRect(const SDL_Rect&) {}
    virtual void  draw(Visualizer&) const = 0;

    [[nodiscard]] bool containsPoint(int x, int y) const {
        return x >= rect.x && x <= rect.x + rect.w &&
               y >= rect.y && y <= rect.y + rect.h;
    }
};
#endif //PROJECT_NAME_UIOBJECT_H