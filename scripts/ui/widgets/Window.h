//
// Created by Artem on 12.10.2026.
//

#ifndef PROJECT_NAME_WINDOW_H
#define PROJECT_NAME_WINDOW_H

#include "Image.h"
#include "../../controllers/visualizer.h"
#include "../windowSkin.h"

class Window : public Image {
public:
    int slice = kWindowSkinSlice;     // граница скина
    int scale = 1;                    // 1 = рамка 2 px; 2 = вдвое толще (для крупных окон)
    Uint8 alpha = 255;                // меньше 255 — сквозь окно видно, что под ним (окна боя поверх задника)

    void draw(Visualizer& v) const override {
        if (!isVisible) return;
        SDL_Texture* skin = texture ? texture : getWindowSkin(v.getRenderer());   // свой скин можно подложить в texture
        if (alpha != 255) SDL_SetTextureAlphaMod(skin, alpha);
        v.drawNinePatch(skin, rect, slice, scale);
        if (alpha != 255) SDL_SetTextureAlphaMod(skin, 255);   // скин общий для всех окон
    }
};


#endif //PROJECT_NAME_WINDOW_H