// Жители городов: спрайт на карте, окно разговора и магазин в стиле FF1 Pixel Remaster.
// Магазин: сверху название и золото, под ним реплика продавца; слева «Купить / Продать / Выйти»,
// справа товары с ценами, внизу описание выбранного и кто из отряда может это надеть
#include <algorithm>

#include "uiManager.h"
#include "windowSkin.h"
#include "../main/game.h"
#include "../gameLogic/notGamePerson.h"
#include "../main/lang.h"
#include "../main/utf8.h"

namespace {
    const SDL_Color kText{255, 255, 255, 255};
    const SDL_Color kDim {150, 160, 200, 255};
    const SDL_Color kHot {255, 232, 120, 255};
    const SDL_Color kGood{120, 230, 140, 255};
    const SDL_Color kBad {255, 120, 120, 255};

    // Окно разговора внизу экрана
    constexpr SDL_Rect kTalkBox{24, 404, 852, 172};
    constexpr int kTalkLines = 3;
    constexpr int kTalkLineH = 34;
    constexpr int kTalkCharsPerSec = 60;

    // Магазин
    constexpr SDL_Rect kShopHeader{24, 24, 852, 52};
    constexpr SDL_Rect kShopTalk  {24, 84, 852, 52};
    constexpr SDL_Rect kShopCmds  {24, 144, 200, 136};
    constexpr SDL_Rect kShopList  {236, 144, 640, 320};
    constexpr SDL_Rect kShopInfo  {24, 472, 852, 104};
    constexpr SDL_Rect kShopCount {300, 214, 300, 132};
    constexpr int kShopRowH = 40;
    constexpr int kShopRows = 7;
    constexpr int kMaxCarry = 99;                                // как в FF: не больше 99 штук одного предмета

    const char* const kShopCommands[] = {"Buy", "Sell", "Exit"};

    bool keyIs(const SDL_Event& e, std::initializer_list<SDL_Scancode> keys)
    {
        if (e.type != SDL_KEYDOWN) return false;
        for (SDL_Scancode k : keys) if (e.key.keysym.scancode == k) return true;
        return false;
    }
    bool confirmKey(const SDL_Event& e)
    {
        return e.type == SDL_KEYDOWN && e.key.repeat == 0 &&
               keyIs(e, {SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_SPACE});
    }
    bool backKey(const SDL_Event& e)
    {
        return (e.type == SDL_KEYDOWN && e.key.repeat == 0 && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) ||
               (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT);
    }
    bool leftClick(const SDL_Event& e) { return e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT; }
    bool isMouse(const SDL_Event& e) { return e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN; }
    SDL_Point mousePos(const SDL_Event& e)
    {
        return e.type == SDL_MOUSEMOTION ? SDL_Point{e.motion.x, e.motion.y} : SDL_Point{e.button.x, e.button.y};
    }

    std::string gil(int amount) { return trf("{0} Gil", {std::to_string(amount)}); }
}

// ─────────────────────────────────────────
// Жители на карте
// ─────────────────────────────────────────

void UIManager::drawNpc(const NotGamePerson& npc, int cellScreenX, int cellScreenY)
{
    // Лист Npc<Имя>.png как у героя: 4 направления x 2 кадра. Как в FF1, жители всё время шагают на месте
    SDL_Texture* sheet = ResourceManager::getTexture(npc.getSprite());
    if (!sheet) return;
    int tw = 0, th = 0;
    SDL_QueryTexture(sheet, nullptr, nullptr, &tw, &th);
    const int fw = tw / 4, fh = th / 2;
    const int col = std::clamp(static_cast<int>(npc.getFacing()), 0, 3);
    const int frame = static_cast<int>((SDL_GetTicks() / 300) % 2);
    const SDL_Rect src{col * fw, frame * fh, fw, fh};
    visualizer.drawSprite(sheet, cellScreenX + CELL_PX / 2, cellScreenY + CELL_PX, PIXEL_SCALE, &src);
}

