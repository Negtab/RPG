#include "Panel.h"
#include "../../controllers/visualizer.h"

void Panel::draw(Visualizer& v) const {
    if (!isVisible) return;
    for (const auto& key : order) {
        if (auto it = buttons.find(key); it != buttons.end())
            { it->second->draw(v); continue; }
        if (auto it = images.find(key);  it != images.end())
            { it->second->draw(v); continue; }
        if (auto it = labels.find(key);  it != labels.end())
            { it->second->draw(v); continue; }
        if (auto it = edits.find(key);   it != edits.end())
            { it->second->draw(v); continue; }
        if (auto it = panels.find(key);  it != panels.end())
            { it->second->draw(v); continue; }
        SDL_Log("Panel::draw: element '%s' not found in order", key.c_str());
    }
}

void Panel::handleClick(int x, int y) {
    if (!isEnabled || !isVisible) return;
    for (auto& [id, btn] : buttons) {
        if (btn->isEnabled && btn->containsPoint(x, y)) {
            btn->handleClick(x, y);
            return;
        }
    }
    for (auto& [id, edt] : edits) {
        if (edt->isEnabled && edt->containsPoint(x, y)) {
            edt->handleClick(x, y);
            return;
        }
    }
    for (auto& [id, panel] : panels)
        panel->handleClick(x, y);
}

void Panel::handleHover(int x, int y) {
    if (!isEnabled || !isVisible) return;
    for (auto& [id, btn] : buttons) {
        if (btn->containsPoint(x, y)) {
            btn->handleHover(x, y);
            return;
        }
    }
    for (auto& [id, panel] : panels)
        panel->handleHover(x, y);
}

UIObject* Panel::find(const std::string& id) {
    if (auto it = buttons.find(id); it != buttons.end()) return it->second.get();
    if (auto it = images.find(id);  it != images.end())  return it->second.get();
    if (auto it = labels.find(id);  it != labels.end())  return it->second.get();
    if (auto it = edits.find(id);   it != edits.end())   return it->second.get();
    if (auto it = panels.find(id);  it != panels.end())  return it->second.get();

    // Рекурсивный поиск в дочерних панелях
    for (auto& [childId, childPanel] : panels) {
        if (auto* found = childPanel->find(id))
            return found;
    }
    return nullptr;
}

Panel* Panel::findPanel(const std::string& id) {
    // Сначала ищем на текущем уровне
    auto it = panels.find(id);
    if (it != panels.end()) return it->second.get();

    // Рекурсивно в дочерних панелях
    for (auto& [childId, childPanel] : panels) {
        if (auto* found = childPanel->findPanel(id))
            return found;
    }
    return nullptr;
}

void Panel::addPanel(const std::string& id) {
    if (panels.contains(id)) {
        SDL_Log("Panel::addPanel: panel '%s' already exists", id.c_str());
        return;
    }
    panels[id] = std::make_unique<Panel>();
    order.push_back(id);
}

void Panel::addButton(const std::string& id, const SDL_Rect& rect,
                      SDL_Texture* texture,
                      std::function<void()> onClick,
                      std::function<void()> onHover)
{
    auto btn      = std::make_unique<Button>();
    btn->rect     = rect;
    btn->texture  = texture;
    btn->onClick  = std::move(onClick);
    btn->onHover  = std::move(onHover);
    order.push_back(id);
    buttons[id]   = std::move(btn);
}

void Panel::addEdit(const std::string& id, const SDL_Rect& rect,
                    TTF_Font* font, SDL_Color color,
                    std::function<void()> onClick,
                    std::function<void()> onHover)
{
    auto edt      = std::make_unique<Edit>();
    edt->rect     = rect;
    edt->font     = font;
    edt->color    = color;
    edt->onClick  = std::move(onClick);
    edt->onHover  = std::move(onHover);
    order.push_back(id);
    edits[id]     = std::move(edt);
}

void Panel::addImage(const std::string& id, const SDL_Rect& rect,
                     SDL_Texture* texture, SDL_Rect srect)
{
    auto img      = std::make_unique<Image>();
    img->rect     = rect;
    img->texture  = texture;
    img->srect    = srect;
    order.push_back(id);
    images[id]    = std::move(img);
}

void Panel::addLabel(const std::string& id, const SDL_Rect& backgroundRect,
                     SDL_Texture* backgroundTexture,
                     const std::string& text, TTF_Font* font,
                     SDL_Color color, int paddingX, int paddingY)
{
    auto lbl                    = std::make_unique<Label>();
    lbl->background.rect        = backgroundRect;
    lbl->background.texture     = backgroundTexture;
    lbl->font                   = font;
    lbl->color                  = color;
    lbl->paddingX               = paddingX;
    lbl->paddingY               = paddingY;
    lbl->setText(text);         // сразу помечает dirty = true
    order.push_back(id);
    labels[id]                  = std::move(lbl);
}