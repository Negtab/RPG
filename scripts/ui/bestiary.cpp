// Бестиарий: пиксельная книга, один враг на разворот.
// Открывается из игрового меню и из боя; страница, на которой остановились, запоминается до следующего раза
#include <algorithm>
#include <cctype>

#include "uiManager.h"
#include "../main/game.h"
#include "../gameLogic/gameData.h"
#include "../main/lang.h"
#include "../main/utf8.h"

namespace {
    constexpr int P = 4;                                         // «пиксель» обложки и рамок

    // Обложка и два листа. Корешок — щель между листами
    constexpr SDL_Rect kCover    {20, 14, 860, 548};
    constexpr SDL_Rect kLeftPage {44, 30, 404, 516};
    constexpr SDL_Rect kRightPage{452, 30, 404, 516};

    // Уголки для листания мышью и поле поиска
    constexpr SDL_Rect kPrevCorner{kLeftPage.x, kLeftPage.y + kLeftPage.h - 48, 48, 48};
    constexpr SDL_Rect kNextCorner{kRightPage.x + kRightPage.w - 48, kRightPage.y + kRightPage.h - 48, 48, 48};
    constexpr SDL_Rect kSearchBox {kRightPage.x + 28, kRightPage.y + 420, kRightPage.w - 56, 36};

    constexpr Uint32 kTurnMs = 320;
    constexpr int kTurnFrames = 8;                               // перелистывание рывками, как в пиксельной игре
    constexpr size_t kMaxQuery = 16;

    // Палитра книги
    constexpr SDL_Color kCoverDark {44, 20, 16, 255};
    constexpr SDL_Color kCoverMid  {112, 44, 32, 255};
    constexpr SDL_Color kCoverLight{150, 72, 48, 255};
    constexpr SDL_Color kGold      {214, 170, 72, 255};
    constexpr SDL_Color kGoldDark  {130, 92, 34, 255};
    constexpr SDL_Color kPaper     {238, 224, 190, 255};
    constexpr SDL_Color kPaperShade{222, 204, 164, 255};
    constexpr SDL_Color kPaperDeep {204, 182, 140, 255};
    constexpr SDL_Color kPaperEdge {160, 128, 88, 255};
    constexpr SDL_Color kInk       {62, 40, 28, 255};
    constexpr SDL_Color kInkDim    {128, 100, 72, 255};
    constexpr SDL_Color kInkRed    {150, 40, 32, 255};
    constexpr SDL_Color kRibbon    {176, 36, 40, 255};
    constexpr SDL_Color kRibbonDark{112, 20, 28, 255};
    constexpr SDL_Color kSilhouette{0, 0, 0, 255};             // color mod умножает: любой пиксель станет чёрным

    constexpr int kLineH = 26;

    std::string elementNames(const std::vector<Elemental>& list)
    {
        std::string s;
        for (Elemental e : list) s += (s.empty() ? "" : ", ") + tr(elementName(e));
        return s.empty() ? tr("None") : s;
    }

    std::string number(int index)                               // 0 -> "No.001"
    {
        std::string n = std::to_string(index + 1);
        return tr("No.") + std::string(n.size() < 3 ? 3 - n.size() : 0, '0') + n;
    }

    const std::vector<int>& entries() { return GameData::instance().enemyIds(); }   // порядок enemies.json
    int entryCount() { return static_cast<int>(entries().size()); }
}

// ── Открытие и закрытие ─────────────────────────────

void UIManager::openBestiary()
{
    bestiaryOpen = true;
    bestiaryIndex = std::clamp(bestiaryIndex, 0, std::max(0, entryCount() - 1));
    bestiaryTurnAt = 0;
    bestiaryQuery.clear();
    bestiaryMatches = 0;
}

void UIManager::openBestiaryFromBattle()
{
    const UIObject* actions = findUIObject("ActionPanel", "Battle");
    if (uiState == UIChooseState::ChooseAction && actions && actions->isVisible)
        openBestiary();
}

void UIManager::closeBestiary()
{
    setBestiarySearch(false);
    bestiaryOpen = false;
}

void UIManager::setBestiarySearch(bool on)
{
    if (bestiarySearching == on) return;
    bestiarySearching = on;
    if (on) SDL_StartTextInput(); else SDL_StopTextInput();
}

void UIManager::turnBestiaryPage(int index)
{
    index = std::clamp(index, 0, std::max(0, entryCount() - 1));
    if (index == bestiaryIndex) return;
    bestiaryTurnFrom = bestiaryIndex;
    bestiaryIndex = index;
    bestiaryTurnAt = SDL_GetTicks();
}