// ─────────────────────────────────────────
// Окно разговора
// ─────────────────────────────────────────

std::vector<std::string> UIManager::wrapText(const std::string& text, int width) const
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
    {
        if (c == ' ') flush();
        else if (c == '\n') { flush(); lines.push_back(line); line.clear(); }
        else word += c;
    }
    flush();
    if (!line.empty()) lines.push_back(line);
    return lines;
}

void UIManager::showDialog(const std::string& speaker, const std::vector<std::string>& pages)
{
    // Длинная реплика делится на страницы по kTalkLines строк: переносим заранее, чтобы слова
    // не перепрыгивали на следующую строку во время печати
    talkSpeaker = speaker;
    talkPages.clear();
    for (const auto& page : pages)
    {
        const auto lines = wrapText(page, kTalkBox.w - 64);
        for (size_t i = 0; i < lines.size(); i += kTalkLines)
        {
            std::string chunk;
            for (size_t k = i; k < std::min(lines.size(), i + kTalkLines); ++k)
                chunk += (chunk.empty() ? "" : "\n") + lines[k];
            talkPages.push_back(chunk);
        }
    }
    talkPage = 0;
    talkStart = SDL_GetTicks();
}

void UIManager::handleDialogEvent(const SDL_Event& e)
{
    if (!confirmKey(e) && !leftClick(e)) return;
    const size_t shown = static_cast<size_t>(SDL_GetTicks() - talkStart) * kTalkCharsPerSec / 1000;
    if (shown < utf8::length(talkPages[talkPage]))
    {
        talkStart = 0;                                            // ещё печатается: показать всю страницу сразу
        return;
    }
    if (++talkPage >= static_cast<int>(talkPages.size()))
    {
        talkPages.clear();                                        // последняя страница: окно закрывается
        return;
    }
    talkStart = SDL_GetTicks();
}

void UIManager::drawDialog()
{
    visualizer.drawNinePatch(getWindowSkin(visualizer.getRenderer()), kTalkBox, kWindowSkinSlice, 1);
    int y = kTalkBox.y + 16;
    if (!talkSpeaker.empty())
    {
        drawText(talkSpeaker, kTalkBox.x + 32, y, kHot);
        y += kTalkLineH + 4;
    }

    const std::string& page = talkPages[talkPage];
    const size_t shown = talkStart == 0 ? page.size()
                       : static_cast<size_t>(SDL_GetTicks() - talkStart) * kTalkCharsPerSec / 1000;
    const std::string visible = utf8::prefix(page, shown);
    size_t from = 0;
    while (from <= visible.size())
    {
        const size_t nl = visible.find('\n', from);
        drawText(visible.substr(from, nl == std::string::npos ? std::string::npos : nl - from), kTalkBox.x + 32, y, kText);
        if (nl == std::string::npos) break;
        from = nl + 1;
        y += kTalkLineH;
    }

    // Страница допечатана: мигающий треугольник «дальше» в правом нижнем углу
    if (visible.size() == page.size() && (SDL_GetTicks() / 400) % 2 == 0)
    {
        const int cx = kTalkBox.x + kTalkBox.w - 36, cy = kTalkBox.y + kTalkBox.h - 30;
        for (int i = 0; i < 5; ++i)
            visualizer.fillRect({cx - 10 + i * 2, cy + i * 2, 20 - i * 4, 2}, kText);
    }
}

// ─────────────────────────────────────────
// Магазин, лавка магии, гостиница, церковь
// ─────────────────────────────────────────

void UIManager::openShop(const ShopDef& def)
{
    shop = &def;
    shopPhase = ShopPhase::Command;
    shopCommand = shopSel = shopScroll = 0;
    shopCount = 1;
    if (def.kind == ShopKind::Inn)
        shopSay(trf("A night's rest is {0}. Will you stay?", {gil(def.price)}));
    else if (def.kind == ShopKind::Church)
        shopSay(tr(def.greeting.empty() ? "Shall I return your fallen to life?" : def.greeting));
    else
        shopSay(tr(def.greeting.empty() ? "Welcome!" : def.greeting));
}

