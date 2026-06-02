#ifndef PROJECT_NAME_BUTTON_H
#define PROJECT_NAME_BUTTON_H
#include <functional>
#include <string>

#include "SDL_pixels.h"
#include "SDL_ttf.h"
#include "../core/UIObject.h"
#include "../../controllers/resourceManager.h"

class Visualizer;

class Button : public UIObject {
public:
    std::function<void()> onClick;
    std::function<void()> onHover;

    void draw(Visualizer& v) const override {
        if (!isVisible || !texture)
            return;
        v.drawTexture(texture, rect.x, rect.y, rect.w, rect.h);
    }

    void setOnClick(std::function<void()> onClick) override {
        this->onClick = onClick;
    }

    void setOnHover(std::function<void()> onHover) override {
        this->onHover = onHover;
    }

    void handleClick(int x, int y) {
        if (!isEnabled) return;
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h)
            if (onClick) onClick();
    }

    void handleHover(int x, int y) {
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h)
            if (onHover) onHover();
    }
};

class Edit : public UIObject {
public:
    bool isFocused{false};
    int  pos{0};
    std::string text;
    TTF_Font*   font{nullptr};
    SDL_Color   color{0, 0, 0, 255};

    Edit() = default;
    Edit(const Edit&) = delete;
    Edit& operator=(const Edit&) = delete;
    Edit(Edit&&) = default;
    Edit& operator=(Edit&&) = default;

    ~Edit() override {
        if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
    }

    void setText(const std::string& t) override {
        if (text != t) { text = t; dirty = true; }
    }

    const std::string& getText() const override { return text; }

    void setFocus(bool f) override {
        if (isFocused != f) { isFocused = f; dirty = true; }
    }

    bool getFocus() const override { return isFocused; }

    void setPos(int p) override {
        int clamped = std::clamp(p, 0, static_cast<int>(text.size()));
        if (pos != clamped) { pos = clamped; dirty = true; }
    }

    int getPos() const override { return pos; }

    void setOnClick(std::function<void()> onClick) override {
        this->onClick = onClick;
    }

    void setOnHover(std::function<void()> onHover) override {
        this->onHover = onHover;
    }

    void handleClick(int x, int y) {
        if (!isEnabled) return;
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h)
            if (onClick) onClick();
    }

    void handleHover(int x, int y) {
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h)
            if (onHover) onHover();
    }

    void draw(Visualizer& v) const override {
        if (!isVisible) return;
        if (dirty) const_cast<Edit*>(this)->rebuildTexture(v);

        // Фон поля
        v.drawTexture(ResourceManager::getTexture("WhiteBox"),
                      rect.x, rect.y, rect.w, rect.h);
        // Текст
        if (texture)
            v.drawTexture(texture, rect.x, rect.y, rect.w, rect.h);
        // Курсор
        if (isFocused)
            drawCursor(v);
    }

    std::function<void()> onClick;
    std::function<void()> onHover;

private:
    mutable bool dirty{true};

    void rebuildTexture(Visualizer& v)
    {
        if (!font)
        {
            dirty = false;
            return;
        }
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        auto block = ResourceManager::renderMultiline(v.getRenderer(), text.empty() ? " " : text, font, color);
        texture  = block.texture;
        dirty    = false;
    }

    void drawCursor(Visualizer& v) const {
        // Измеряем ширину текста до позиции курсора
        int cursorX = rect.x;
        if (font && !text.empty()) {
            std::string before = text.substr(0, pos);
            int w = 0, h = 0;
            TTF_SizeUTF8(font, before.c_str(), &w, &h);
            cursorX += w;
        }

        // Мигание через SDL_GetTicks
        if ((SDL_GetTicks() / 500) % 2 == 0) {
            SDL_Rect cursorRect{cursorX, rect.y + 2, 2, rect.h - 4};
            v.drawRect(cursorRect, {0, 0, 0, 255});  // нужен метод в Visualizer
        }
    }
};


#endif //PROJECT_NAME_BUTTON_H