// Поиск только среди встреченных: имя неизвестного врага не должно выдавать себя.
// Число — сразу на страницу с этим номером
void UIManager::runBestiarySearch(bool next)
{
    bestiaryMatches = bestiaryMatchPos = 0;
    if (bestiaryQuery.empty()) return;
    const int n = entryCount();

    if (std::all_of(bestiaryQuery.begin(), bestiaryQuery.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); }))
    {
        const int no = std::stoi(bestiaryQuery.substr(0, 6));
        if (no < 1 || no > n) return;
        bestiaryMatches = bestiaryMatchPos = 1;
        turnBestiaryPage(no - 1);
        return;
    }

    const std::string q = utf8::lower(bestiaryQuery);
    std::vector<int> hits;
    for (int i = 0; i < n; ++i)
    {
        const EnemyDef* def = GameData::instance().enemy(entries()[i]);
        if (def && player.isEnemyDiscovered(def->id) && utf8::lower(def->name).find(q) != std::string::npos)
            hits.push_back(i);
    }
    if (hits.empty()) return;

    // Печатаем — первое совпадение с начала книги; Enter — следующее после текущей страницы, по кругу
    int target = hits.front();
    if (next)
        if (const auto it = std::upper_bound(hits.begin(), hits.end(), bestiaryIndex); it != hits.end()) target = *it;
    bestiaryMatches = static_cast<int>(hits.size());
    bestiaryMatchPos = static_cast<int>(std::find(hits.begin(), hits.end(), target) - hits.begin()) + 1;
    turnBestiaryPage(target);
}

// ── Ввод ─────────────────────────────────────────────

void UIManager::handleBestiaryEvent(const SDL_Event& e)
{
    const int n = entryCount();
    const bool key = e.type == SDL_KEYDOWN;
    const SDL_Scancode k = key ? e.key.keysym.scancode : SDL_SCANCODE_UNKNOWN;
    const bool click = e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
    const SDL_Point m{e.button.x, e.button.y};

    if (bestiarySearching)
    {
        if (e.type == SDL_TEXTINPUT)
        {
            const std::string in(e.text.text);                    // буквы (и русские), цифры, пробел, дефис, апостроф
            for (size_t i = 0; i < in.size(); i = utf8::nextBoundary(in, i))
            {
                const auto c = static_cast<unsigned char>(in[i]);
                if (utf8::length(bestiaryQuery) < kMaxQuery &&
                    (std::isalnum(c) || c == ' ' || c == '-' || c == '\'' || c >= 0xC0))
                    bestiaryQuery.append(in, i, utf8::nextBoundary(in, i) - i);
            }
            runBestiarySearch(false);
            return;
        }
        if (key)
        {
            switch (k)
            {
                case SDL_SCANCODE_BACKSPACE:
                    if (!bestiaryQuery.empty()) utf8::popBack(bestiaryQuery);
                    runBestiarySearch(false);
                    break;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_KP_ENTER: runBestiarySearch(true); break;
                case SDL_SCANCODE_ESCAPE:   bestiaryQuery.clear(); bestiaryMatches = 0; setBestiarySearch(false); break;
                case SDL_SCANCODE_TAB:      setBestiarySearch(false); break;
                case SDL_SCANCODE_LEFT:     turnBestiaryPage(bestiaryIndex - 1); break;
                case SDL_SCANCODE_RIGHT:    turnBestiaryPage(bestiaryIndex + 1); break;
                default: break;                                    // буквы приходят через SDL_TEXTINPUT
            }
            return;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) { setBestiarySearch(false); return; }
        if (click && !SDL_PointInRect(&m, &kSearchBox)) setBestiarySearch(false);   // и обрабатываем клик дальше
        else if (e.type != SDL_MOUSEWHEEL) return;
    }

    if ((key && e.key.repeat == 0 && k == SDL_SCANCODE_ESCAPE) ||
        (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT))
    {
        closeBestiary();
        return;
    }

    if (key)
        switch (k)
        {
            case SDL_SCANCODE_LEFT:  case SDL_SCANCODE_A: turnBestiaryPage(bestiaryIndex - 1); break;
            case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D: turnBestiaryPage(bestiaryIndex + 1); break;
            case SDL_SCANCODE_Q: case SDL_SCANCODE_PAGEUP:   turnBestiaryPage(bestiaryIndex - 10); break;
            case SDL_SCANCODE_E: case SDL_SCANCODE_PAGEDOWN: turnBestiaryPage(bestiaryIndex + 10); break;
            case SDL_SCANCODE_HOME: turnBestiaryPage(0); break;
            case SDL_SCANCODE_END:  turnBestiaryPage(n - 1); break;
            case SDL_SCANCODE_TAB:
                if (e.key.repeat == 0) { setBestiarySearch(true); runBestiarySearch(false); }
                break;
            default: break;
        }

    if (e.type == SDL_MOUSEWHEEL && e.wheel.y != 0)
        turnBestiaryPage(bestiaryIndex + (e.wheel.y > 0 ? -1 : 1));

    if (click)
    {
        if (SDL_PointInRect(&m, &kPrevCorner)) turnBestiaryPage(bestiaryIndex - 1);
        else if (SDL_PointInRect(&m, &kNextCorner)) turnBestiaryPage(bestiaryIndex + 1);
        else if (SDL_PointInRect(&m, &kSearchBox)) { setBestiarySearch(true); runBestiarySearch(false); }
    }
}