std::vector<const char*> UIManager::shopCommands() const
{
    if (!shop) return {};
    if (shop->kind == ShopKind::Inn)
        return shopPhase == ShopPhase::InnSave ? std::vector<const char*>{"Save", "Exit"}
                                               : std::vector<const char*>{"Stay", "Exit"};
    if (shop->kind == ShopKind::Church) return {"Revive", "Exit"};
    if (shop->sellsSpells()) return {"Buy", "Exit"};                 // заклинания в FF1 не продают обратно
    return {kShopCommands[0], kShopCommands[1], kShopCommands[2]};
}

int UIManager::shopPrice(const Item& item, bool selling) const
{
    return selling ? item.cost / 2 : item.cost;                     // как в FF1: продаём за полцены
}

std::vector<std::pair<int, int>> UIManager::shopList() const
{
    std::vector<std::pair<int, int>> out;
    if (!shop) return out;
    if (shop->kind == ShopKind::Church)                              // павшие герои и цена воскрешения
    {
        const auto& heroes = player.getHeroes();
        for (int i = 0; i < static_cast<int>(heroes.size()); ++i)
            if (!heroes[i].getIsAlive()) out.emplace_back(i, shop->price);
        return out;
    }
    if (shop->sellsSpells())
    {
        for (int id : shop->goods)
            out.emplace_back(id, kSpellPrice[std::clamp(game.getSkill(id).spellLevel, 1, kSpellLevels) - 1]);
        return out;
    }
    const bool selling = shopPhase == ShopPhase::Sell || (shopPhase == ShopPhase::Quantity && shopSelling);
    if (selling)
    {
        for (const auto& [id, count] : player.getItemCounts())
            if (count > 0) out.emplace_back(id, shopPrice(game.getItem(id), true));
    }
    else
        for (int id : shop->goods) out.emplace_back(id, shopPrice(game.getItem(id), false));
    return out;
}

int UIManager::shopMaxCount() const
{
    const auto list = shopList();
    if (shopSel >= static_cast<int>(list.size())) return 0;
    const auto [id, price] = list[shopSel];
    const auto& counts = player.getItemCounts();
    const auto it = counts.find(id);
    const int owned = it != counts.end() ? it->second : 0;
    if (shopSelling) return owned;
    const int room = kMaxCarry - owned;
    return price > 0 ? std::min<int>(room, static_cast<int>(player.getPlayerGold() / price)) : room;
}

SDL_Rect UIManager::shopCommandRect(int row) const
{
    return {kShopCmds.x + 16, kShopCmds.y + 12 + row * kShopRowH, kShopCmds.w - 32, kShopRowH - 4};
}

SDL_Rect UIManager::shopRowRect(int row) const
{
    return {kShopList.x + 16, kShopList.y + 12 + row * kShopRowH, kShopList.w - 32, kShopRowH - 4};
}

std::string UIManager::learnBlock(const Hero& hero, const Skill& spell) const
{
    if (!spell.learnableBy(hero.getClass())) return "Can't learn";
    if (hero.knowsSkill(spell.id))           return "Knows it";
    if (hero.spellsKnown(spell.spellLevel, spell.white) >= kSpellsPerLevel) return "Level full";
    return "";
}

