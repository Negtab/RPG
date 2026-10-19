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
    bool        isFocused{false};
    int         pos{0};                       // позиция курсора (только печатный ASCII, поэтому байты = символы)
    std::string text;
    TTF_Font*   font{nullptr};
    SDL_Color   color{0, 0, 0, 255};
    SDL_Color   background{255, 255, 255, 255};
    SDL_Color   border{0, 0, 0, 255};
    int         paddingX{6};
    int         maxLength{32};

    std::function<void()> onClick;
    std::function<void()> onHover;
    std::function<void()> onSubmit;           // Enter

    Edit() = default;
    Edit(const Edit&) = delete;
    Edit& operator=(const Edit&) = delete;
    Edit(Edit&&) = default;
    Edit& operator=(Edit&&) = default;

    ~Edit() override {
        if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
    }

    void setText(const std::string& t) override {
        const std::string clean = filter(t);
        if (text != clean) { text = clean; dirty = true; }
        pos = static_cast<int>(text.size());                  // курсор в конец
    }
    const std::string& getText() const override { return text; }

    void setFocus(bool f) override {
        if (isFocused == f) return;
        isFocused = f;
        if (f) SDL_StartTextInput(); else SDL_StopTextInput();
    }
    bool getFocus() const override { return isFocused; }

    void setPos(int p) override { pos = std::clamp(p, 0, static_cast<int>(text.size())); }
    int  getPos() const override { return pos; }

    void setOnClick(std::function<void()> c) override { onClick = std::move(c); }
    void setOnHover(std::function<void()> h) override { onHover = std::move(h); }

    void handleClick(int x, int y) {
        if (!isEnabled || !containsPoint(x, y)) return;
        setFocus(true);
        pos = posFromX(x);
        if (onClick) onClick();
    }

    void handleHover(int x, int y) {
        if (containsPoint(x, y) && onHover) onHover();
    }

    // SDL_TEXTINPUT: добавляет символы в позицию курсора
    bool handleTextInput(const char* utf8) {
        bool changed = false;
        for (const char* c = utf8; *c; ++c) {
            const unsigned char ch = static_cast<unsigned char>(*c);
            if (ch < 32 || ch > 126) continue;                // шрифту нужен печатный ASCII
            if (static_cast<int>(text.size()) >= maxLength) break;
            text.insert(text.begin() + pos, static_cast<char>(ch));
            ++pos;
            changed = true;
        }
        if (changed) dirty = true;
        return true;
    }

    // SDL_KEYDOWN: true, если клавишу поглотило поле
    bool handleKey(SDL_Scancode key) {
        const int n = static_cast<int>(text.size());
        switch (key) {
            case SDL_SCANCODE_BACKSPACE:
                if (pos > 0) { text.erase(pos - 1, 1); --pos; dirty = true; }
                return true;
            case SDL_SCANCODE_DELETE:
                if (pos < n) { text.erase(pos, 1); dirty = true; }
                return true;
            case SDL_SCANCODE_LEFT:  setPos(pos - 1); return true;
            case SDL_SCANCODE_RIGHT: setPos(pos + 1); return true;
            case SDL_SCANCODE_HOME:  setPos(0);       return true;
            case SDL_SCANCODE_END:   setPos(n);       return true;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
                setFocus(false);
                if (onSubmit) onSubmit();
                return true;
            case SDL_SCANCODE_ESCAPE:
                setFocus(false);
                return true;
            case SDL_SCANCODE_V:                              // Ctrl+V: удобно для IP-адреса
                if (SDL_GetModState() & KMOD_CTRL) {
                    if (char* clip = SDL_GetClipboardText()) { handleTextInput(clip); SDL_free(clip); }
                    return true;
                }
                return false;
            default:
                return false;
        }
    }

    void draw(Visualizer& v) const override {
        if (!isVisible) return;
        if (dirty) const_cast<Edit*>(this)->rebuildTexture(v);

        v.fillRect(rect, border);                                        // рамка в 1 px и фон
        v.fillRect({rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2}, background);

        const int viewW = rect.w - 2 * paddingX;
        const int textY = rect.y + (rect.h - textH) / 2;

        int cursorPx = 0;                                                // ширина текста до курсора
        if (font && pos > 0) { int h = 0; TTF_SizeUTF8(font, text.substr(0, pos).c_str(), &cursorPx, &h); }

        if (cursorPx - scrollPx > viewW) scrollPx = cursorPx - viewW;    // курсор всегда в видимой части
        if (cursorPx - scrollPx < 0)     scrollPx = cursorPx;
        if (textW <= viewW)              scrollPx = 0;

        if (texture && textW > 0) {
            const int visible = std::min(viewW, textW - scrollPx);
            if (visible > 0)
                v.drawTexture(texture, rect.x + paddingX, textY, visible, textH, scrollPx, 0, visible, textH);
        }

        if (isFocused && (SDL_GetTicks() / 500) % 2 == 0)
            v.fillRect({rect.x + paddingX + cursorPx - scrollPx, textY, 2, textH}, color);
    }

private:
    mutable bool dirty{true};
    mutable int  scrollPx{0};
    mutable int  textW{0};
    mutable int  textH{0};

    std::string filter(const std::string& t) const {
        std::string out;
        for (unsigned char ch : t)
            if (ch >= 32 && ch <= 126 && static_cast<int>(out.size()) < maxLength)
                out += static_cast<char>(ch);
        return out;
    }

    int posFromX(int x) const {                                          // ближайшая граница символа к клику
        if (!font) return static_cast<int>(text.size());
        const int local = x - (rect.x + paddingX) + scrollPx;
        int prev = 0;
        for (int i = 1; i <= static_cast<int>(text.size()); ++i) {
            int w = 0, h = 0;
            TTF_SizeUTF8(font, text.substr(0, i).c_str(), &w, &h);
            if (local < (prev + w) / 2) return i - 1;
            prev = w;
        }
        return static_cast<int>(text.size());
    }

    void rebuildTexture(Visualizer& v) {
        if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
        textW = 0;
        textH = font ? TTF_FontHeight(font) : 0;
        dirty = false;
        if (!font || text.empty()) return;

        SDL_Surface* s = TTF_RenderUTF8_Blended(font, text.c_str(), color);   // одна строка, без переноса
        if (!s) return;
        textW = s->w;
        textH = s->h;
        texture = SDL_CreateTextureFromSurface(v.getRenderer(), s);
        SDL_FreeSurface(s);
    }
};

#endif //PROJECT_NAME_BUTTON_H