// ── Отрисовка ────────────────────────────────────────

namespace {
    // Лист бумаги: тень к корешку, тёмный край снаружи
    void drawPaper(const Visualizer& v, const SDL_Rect& r, bool spineOnLeft)
    {
        v.fillRect(r, kPaper);
        const SDL_Color shades[] = {kPaperDeep, kPaperShade, kPaperShade};
        for (int i = 0; i < 3; ++i)
        {
            const int x = spineOnLeft ? r.x + i * P : r.x + r.w - (i + 1) * P;
            if (x >= r.x && x + P <= r.x + r.w) v.fillRect({x, r.y, P, r.h}, shades[i]);
        }
        const int edgeX = spineOnLeft ? r.x + r.w - 2 : r.x;
        if (r.w >= 2) v.fillRect({edgeX, r.y, 2, r.h}, kPaperEdge);
        v.fillRect({r.x, r.y, r.w, 2}, kPaperEdge);
        v.fillRect({r.x, r.y + r.h - 2, r.w, 2}, kPaperEdge);
    }

    // Узор-разделитель: линия с ромбом посередине
    void drawDivider(const Visualizer& v, int cx, int y, int halfW)
    {
        v.fillRect({cx - halfW, y, halfW - 2 * P, 2}, kInkDim);
        v.fillRect({cx + 2 * P, y, halfW - 2 * P, 2}, kInkDim);
        v.fillRect({cx - P / 2, y - P - 1, P, P}, kInkRed);
        v.fillRect({cx - P - P / 2, y - 1, P, P}, kInkRed);
        v.fillRect({cx + P / 2, y - 1, P, P}, kInkRed);
        v.fillRect({cx - P / 2, y + P - 1, P, P}, kInkRed);
    }
}

int UIManager::drawWrapped(const std::string& text, int x, int y, int width, int maxLines, SDL_Color color) const
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    std::vector<std::string> lines;
    std::string line, word;
    auto flush = [&]() {
        if (word.empty()) return;
        const std::string tryLine = line.empty() ? word : line + " " + word;
        if (!line.empty() && ResourceManager::measureText(tryLine, font).w > width) { lines.push_back(line); line = word; }
        else line = tryLine;
        word.clear();
    };
    for (char c : text)
        if (c == ' ') flush(); else word += c;
    flush();
    if (!line.empty()) lines.push_back(line);

    if (static_cast<int>(lines.size()) > maxLines)                  // не влезло: последняя строка с многоточием
    {
        lines.resize(maxLines);
        std::string& last = lines.back();
        while (!last.empty() && ResourceManager::measureText(last + "...", font).w > width) utf8::popBack(last);
        while (!last.empty() && (last.back() == ',' || last.back() == ' ')) last.pop_back();
        last += "...";
    }
    for (const auto& l : lines) { drawText(l, x, y, color); y += kLineH; }
    return y;
}