void UIManager::shopPick(int index)
{
    const auto list = shopList();
    if (index < 0 || index >= static_cast<int>(list.size())) return;
    shopSel = index;
    const int price = list[index].second;

    if (shop->kind == ShopKind::Church)                              // воскрешаем выбранного павшего
    {
        if (player.getPlayerGold() < static_cast<uint32_t>(price)) { shopSay(tr("You don't have enough gil.")); return; }
        Hero& hero = player.getLinkTOHeroes()[list[index].first];
        player.setPlayerGold(player.getPlayerGold() - price);
        hero.resurrectPerson(1);                                     // как в FF1: встаёт с 1 HP
        shopSay(trf("{0} has returned to life!", {hero.getName()}));
        game.sendPartyInfo();
        if (shopList().empty()) shopPhase = ShopPhase::Command;
        else shopSel = std::min(shopSel, static_cast<int>(shopList().size()) - 1);
        return;
    }

    if (shop->sellsSpells())                                         // дальше — кто выучит
    {
        if (player.getPlayerGold() < static_cast<uint32_t>(price)) { shopSay(tr("You don't have enough gil.")); return; }
        partySel = 0;
        shopPhase = ShopPhase::LearnWho;
        shopSay(trf("Who will learn {0}?", {game.getSkill(list[index].first).name}));
        return;
    }

    shopSelling = shopPhase == ShopPhase::Sell;
    const Item& item = game.getItem(list[index].first);
    if (shopSelling && price <= 0) { shopSay(tr("I have no use for that.")); return; }
    if (!shopSelling && player.getPlayerGold() < static_cast<uint32_t>(price)) { shopSay(tr("You don't have enough gil.")); return; }
    if (shopMaxCount() <= 0) { shopSay(tr("You can't carry any more.")); return; }

    shopCount = 1;
    shopPhase = ShopPhase::Quantity;
    shopSay(shopSelling ? tr("How many will you sell?") : trf("{0}? How many?", {item.name}));
}

void UIManager::learnSpell(int heroIndex)
{
    const auto list = shopList();
    auto& heroes = player.getLinkTOHeroes();
    if (shopSel >= static_cast<int>(list.size()) || heroIndex < 0 || heroIndex >= static_cast<int>(heroes.size())) return;
    const Skill& spell = game.getSkill(list[shopSel].first);
    Hero& hero = heroes[heroIndex];
    if (const std::string why = learnBlock(hero, spell); !why.empty())
    {
        shopSay(trf(why == "Can't learn" ? "{0} can't learn that." : why == "Knows it" ? "{0} already knows it."
                                                                    : "{0} can't learn more spells of this level.",
                    {hero.getName()}));
        return;
    }
    player.setPlayerGold(player.getPlayerGold() - list[shopSel].second);
    hero.learnSkill(spell.id);
    game.sendPartyInfo();                                            // напарник и хост узнают о новом заклинании
    shopPhase = ShopPhase::Buy;
    shopSay(trf("{0} learned {1}!", {hero.getName(), spell.name}));
}

void UIManager::restAtInn()
{
    // Затемнение, ночь, утро: живые встают с полными HP и MP. Павших поднимает только церковь
    game.startTransition([this]() {
        for (auto& h : player.getLinkTOHeroes())
            if (h.getIsAlive())
            {
                h.setCurrentHp(h.getMaxHp());
                h.setCurrentMp(h.getMaxMp());
            }
        game.sendPartyInfo();
    });
    shopPhase = ShopPhase::InnSave;
    shopCommand = 0;
    shopSay(tr("Good morning! Would you like to save?"));
}

void UIManager::shopRunCommand(int index)
{
    const auto commands = shopCommands();
    if (index < 0 || index >= static_cast<int>(commands.size())) return;
    const std::string cmd = commands[index];
    if (cmd == "Exit") { game.closeShop(); return; }
    if (cmd == "Stay")
    {
        if (player.getPlayerGold() < static_cast<uint32_t>(shop->price)) { shopSay(tr("You don't have enough gil.")); return; }
        player.setPlayerGold(player.getPlayerGold() - shop->price);
        restAtInn();
        return;
    }
    if (cmd == "Save") { game.openSaveScreen(true); return; }
    if (cmd == "Revive")
    {
        shopSel = shopScroll = 0;
        if (shopList().empty()) { shopSay(tr("Your party is in fine health.")); return; }
        shopPhase = ShopPhase::Buy;
        shopSay(tr("Whom shall I revive?"));
        return;
    }
    if (cmd == "Buy")
    {
        shopPhase = ShopPhase::Buy;
        shopSel = shopScroll = 0;
        shopSay(tr(shop->sellsSpells() ? "Which spell would you like?" : "What would you like to buy?"));
        return;
    }
    shopPhase = ShopPhase::Sell;                                     // "Sell"
    shopSel = shopScroll = 0;
    if (shopList().empty()) { shopPhase = ShopPhase::Command; shopSay(tr("You have nothing to sell.")); }
    else shopSay(tr("What will you sell?"));
}

