// windowSkin.h
#ifndef PROJECT_NAME_WINDOWSKIN_H
#define PROJECT_NAME_WINDOWSKIN_H

#include <SDL.h>

constexpr int kWindowSkinSlice = 8;    // граница скина в его пикселях (углы 8x8)
constexpr int kWindowSkinSize  = 32;

struct WindowStyle {

    SDL_Color top    {  24,  40, 168, 255 };   // верх градиента
    SDL_Color bottom {   4,  10,  80, 255 };   // низ градиента
    SDL_Color border { 232, 232, 240, 255 };   // внешняя рамка
    SDL_Color shadow {  16,  24, 120, 255 };   // тонкая линия внутри рамки (даёт объём)
    int unit   = 2;                            // толщина «пикселя» рамки в экранных пикселях
    int radius = 3;                            // радиус скругления в unit'ах
};

// Возвращает текстуру ровно w*h. Рисуй её в прямоугольник того же размера, тогда без растяжения
SDL_Texture* makeWindowTexture(SDL_Renderer* renderer, int w, int h, const WindowStyle& style = {});
SDL_Texture* getWindowSkin(SDL_Renderer* renderer);   // создаётся один раз и общий для всех окон

#endif