void UIManager::drawBestiaryCorner(const SDL_Rect& r, bool right, bool enabled)
{
    // Загнутый уголок страницы: ступенчатый треугольник, на нём стрелка
    const int steps = r.w / P;
    for (int i = 0; i < steps; ++i)
    {
        const int w = (i + 1) * P;                                   // сверху узко, снизу широко
        const int x = right ? r.x + r.w - w : r.x;
        visualizer.fillRect({x, r.y + i * P, w, P}, kPaperDeep);
        visualizer.fillRect({right ? x : x + w - P, r.y + i * P, P, P}, kPaperEdge);   // линия сгиба
    }
    if (!enabled) return;
    const int cy = r.y + r.h - 3 * P - 2;
    const int tip = right ? r.x + r.w - 2 * P : r.x + 2 * P;
    const int pulse = (SDL_GetTicks() / 300) % 2;
    for (int i = 0; i < 4; ++i)                                     // треугольник-стрелка 4 «пикселя» в длину
    {
        const int x = right ? tip - (i + 1) * P - pulse * 2 : tip + i * P + pulse * 2;
        visualizer.fillRect({x, cy - i * P / 2 - P / 2, P, i * P + P}, kInkRed);
    }
}

void UIManager::drawBestiaryLeft(int index, const SDL_Rect& page)
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    if (index < 0 || index >= entryCount()) return;
    const EnemyDef* def = GameData::instance().enemy(entries()[index]);
    if (!def) return;
    const bool known = player.isEnemyDiscovered(def->id);

    drawText(number(index), page.x + 28, page.y + 18, kInkDim);
    const std::string title = tr("Bestiary");
    drawText(title, page.x + page.w - 28 - ResourceManager::measureText(title, font).w, page.y + 18, kInkRed);

    const std::string name = known ? def->name : "???";
    drawText(name, page.x + (page.w - ResourceManager::measureText(name, font).w) / 2, page.y + 58, kInk);
    drawDivider(visualizer, page.x + page.w / 2, page.y + 100, page.w / 2 - 36);

    // Рамка с портретом: пунктирный бордюр, спрайт целым масштабом
    const SDL_Rect frame{page.x + 40, page.y + 116, page.w - 80, 290};
    visualizer.fillRect(frame, kPaperShade);
    for (int x = frame.x; x < frame.x + frame.w; x += 2 * P)
    {
        visualizer.fillRect({x, frame.y, P, P / 2}, kPaperEdge);
        visualizer.fillRect({x, frame.y + frame.h - P / 2, P, P / 2}, kPaperEdge);
    }
    for (int y = frame.y; y < frame.y + frame.h; y += 2 * P)
    {
        visualizer.fillRect({frame.x, y, P / 2, P}, kPaperEdge);
        visualizer.fillRect({frame.x + frame.w - P / 2, y, P / 2, P}, kPaperEdge);
    }

    SDL_Texture* tex = ResourceManager::getTexture(def->texture);
    if (tex)
    {
        SDL_Rect src = def->srcRect;
        if (src.w <= 0 || src.h <= 0) { src = {0, 0, 0, 0}; SDL_QueryTexture(tex, nullptr, nullptr, &src.w, &src.h); }
        const int scale = std::clamp(std::min((frame.w - 40) / std::max(1, src.w), (frame.h - 50) / std::max(1, src.h)), 1, 6);
        const int w = src.w * scale, h = src.h * scale;
        const int x = frame.x + (frame.w - w) / 2;
        const int y = frame.y + (frame.h - 24 - h) / 2;

        const int shadowW = std::max(w * 3 / 4, 8 * P);               // пиксельная тень-овал под ногами
        const int sy = y + h - P;
        visualizer.fillRect({x + (w - shadowW) / 2 + 2 * P, sy, shadowW - 4 * P, P}, kPaperDeep);
        visualizer.fillRect({x + (w - shadowW) / 2, sy + P, shadowW, P}, kPaperDeep);
        visualizer.fillRect({x + (w - shadowW) / 2 + 2 * P, sy + 2 * P, shadowW - 4 * P, P}, kPaperDeep);

        if (!known) SDL_SetTextureColorMod(tex, kSilhouette.r, kSilhouette.g, kSilhouette.b);
        visualizer.drawTexture(tex, x, y, w, h, src.x, src.y, src.w, src.h);
        SDL_SetTextureColorMod(tex, 255, 255, 255);                  // текстура общая с боем
    }

    // Сколько открыто
    int seen = 0;
    for (int id : entries()) seen += player.isEnemyDiscovered(id) ? 1 : 0;
    const std::string count = trf("Seen {0} / {1}", {std::to_string(seen), std::to_string(entryCount())});
    drawText(count, page.x + (page.w - ResourceManager::measureText(count, font).w) / 2, page.y + 424, kInkDim);

    const std::string folio = std::to_string(index * 2 + 1);           // номер страницы внизу
    drawText(folio, page.x + (page.w - ResourceManager::measureText(folio, font).w) / 2, page.y + page.h - 38, kInkDim);
    drawBestiaryCorner(kPrevCorner, false, index > 0);
}