void UIManager::shopDeal()
{
    const auto list = shopList();
    if (shopSel >= static_cast<int>(list.size())) { shopPhase = ShopPhase::Command; return; }
    const auto [id, price] = list[shopSel];
    const uint32_t total = static_cast<uint32_t>(price) * shopCount;
    if (shopSelling)
    {
        player.removeItem(id, static_cast<uint8_t>(shopCount));
        player.setPlayerGold(player.getPlayerGold() + total);
        shopPhase = ShopPhase::Sell;
        const int left = static_cast<int>(shopList().size());
        if (left == 0)
        {
            shopPhase = ShopPhase::Command;
            shopSay(tr("Thank you! Anything else?"));
            return;
        }
        shopSel = std::min(shopSel, left - 1);
        shopScroll = std::clamp(shopScroll, 0, std::max(0, left - kShopRows));
    }
    else
    {
        player.setPlayerGold(player.getPlayerGold() - total);
        player.addItem(id, static_cast<uint8_t>(shopCount));
        shopPhase = ShopPhase::Buy;
    }
    shopSay(tr("Thank you! Anything else?"));
}

void UIManager::handleShopEvent(const SDL_Event& e)
{
    if (!shop) { game.closeShop(); return; }
    if (game.getFadeAlpha() > 0.0f) return;                         // ночь в гостинице: ждём утра

    if (shopPhase == ShopPhase::Command || shopPhase == ShopPhase::InnSave)
    {
        const int n = static_cast<int>(shopCommands().size());
        if (backKey(e)) { game.closeShop(); return; }
        if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))   shopCommand = (shopCommand + n - 1) % n;
        if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S})) shopCommand = (shopCommand + 1) % n;
        int activate = confirmKey(e) ? shopCommand : -1;
        if (isMouse(e))
        {
            const SDL_Point m = mousePos(e);
            for (int r = 0; r < n; ++r)
            {
                const SDL_Rect rr = shopCommandRect(r);
                if (!SDL_PointInRect(&m, &rr)) continue;
                shopCommand = r;
                if (leftClick(e)) activate = r;
            }
        }
        if (activate >= 0) shopRunCommand(activate);
        return;
    }

    if (shopPhase == ShopPhase::LearnWho)                            // герои — четыре колонки внизу
    {
        const int heroes = static_cast<int>(player.getHeroes().size());
        if (backKey(e))
        {
            shopPhase = ShopPhase::Buy;
            shopSay(tr("Which spell would you like?"));
            return;
        }
        if (heroes == 0) return;
        if (keyIs(e, {SDL_SCANCODE_LEFT, SDL_SCANCODE_A, SDL_SCANCODE_UP, SDL_SCANCODE_W}))
            partySel = (partySel + heroes - 1) % heroes;
        if (keyIs(e, {SDL_SCANCODE_RIGHT, SDL_SCANCODE_D, SDL_SCANCODE_DOWN, SDL_SCANCODE_S}))
            partySel = (partySel + 1) % heroes;
        int pick = confirmKey(e) ? partySel : -1;
        if (isMouse(e))
        {
            const SDL_Point m = mousePos(e);
            const int colW = (kShopInfo.w - 40) / 4;
            for (int i = 0; i < heroes && i < 4; ++i)
            {
                const SDL_Rect r{kShopInfo.x + 20 + i * colW, kShopInfo.y + 48, colW, 40};
                if (!SDL_PointInRect(&m, &r)) continue;
                partySel = i;
                if (leftClick(e)) pick = i;
            }
        }
        if (pick >= 0) learnSpell(pick);
        return;
    }

    if (shopPhase == ShopPhase::Quantity)
    {
        const int max = std::max(1, shopMaxCount());
        if (backKey(e))
        {
            shopPhase = shopSelling ? ShopPhase::Sell : ShopPhase::Buy;
            shopSay(tr(shopSelling ? "What will you sell?" : "What would you like to buy?"));
            return;
        }
        if (keyIs(e, {SDL_SCANCODE_RIGHT, SDL_SCANCODE_D})) shopCount = shopCount % max + 1;         // по кругу, как в ремастере
        if (keyIs(e, {SDL_SCANCODE_LEFT, SDL_SCANCODE_A}))  shopCount = (shopCount + max - 2) % max + 1;
        if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))    shopCount = std::min(max, shopCount + 10);
        if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S}))  shopCount = std::max(1, shopCount - 10);
        if (e.type == SDL_MOUSEWHEEL) shopCount = std::clamp(shopCount + e.wheel.y, 1, max);
        if (confirmKey(e) || leftClick(e)) shopDeal();
        return;
    }

    // Список товаров, своих вещей, заклинаний или павших
    const int count = static_cast<int>(shopList().size());
    if (backKey(e) || count == 0)
    {
        if (count == 0 && !backKey(e)) return;
        shopPhase = ShopPhase::Command;
        shopSay(tr("Anything else?"));
        return;
    }
    if (keyIs(e, {SDL_SCANCODE_UP, SDL_SCANCODE_W}))   shopSel = (shopSel - 1 + count) % count;
    if (keyIs(e, {SDL_SCANCODE_DOWN, SDL_SCANCODE_S})) shopSel = (shopSel + 1) % count;
    if (e.type == SDL_MOUSEWHEEL)
        shopScroll = std::clamp(shopScroll - e.wheel.y, 0, std::max(0, count - kShopRows));
    else
    {
        if (shopSel < shopScroll) shopScroll = shopSel;                 // прокрутка за курсором
        if (shopSel >= shopScroll + kShopRows) shopScroll = shopSel - kShopRows + 1;
    }
    int pick = confirmKey(e) ? shopSel : -1;
    if (isMouse(e))
    {
        const SDL_Point m = mousePos(e);
        for (int r = 0; r < kShopRows && shopScroll + r < count; ++r)
        {
            const SDL_Rect rr = shopRowRect(r);
            if (!SDL_PointInRect(&m, &rr)) continue;
            shopSel = shopScroll + r;
            if (leftClick(e)) pick = shopSel;
        }
    }
    if (pick >= 0) shopPick(pick);
}

