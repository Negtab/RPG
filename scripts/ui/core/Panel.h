#ifndef PROJECT_NAME_PANEL_H
#define PROJECT_NAME_PANEL_H

#include <map>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_ttf.h"
#include "UIObject.h"
#include "../widgets/Button.h"
#include "../widgets/Image.h"

class Visualizer;

class Panel : public UIObject {
public:
    std::map<std::string, std::unique_ptr<Button>> buttons;
    std::map<std::string, std::unique_ptr<Edit>>   edits;
    std::map<std::string, std::unique_ptr<Label>>  labels;
    std::map<std::string, std::unique_ptr<Image>>  images;
    std::map<std::string, std::unique_ptr<Panel>>  panels;
    std::vector<std::string> order;

    void draw(Visualizer& v) const override;

    void handleClick(int x, int y);
    void handleHover(int x, int y);

    void addPanel(const std::string& id);
    void addButton(const std::string& id, const SDL_Rect& rect,
                   SDL_Texture* texture,
                   std::function<void()> onClick,
                   std::function<void()> onHover);
    void addEdit(const std::string& id, const SDL_Rect& rect,
                 TTF_Font* font, SDL_Color color,
                 std::function<void()> onClick,
                 std::function<void()> onHover);
    void addImage(const std::string& id, const SDL_Rect& rect,
                  SDL_Texture* texture, SDL_Rect srect = {0,0,0,0});
    void addLabel(const std::string& id, const SDL_Rect& backgroundRect,
                  SDL_Texture* backgroundTexture,
                  const std::string& text, TTF_Font* font,
                  SDL_Color color = {0,0,0,255},
                  int paddingX = 8, int paddingY = 4);

    // Поиск любого виджета по id рекурсивно
    UIObject* find(const std::string& id);
    Panel*    findPanel(const std::string& id);
};

#endif