void UIManager::drawBestiaryRight(int index, const SDL_Rect& page)
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    if (index < 0 || index >= entryCount()) return;
    const EnemyDef* def = GameData::instance().enemy(entries()[index]);
    if (!def) return;
    const bool known = player.isEnemyDiscovered(def->id);

    const int x = page.x + 28, w = page.w - 56;
    int y = page.y + 18;
    auto section = [&](const std::string& heading, const std::string& value, int maxLines) {
        visualizer.fillRect({x, y + 9, P, P}, kInkRed);                  // маркер раздела
        drawText(heading, x + 2 * P + 4, y, kInkRed);
        y = drawWrapped(known ? value : "???", x + 2 * P + 4, y + kLineH + 2, w - 2 * P - 4, maxLines, kInk) + 8;
    };

    std::string skills;
    for (const EnemyAbility& a : def->abilities)
    {
        std::string s = tr("Attack");
        if (a.skillId >= 0)
        {
            const auto it = GameData::instance().skills().find(a.skillId);
            if (it == GameData::instance().skills().end()) continue;
            s = it->second.name;
        }
        else if (def->hits > 1) s += " x" + std::to_string(def->hits);
        if (skills.find(s) == std::string::npos) skills += (skills.empty() ? "" : ", ") + s;
    }

    std::string places;
    for (const auto& h : GameData::instance().habitats(def->id)) places += (places.empty() ? "" : ", ") + tr(h);

    section(tr("Weak"),    elementNames(def->vulnerabilities), 1);
    section(tr("Resists"), elementNames(def->resists), 1);
    section(tr("Immune"),  elementNames(def->immunities), 1);
    section(tr("Habitat"), places.empty() ? tr("Unknown") : places, 3);
    section(tr("Skills"),  skills.empty() ? tr("None") : skills, 2);

    // Поле поиска: рамка-«чернильница», при вводе мигает курсор
    const SDL_Rect box = kSearchBox;
    visualizer.fillRect(box, bestiarySearching ? kPaper : kPaperShade);
    visualizer.fillRect({box.x, box.y + box.h - P / 2, box.w, P / 2}, bestiarySearching ? kInkRed : kPaperEdge);
    visualizer.fillRect({box.x, box.y, box.w, 2}, kPaperEdge);
    visualizer.fillRect({box.x, box.y, 2, box.h}, kPaperEdge);
    visualizer.fillRect({box.x + box.w - 2, box.y, 2, box.h}, kPaperEdge);

    std::string shown = bestiaryQuery;
    if (bestiarySearching && (SDL_GetTicks() / 400) % 2 == 0) shown += "_";
    if (shown.empty() && !bestiarySearching) drawText(tr("Search (Tab)"), box.x + 10, box.y + 6, kInkDim);
    else drawText(shown, box.x + 10, box.y + 6, kInk);
    if (!bestiaryQuery.empty())
    {
        const std::string info = bestiaryMatches > 0 ? std::to_string(bestiaryMatchPos) + "/" + std::to_string(bestiaryMatches)
                                                     : tr("No match");
        drawText(info, box.x + box.w - 10 - ResourceManager::measureText(info, font).w, box.y + 6,
                 bestiaryMatches > 0 ? kInkDim : kInkRed);
    }

    const std::string folio = std::to_string(index * 2 + 2);
    drawText(folio, page.x + (page.w - ResourceManager::measureText(folio, font).w) / 2, page.y + page.h - 38, kInkDim);
    drawBestiaryCorner(kNextCorner, true, index + 1 < entryCount());
}