void UIManager::drawShopInfo(const Item& item)
{
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    const auto& counts = player.getItemCounts();
    const auto it = counts.find(item.id);
    const std::string owned = trf("Owned: {0}", {std::to_string(it != counts.end() ? it->second : 0)});
    drawText(owned, kShopInfo.x + kShopInfo.w - 20 - ResourceManager::measureText(owned, font).w, kShopInfo.y + 14, kDim);

    if (!item.isEquipment())
    {
        drawText(item.name, kShopInfo.x + 20, kShopInfo.y + 14, kText);
        drawText(itemDescription(item), kShopInfo.x + 20, kShopInfo.y + 54, kDim);
        return;
    }

    // Экипировка: кто из отряда наденет и насколько изменится главный стат (атака у оружия, защита у прочего)
    drawText(itemDescription(item), kShopInfo.x + 20, kShopInfo.y + 14, kDim);
    const auto& heroes = player.getHeroes();
    const int colW = (kShopInfo.w - 40) / 4;
    for (int i = 0; i < static_cast<int>(heroes.size()) && i < 4; ++i)
    {
        const Hero& h = heroes[i];
        const int x = kShopInfo.x + 20 + i * colW, y = kShopInfo.y + 54;
        if (!item.canEquip(h.getClass()))
        {
            drawText(h.getName() + " —", x, y, kDim);
            continue;
        }
        const Item* worn = game.findItem(h.getEquipped(item.slot));
        const bool weapon = item.slot == EquipSlot::Weapon;
        const int now = worn ? (weapon ? worn->attack : worn->defence) : 0;
        const int diff = (weapon ? item.attack : item.defence) - now;
        const int w = drawText(h.getName() + " ", x, y, kText);
        drawText(diff > 0 ? "+" + std::to_string(diff) : diff < 0 ? std::to_string(diff) : "=",
                 x + w, y, diff > 0 ? kGood : diff < 0 ? kBad : kDim);
    }
}

