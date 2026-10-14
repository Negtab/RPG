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

    void draw(Visualizer& v) const override {
        if (!isVisible) return;
        SDL_Texture* skin = texture ? texture : getWindowSkin(v.getRenderer());   // свой скин можно подложить в texture
        v.drawNinePatch(skin, rect, slice, scale);
    }
};


#endif //PROJECT_NAME_WINDOW_H