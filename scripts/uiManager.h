#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include <string>

#include "SDL_events.h"
#include "visualizer.h"

class UIManager {
public:
    explicit UIManager(Visualizer& visualizer) : visualizer(visualizer) {}

    void addButton(const std::string& id, const SDL_Rect& rect, SDL_Texture* texture, std::function<void()> onClick);
    void handleEvent(const SDL_Event& event);
    void drawAll();

private:
    struct Button
    {
        SDL_Rect rect;
        SDL_Texture* texture;
        std::function<void()> onClick;
    };

    std::map<std::string, Button> buttons;
    Visualizer& visualizer;
};

#endif