void UIManager::drawSpellInfo(const Skill& spell)
{
    // Сверху: круг, MP и что делает; снизу — кто из отряда может выучить (при выборе героя — со стрелкой)
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    const std::string head = trf("Level {0}   {1} MP", {std::to_string(spell.spellLevel), std::to_string(spell.manaCost)});
    drawText(head, kShopInfo.x + kShopInfo.w - 20 - ResourceManager::measureText(head, font).w, kShopInfo.y + 14, kDim);
    drawText(skillDescription(spell), kShopInfo.x + 20, kShopInfo.y + 14, kDim);

    const auto& heroes = player.getHeroes();
    const int colW = (kShopInfo.w - 40) / 4;
    const int blink = static_cast<int>((SDL_GetTicks() / 220) % 2) * 2;
    for (int i = 0; i < static_cast<int>(heroes.size()) && i < 4; ++i)
    {
        const Hero& h = heroes[i];
        const int x = kShopInfo.x + 20 + i * colW, y = kShopInfo.y + 54;
        const std::string why = learnBlock(h, spell);
        const SDL_Color c = why.empty() ? (shopPhase == ShopPhase::LearnWho && i == partySel ? kHot : kText) : kDim;
        const int w = drawText(h.getName() + " ", x + 16, y, c);
        if (!why.empty()) drawText(why == "Can't learn" ? "—" : why == "Knows it" ? tr("knows") : "3/3", x + 16 + w, y, kDim);
        if (shopPhase == ShopPhase::LearnWho && i == partySel) drawPointer(x + 10 - blink, y + 14, 2);
    }
}

