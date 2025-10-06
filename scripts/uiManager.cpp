#include "uiManager.h"

void UIManager::addButton(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick)
{
    buttons[id] = {rect, texture, onClick};
}

void UIManager::drawAll()
{
    for (auto& [_, btn] : buttons)
        visualizer.drawTexture(btn.texture, btn.rect.x, btn.rect.y, btn.rect.w, btn.rect.h);
}

void UIManager::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_MOUSEBUTTONDOWN)
    {
        int x = event.button.x;
        int y = event.button.y;

        for (auto& [_, btn] : buttons)
        {
            if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
                y >= btn.rect.y && y <= btn.rect.y + btn.rect.h)
            {
                if (btn.onClick) btn.onClick();
                break;
            }
        }
    }
}
