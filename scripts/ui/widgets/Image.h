#ifndef PROJECT_NAME_IMAGE_H
#define PROJECT_NAME_IMAGE_H

#include "SDL_pixels.h"
#include "SDL_ttf.h"
#include "../core/UIObject.h"
#include "../../controllers/resourceManager.h"

class Visualizer;

class Image : public UIObject {
public:
    SDL_Rect srect{0,0,0,0};

    void draw(Visualizer& v) const override {
        if (!isVisible || !texture) return;
        if (srect.w == 0 || srect.h == 0)
            v.drawTexture(texture, rect.x, rect.y, rect.w, rect.h);
        else
            v.drawTexture(texture, rect.x, rect.y, rect.w, rect.h,
                          srect.x, srect.y, srect.w, srect.h);
    }
};

class Label : public UIObject {
public:
    int paddingX = 8;
    int paddingY = 4;
    Image background;
    std::string text;
    TTF_Font* font = nullptr;
    SDL_Color color{0,0,0,255};

    Label() = default;
    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;
    Label(Label&&) = default;
    Label& operator=(Label&&) = default;

    ~Label() override {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    void setText(const std::string& t) override {
        if (text != t) { text = t; dirty = true; }
    }

    void draw(Visualizer& v) const override {
        if (!isVisible) return;
        if (dirty) const_cast<Label*>(this)->rebuildTexture(v);
        background.draw(v);
        if (texture)
            v.drawTexture(texture, rect.x, rect.y, rect.w, rect.h);
    }

private:
    mutable bool dirty = true;

    void rebuildTexture(Visualizer& v) {
        if (!font || text.empty()) { dirty = false; return; }
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        auto block = ResourceManager::renderMultiline(v.getRenderer(), text, font, color);
        texture = block.texture;
        rect = { background.rect.x + paddingX,
                 background.rect.y + paddingY,
                 block.w, block.h };
        background.rect.w = block.w + paddingX * 2;
        background.rect.h = block.h + paddingY * 2;
        dirty = false;
    }
};

#endif