void UIManager::drawBestiary()
{
    SDL_Renderer* r = visualizer.getRenderer();
    visualizer.fillRect(screen, {0, 0, 0, 150});

    // Обложка: тёмный контур, кожа, светлый блик сверху, золотые уголки
    visualizer.fillRect(kCover, kCoverDark);
    const SDL_Rect leather{kCover.x + P, kCover.y + P, kCover.w - 2 * P, kCover.h - 2 * P};
    visualizer.fillRect(leather, kCoverMid);
    visualizer.fillRect({leather.x, leather.y, leather.w, P}, kCoverLight);
    visualizer.fillRect({leather.x, leather.y + leather.h - P, leather.w, P}, kCoverDark);
    for (const SDL_Point c : {SDL_Point{kCover.x, kCover.y}, SDL_Point{kCover.x + kCover.w - 4 * P, kCover.y},
                              SDL_Point{kCover.x, kCover.y + kCover.h - 4 * P},
                              SDL_Point{kCover.x + kCover.w - 4 * P, kCover.y + kCover.h - 4 * P}})
    {
        visualizer.fillRect({c.x, c.y, 4 * P, 4 * P}, kGoldDark);
        visualizer.fillRect({c.x + P / 2, c.y + P / 2, 3 * P, 3 * P}, kGold);
        visualizer.fillRect({c.x + 3 * P / 2, c.y + 3 * P / 2, P, P}, kGoldDark);   // заклёпка
    }

    // Стопка листов под страницами и корешок
    for (int i = 2; i >= 1; --i)
        for (const SDL_Rect& pg : {kLeftPage, kRightPage})
            visualizer.fillRect({pg.x + i * 2, pg.y + i * 3, pg.w - i * 4, pg.h}, i == 2 ? kPaperEdge : kPaperDeep);
    visualizer.fillRect({kLeftPage.x + kLeftPage.w - P, kLeftPage.y - P, 2 * P + kRightPage.x - kLeftPage.x - kLeftPage.w,
                         kLeftPage.h + 3 * P}, kCoverDark);

    // Листаем: снизу старая левая и новая правая (или наоборот), сверху лист,
    // который сначала сжимается к корешку, а потом раскрывается на другой стороне
    const int to = bestiaryIndex;
    float t = 1.0f;
    if (bestiaryTurnAt)
    {
        const Uint32 age = SDL_GetTicks() - bestiaryTurnAt;
        if (age >= kTurnMs) bestiaryTurnAt = 0;
        else t = static_cast<float>(age * kTurnFrames / kTurnMs) / kTurnFrames;
    }
    const bool turning = bestiaryTurnAt != 0;
    const bool forward = to > bestiaryTurnFrom;
    const int from = turning ? bestiaryTurnFrom : to;

    drawPaper(visualizer, kLeftPage, false);
    drawPaper(visualizer, kRightPage, true);
    drawBestiaryLeft(turning && forward ? from : to, kLeftPage);
    drawBestiaryRight(turning && !forward ? from : to, kRightPage);

    if (turning)
    {
        const bool firstHalf = t < 0.5f;
        const float k = firstHalf ? 1.0f - 2.0f * t : 2.0f * t - 1.0f;
        // Первая половина: лист отрывается от своей стороны; вторая: ложится на другую
        const bool onRight = forward == firstHalf;
        const SDL_Rect& pg = onRight ? kRightPage : kLeftPage;
        const int w = std::max(P, static_cast<int>(pg.w * k) / P * P);
        const SDL_Rect sheet = onRight ? SDL_Rect{pg.x, pg.y, w, pg.h} : SDL_Rect{pg.x + pg.w - w, pg.y, w, pg.h};
        drawPaper(visualizer, sheet, onRight);
        SDL_RenderSetClipRect(r, &sheet);
        const int shownIndex = firstHalf ? from : to;
        if (onRight) drawBestiaryRight(shownIndex, kRightPage);
        else         drawBestiaryLeft(shownIndex, kLeftPage);
        SDL_RenderSetClipRect(r, nullptr);
        const int edge = onRight ? sheet.x + sheet.w - P : sheet.x;        // тень у свободного края листа
        visualizer.fillRect({edge, sheet.y, P, sheet.h}, kPaperDeep);
    }

    // Закладка-ленточка сверху правой страницы с раздвоенным концом (короткая: не закрывает текст)
    const int rx = kRightPage.x + kRightPage.w - 80;
    const int rh = kRightPage.y + 5 * P - kCover.y;
    visualizer.fillRect({rx, kCover.y, 4 * P, rh}, kRibbon);
    visualizer.fillRect({rx + 3 * P, kCover.y, P, rh}, kRibbonDark);
    visualizer.fillRect({rx, kCover.y + rh, P, P}, kRibbon);
    visualizer.fillRect({rx + 3 * P, kCover.y + rh, P, P}, kRibbonDark);

    const std::string hint = tr(bestiarySearching ? "Type a name or No.   Enter - next   Esc - clear"
                                                  : "A/D - turn   Q/E - 10 pages   Tab - search   Esc - close");
    drawOutlinedText(hint, screen.w / 2, kCover.y + kCover.h + 6, {255, 255, 255, 255}, 255);
}