void UIManager::drawShop()
{
    if (!shop) return;
    SDL_Texture* skin = getWindowSkin(visualizer.getRenderer());
    TTF_Font* font = ResourceManager::getFont(kUiFont);
    visualizer.fillRect(screen, {0, 0, 0, 120});                   // город виден, но приглушён
    auto window = [&](const SDL_Rect& r) { visualizer.drawNinePatch(skin, r, kWindowSkinSlice, 1); };
    const int blink = static_cast<int>((SDL_GetTicks() / 220) % 2) * 2;

    // Название и золото
    window(kShopHeader);
    drawText(tr(shop->title), kShopHeader.x + 20, kShopHeader.y + 14, kText);
    const std::string money = gil(static_cast<int>(player.getPlayerGold()));
    drawText(money, kShopHeader.x + kShopHeader.w - 20 - ResourceManager::measureText(money, font).w,
             kShopHeader.y + 14, kHot);

    // Реплика продавца
    window(kShopTalk);
    drawText(shopLine, kShopTalk.x + 20, kShopTalk.y + 14, kText);

    // Команды
    const auto commands = shopCommands();
    const bool onCommands = shopPhase == ShopPhase::Command || shopPhase == ShopPhase::InnSave;
    window({kShopCmds.x, kShopCmds.y, kShopCmds.w, 24 + static_cast<int>(commands.size()) * kShopRowH});
    for (int r = 0; r < static_cast<int>(commands.size()); ++r)
    {
        const SDL_Rect rr = shopCommandRect(r);
        const bool sel = r == shopCommand;
        drawText(tr(commands[r]), rr.x + 22, rr.y + 6, sel && onCommands ? kHot : (onCommands ? kText : kDim));
        if (sel && onCommands) drawPointer(rr.x + 16 - blink, rr.y + rr.h / 2, 2);
    }

    // Список: товары (на главной — приглушённо), свои вещи на продажу, заклинания или павшие герои
    window(kShopList);
    const auto list = shopList();
    const bool picking = shopPhase == ShopPhase::Buy || shopPhase == ShopPhase::Sell;
    const bool selling = shopPhase == ShopPhase::Sell || (shopPhase == ShopPhase::Quantity && shopSelling);
    const bool church = shop->kind == ShopKind::Church;
    if (shop->kind == ShopKind::Inn)
    {
        const std::string price = gil(shop->price);
        drawText(tr("One night"), shopRowRect(0).x + 22, shopRowRect(0).y + 6, kText);
        drawText(price, shopRowRect(0).x + shopRowRect(0).w - ResourceManager::measureText(price, font).w,
                 shopRowRect(0).y + 6, kText);
    }
    if (church && list.empty())
        drawText(tr("Everyone is alive and well."), shopRowRect(0).x + 22, shopRowRect(0).y + 6, kDim);
    for (int r = 0; r < kShopRows && shopScroll + r < static_cast<int>(list.size()); ++r)
    {
        const int i = shopScroll + r;
        const SDL_Rect rr = shopRowRect(r);
        const bool sel = i == shopSel && !onCommands;
        const bool affordable = selling || player.getPlayerGold() >= static_cast<uint32_t>(list[i].second);
        const SDL_Color c = onCommands ? kDim : !affordable ? kDim : sel ? kHot : kText;
        const std::string name = church ? player.getHeroes()[list[i].first].getName()
                               : shop->sellsSpells() ? game.getSkill(list[i].first).name
                               : game.getItem(list[i].first).name;
        drawText(name, rr.x + 22, rr.y + 6, c);
        std::string right = gil(list[i].second);
        if (selling)
        {
            const auto it = player.getItemCounts().find(list[i].first);
            right = "x" + std::to_string(it != player.getItemCounts().end() ? it->second : 0) + "   " + right;
        }
        drawText(right, rr.x + rr.w - ResourceManager::measureText(right, font).w, rr.y + 6, c);
        if (sel && picking) drawPointer(rr.x + 16 - blink, rr.y + rr.h / 2, 2);
    }
    if (list.size() > static_cast<size_t>(kShopRows))                // есть ещё: полоска прокрутки справа
    {
        const int trackH = kShopRows * kShopRowH;
        const int barH = std::max(16, trackH * kShopRows / static_cast<int>(list.size()));
        const int barY = (trackH - barH) * shopScroll / std::max(1, static_cast<int>(list.size()) - kShopRows);
        visualizer.fillRect({kShopList.x + kShopList.w - 10, kShopList.y + 12 + barY, 4, barH}, kDim);
    }

    // Описание выбранного
    window(kShopInfo);
    if (!onCommands && shopSel < static_cast<int>(list.size()) && !church)
    {
        if (shop->sellsSpells()) drawSpellInfo(game.getSkill(list[shopSel].first));
        else                     drawShopInfo(game.getItem(list[shopSel].first));
    }

    // Сколько штук
    if (shopPhase == ShopPhase::Quantity && shopSel < static_cast<int>(list.size()))
    {
        window(kShopCount);
        drawText(tr("Quantity"), kShopCount.x + 20, kShopCount.y + 14, kDim);
        const std::string n = std::to_string(shopCount);
        const int nw = ResourceManager::measureText(n, font).w;
        const int cx = kShopCount.x + kShopCount.w / 2;
        drawText("<", cx - 60, kShopCount.y + 50, kHot);
        drawText(n, cx - nw / 2, kShopCount.y + 50, kText);
        drawText(">", cx + 50, kShopCount.y + 50, kHot);
        const std::string total = trf("Total: {0}", {gil(list[shopSel].second * shopCount)});
        drawText(total, kShopCount.x + kShopCount.w - 20 - ResourceManager::measureText(total, font).w,
                 kShopCount.y + 90, kText);
    }

    if (const float a = game.getFadeAlpha(); a > 0.0f)               // ночь в гостинице
        visualizer.fillRect(screen, {0, 0, 0, static_cast<Uint8>(255 * a)});
}
