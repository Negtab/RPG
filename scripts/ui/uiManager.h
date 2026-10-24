#ifndef PROJECT_NAME_UIMANAGER_H
#define PROJECT_NAME_UIMANAGER_H

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "SDL_events.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_ttf.h"

#include "../controllers/visualizer.h"
#include "../controllers/resourceManager.h"
#include "core/Scene.h"
#include "animation/animPlayer.h"
#include "../main/mapRenderer.h"
#include "../gameLogic/gameData.h"



class Game;
class Player;
class NotGamePerson;
class UIObject;

struct MenuListLayout { SDL_Rect area; int cols; int rows; int rowH; };   // список в две колонки с прокруткой

class UIManager {
public:
    using AnimPlayer = ::AnimPlayer;

    explicit UIManager(Visualizer& visualizer, AudioManager& audioManager, ResourceManager& resourceManager, Game& game, Player& player)
        : visualizer(visualizer), audioManager(audioManager), resourceManager(resourceManager),
          game(game), player(player), screen({}) {}

    // ── Сцены ──────────────────────────────────────────
    void initialize();
    void update(float dt);
    void drawScene(const std::string& sceneId);

    void handleClickEvent(const SDL_Event& event, const std::string& sceneId);
    void handleHoverEvent(const SDL_Event& event, const std::string& sceneId);

    // ── Добавление виджетов ────────────────────────────
    void addScene(const std::string& id);
    void addPanel(const std::string& id, const std::string& sceneId,
                  const std::string& panelId = "");
    void addButton(const std::string& id, const std::string& sceneId,
                   const SDL_Rect& rect, SDL_Texture* texture,
                   std::function<void()> onClick, std::function<void()> onHover,
                   const std::string& panelId = "");
    void addEdit(const std::string& id, const std::string& sceneId,
                 const SDL_Rect& rect, TTF_Font* font, SDL_Color color,
                 std::function<void()> onClick, std::function<void()> onHover,
                 const std::string& panelId = "");
    void addImage(const std::string& id, const std::string& sceneId,
                  const SDL_Rect& rect, SDL_Texture* texture,
                  SDL_Rect srect = {0,0,0,0}, const std::string& panelId = "");
    void addLabel(const std::string& id, const std::string& sceneId,
                  const SDL_Rect& bgRect, SDL_Texture* bgTexture,
                  const std::string& text, TTF_Font* font,
                  SDL_Color color = {0,0,0,255},
                  int paddingX = 8, int paddingY = 4,
                  const std::string& panelId = "");
    void addWindow(const std::string& id, const std::string& sceneId,
                   const SDL_Rect& rect, const std::string& panelId = "");

    // ── Аудио ──────────────────────────────────────────
    void addMusic(const std::string& musicId, const std::string& sceneId);
    void addSound(const std::string& soundId, const std::string& sceneId);
    void playSound(const std::string& sceneId, const std::string& soundId);

    // ── Анимация ───────────────────────────────────────
    void addAnimation(const std::string& id, const std::string& sceneId,
                      std::vector<SDL_Texture*> frames, std::vector<SDL_Rect> rects,
                      float frameTime = -1.0f,
                      AnimPlayMode mode = AnimPlayMode::OneShot);
    void addAnimation(const std::string& id, const std::string& sceneId,
                      std::vector<SDL_Texture*> frames, SDL_Rect rect,
                      float frameTime = -1.0f,
                      AnimPlayMode mode = AnimPlayMode::Loop);
    void playAnimation(const std::string& animId, const std::string& sceneId,
                       bool restart);
    [[nodiscard]] bool isAnimationFinished(const std::string& sceneId) const;

    void updateRemoteAnimation(AnimPlayer& animPlayer, const Player& player);

    // ── Setters ────────────────────────────────────────
    void setVisible (const std::string& id, const std::string& sceneId, bool visible);
    void setEnabled (const std::string& id, const std::string& sceneId, bool enabled);
    void setEnVI    (const std::string& id, const std::string& sceneId, bool value);
    void setTexture (const std::string& id, const std::string& sceneId, SDL_Texture* texture);
    void setRect    (const std::string& id, const std::string& sceneId, const SDL_Rect& rect);
    void setSrcRect (const std::string& id, const std::string& sceneId, const SDL_Rect& srect);
    void setOnClick (const std::string& id, const std::string& sceneId,
                     std::function<void()> onClick);
    void setText    (const std::string& id, const std::string& sceneId,
                     const std::string& text);
    void setFocus   (const std::string& id, const std::string& sceneId, bool focus);
    void setPos     (const std::string& id, const std::string& sceneId, int pos);

    // ── Getters ────────────────────────────────────────
    [[nodiscard]] std::string  getText  (const std::string& id, const std::string& sceneId);
    [[nodiscard]] bool         getFocus (const std::string& id, const std::string& sceneId);
    [[nodiscard]] int          getPos   (const std::string& id, const std::string& sceneId);
    [[nodiscard]] SDL_Rect     getRect  (const std::string& id, const std::string& sceneId);

    // ── Battle ─────────────────────────────────────────
    void addEnemys();
    void addCharacters();

    void moveSelectorToNext();
    void moveSelectorToPrevious();
    void moveSelector();
    void confirmTarget();
    void cancelChoice();                       // Esc: шаг назад (цель -> список -> команды)

    void setState(const UIChooseState& state);
    [[nodiscard]] UIChooseState getState() const;

    void showBattleResults(const BattleReport& r);
    void setBattleBackground(const std::string& name);         // BB<name>.png; нет такого — прежний
    void enemyLunge(int index);                                // враг шагает к героям
    void showActionName(const std::string& text);

    // ── Боевые эффекты (effects.json) ───────────────────
    void playEffect(const std::string& id, SDL_Point center, float delay = 0.0f);
    void playEffectOn(const std::string& id, bool onHero, uint32_t owner, int index, float delay = 0.0f);
    void playSwing(int swing, const std::string& heroImageId, float delay = 0.0f);   // замах SwingNN у руки героя
    [[nodiscard]] static std::string effectForSkill(const Skill& skill);
    [[nodiscard]] static std::string effectForItem(const Item& item);
    void hideBattleResults();

    void resetPartyCreation();

    // ── Экран сохранений ───────────────────────────────
    void openSaveScreen(bool saving);
    void openGameOver();
    void handleGameOverEvent(const SDL_Event& event);
    void openIntro();
    void handleIntroEvent(const SDL_Event& event);
    void handleSavesEvent(const SDL_Event& event);

    // ── Игровое меню и настройки ───────────────────────
    void openGameMenu();
    void showMapNotice(const std::string& text);
    void handleGameMenuEvent(const SDL_Event& event);
    void handleConfigEvent(const SDL_Event& event);

    // ── Бестиарий: из игрового меню и из боя ────────────
    void openBestiary();
    void openBestiaryFromBattle();                             // только пока выбираем команду (кнопка Book или B)
    void closeBestiary();
    [[nodiscard]] bool isBestiaryOpen() const { return bestiaryOpen; }
    void handleBestiaryEvent(const SDL_Event& event);

    void applyLanguage();                                   // язык сменили: пересоздать надписи-картинки

    // ── Разговор с NPC и магазин (shopScreen.cpp) ──
    void showDialog(const std::string& speaker, const std::vector<std::string>& pages);
    [[nodiscard]] bool isDialogOpen() const { return !talkPages.empty(); }
    void handleDialogEvent(const SDL_Event& event);
    void showChoice(const std::string& speaker, const std::string& question);   // вопрос и «Да / Нет»
    [[nodiscard]] std::optional<bool> takeChoice();          // ответ, когда игрок выбрал (один раз)
    void openShop(const ShopDef& shop);
    void handleShopEvent(const SDL_Event& event);

    bool handleTextEvent(const SDL_Event& event, const std::string& sceneId);   // true: событие поглотило поле ввода
    void styleEdit(const std::string& id, const std::string& sceneId,
                   SDL_Color background, SDL_Color border, int maxLength = 32);

private:
    // ── Поиск ─────────────────────────────────────────
    Scene*     findScene    (const std::string& id);
    UIObject*  findUIObject (const std::string& id, const std::string& sceneId);

    // ── Отрисовка ──────────────────────────────────────
    void drawMapWorld();
    void drawActor(const Player& p, int cellScreenX, int cellScreenY);
    bool drawVehicle(Vehicle v, Direction dir, int cellScreenX, int cellScreenY, bool flying, bool moving);   // false: нет спрайта
    MapRenderer* rendererFor(const MapData& map, bool roomView = false);   // roomView — вид изнутри комнат
    std::unordered_map<int, std::unique_ptr<MapRenderer>> mapRenderers;

    // ── Обновление данных ──────────────────────────────
    void updateBattleData(Scene& scene);
    void updateSceneData(const std::string &sceneId, Scene &scene);

    // ── Инициализация сцен ─────────────────────────────
    void initMapScene    ();
    void initMenuScene   ();
    void initOptionsScene();
    void initOnlineScene ();
    void initBattleScene ();

    // ── Battle logic ───────────────────────────────────
    void onActionButton    (ActionType type);
    void onEnvironmentCLick(int id, bool isMagic);

    void initMagic();
    void initItems();
    void closeList();
    void setListEntry(int i, const std::string& name, const std::string& info,
                      bool usable, std::function<void()> onClick);
    [[nodiscard]] bool hasUsableSkill() const;
    [[nodiscard]] bool hasItems() const;

    void snapSelectorToTarget();
    [[nodiscard]] bool heroTargetable(int i) const;          // живой, а для Жизни — павший
    bool targetFallen = false;
    [[nodiscard]] int getTargetCount(TargetType type) const;
    [[nodiscard]] SDL_Rect selectedTargetRect() const;

    // ── Поверх сцены боя: указатели, имя цели, всплывающие числа ──
    struct Popup { int x, y; std::string text; SDL_Color color; Uint32 start; };   // start может быть в будущем
    struct FxInstance { EffectLayer layer; SDL_Texture* tex; SDL_Point at; Uint32 start; };
    std::vector<FxInstance> fx;
    void drawEffects();
    [[nodiscard]] SDL_Rect battlerRect(bool onHero, uint32_t owner, int index) const;
    [[nodiscard]] const FxInstance* recentEffectAt(SDL_Point p) const;   // эффект, только что запущенный здесь
    void drawBattleOverlay();
    void drawPointer(int x, int y, int scale) const;          // белая стрелка вправо, (x, y) — её острие
    void drawOutlinedText(const std::string& text, int x, int y, SDL_Color color, Uint8 alpha) const;
    Uint32 trackHp(int& last, int hp, const SDL_Rect& slot);   // момент попадания, если HP упало; иначе 0
    void resetHpTracking();
    std::vector<Popup> popups;
    std::array<int, MAX_ENEMY_COUNT> lastEnemyHp{};
    std::array<int, 4> lastHeroHp{}, lastRemoteHp{};
    std::string actionBanner;                                  // «Warrior - Attack» сверху во время хода
    Uint32 actionBannerAt = 0;
    std::array<SDL_Rect, 8> listRect{};                        // строки списка: для стрелки под мышью

    void initCreateScene();
    void refreshPartySlot(int slot);
    void cycleClass(int slot, int dir);
    void focusNameField(int slot);
    void confirmParty();

    void layoutBattle();

    // ── Экран сохранений ───────────────────────────────
    void initSavesScene();
    void initGameMenuScene();
    void updateMenuButtons();
    void drawSavesScreen();
    void drawGameOver();
    void drawIntro();
    Uint32 gameOverStart = 0;                                  // когда показали Game Over
    Uint32 introStart = 0;                                     // когда началась заставка
    int  drawText(const std::string& text, int x, int y, SDL_Color color) const;    // возвращает ширину
    [[nodiscard]] SDL_Rect slotCardRect(int row) const;
    [[nodiscard]] SDL_Rect dialogOptionRect(bool yes) const;
    [[nodiscard]] static std::string slotName(int slot);
    void activateSlot(int slot);
    void answerDialog(bool yes);
    void showSavesNotice(const std::string& text);

    bool savesSaving = false;
    std::vector<int> savesRows;                                // какие слоты на экране, сверху вниз
    int savesSelected = 0;                                     // строка в savesRows
    std::array<std::optional<SaveSummary>, 5> slotCache;       // читаем файлы при открытии экрана, а не каждый кадр
    int  dialogSlot = -1;                                      // >= 0: открыт вопрос «точно?» про этот слот
    bool dialogYes = false;
    std::string dialogLine1, dialogLine2;
    std::string savesNotice;
    Uint32 savesNoticeTicks = 0;

    // ── Игровое меню и настройки ───────────────────────
    struct MenuCommand { std::string text; bool enabled; std::function<void()> action; };
    [[nodiscard]] std::vector<MenuCommand> gameMenuCommands();
    [[nodiscard]] SDL_Rect gameMenuRowRect(int row) const;
    void drawGameMenu();

    // Страницы игрового меню. *Target — выбор героя, на которого применить предмет или заклинание
    enum class MenuPage { Main, PickHero, Items, ItemTarget, Magic, MagicTarget, Equip, EquipPick, Status, Journal };
    void drawJournalPage();
    using ListLayout = MenuListLayout;

    void openMenuPage(MenuPage page);
    void showMenuNotice(const std::string& text);
    [[nodiscard]] SDL_Rect partyRowRect(int i) const;
    [[nodiscard]] SDL_Rect targetRowRect(int i) const;
    [[nodiscard]] static SDL_Rect listCellRect(const ListLayout& l, int scroll, int index);
    static void listNavigate(const ListLayout& l, int& sel, int& scroll, int count, SDL_Scancode k);
    [[nodiscard]] static int listHit(const ListLayout& l, int scroll, int count, SDL_Point p);
    [[nodiscard]] std::vector<std::pair<int, int>> inventoryList() const;   // (id, количество) для Items
    [[nodiscard]] std::vector<int> equipCandidates() const;                // для слота equipSlotSel; 0 — «снять»
    [[nodiscard]] std::string itemDescription(const Item& item) const;
    [[nodiscard]] std::string skillDescription(const Skill& skill) const;
    [[nodiscard]] bool skillUsableInField(const Hero& caster, const Skill& skill) const;
    bool applyFieldEffect(Hero& target, bool restoresMp, int power, bool revive = false);   // false: эффекта нет
    void useItemOn(int heroIndex);
    void castOn(int heroIndex);
    void cycleMenuHero(int dir);

    void handleMenuMain(const SDL_Event& e);
    void handleMenuList(const SDL_Event& e);                                // Items и Magic
    void handleMenuTarget(const SDL_Event& e);
    void handleMenuEquip(const SDL_Event& e);
    void drawMenuHeader(const std::string& title, const std::string& right);
    void drawPartyPanel(const SDL_Rect& win, int pointerRow, bool compact);
    void drawItemsPage();
    void drawMagicPage();
    void drawEquipPage();
    void drawStatusPage();
    void drawTargetOverlay();

    MenuPage menuPage = MenuPage::Main;
    MenuPage pickFor  = MenuPage::Status;                       // куда ведёт выбор героя на главной странице
    int menuHero = 0;                                           // чей экран Magic, Equipment, Status
    int partySel = 0;                                           // строка отряда при выборе героя или цели
    int listSel = 0, listScroll = 0;
    int equipSlotSel = 0;
    std::string menuNotice;
    Uint32 menuNoticeTicks = 0;
    // ── Бестиарий (bestiary.cpp) ────────────────────────
    void drawBestiary();
    void drawBestiaryLeft(int index, const SDL_Rect& page);    // номер, имя, спрайт или силуэт
    void drawBestiaryRight(int index, const SDL_Rect& page);   // слабости, сильные стороны, места, умения
    void drawBestiaryCorner(const SDL_Rect& r, bool right, bool enabled);
    int  drawWrapped(const std::string& text, int x, int y, int width, int maxLines, SDL_Color color) const;   // y под текстом
    void turnBestiaryPage(int index);
    void runBestiarySearch(bool next);                         // next: к следующему совпадению после текущей страницы
    void setBestiarySearch(bool on);
    bool bestiaryOpen = false;
    int  bestiaryIndex = 0;                                    // страница, на которой остановились (не сбрасывается)
    int  bestiaryTurnFrom = 0;
    Uint32 bestiaryTurnAt = 0;                                 // 0 — страница не листается
    bool bestiarySearching = false;
    std::string bestiaryQuery;
    int  bestiaryMatches = 0, bestiaryMatchPos = 0;           // для подписи «2/5»

    // ── Жители на карте, окно разговора, магазин (shopScreen.cpp) ──
    void drawNpc(const NotGamePerson& npc, int cellScreenX, int cellScreenY);
    void drawDialog();
    [[nodiscard]] std::vector<std::string> wrapText(const std::string& text, int width) const;
    std::string talkSpeaker;
    std::vector<std::string> talkPages;                        // пусто — окна нет
    bool talkChoice = false;                                   // последняя страница — вопрос «Да / Нет»
    int choiceSel = 0;                                         // 0 — «Да»
    std::optional<bool> choiceResult;
    [[nodiscard]] SDL_Rect choiceRect(int row) const;
    int talkPage = 0;
    Uint32 talkStart = 0;                                      // когда страница начала печататься

    enum class ShopPhase { Command, Buy, Sell, Quantity, LearnWho, InnSave };
    void drawShop();
    void drawShopInfo(const Item& item);
    [[nodiscard]] std::vector<std::pair<int, int>> shopList() const;   // (id, цена): товары или что можно продать
    [[nodiscard]] int shopPrice(const Item& item, bool selling) const;
    [[nodiscard]] int shopMaxCount() const;                    // сколько можно купить или продать выбранного
    [[nodiscard]] SDL_Rect shopCommandRect(int row) const;
    [[nodiscard]] SDL_Rect shopRowRect(int row) const;
    void shopPick(int index);                                  // выбран товар: к выбору количества
    void shopDeal();                                           // количество подтверждено: купить или продать
    void shopSay(const std::string& line) { shopLine = line; }
    [[nodiscard]] std::vector<const char*> shopCommands() const;  // «Купить / Продать / Выйти» или услуги
    void shopRunCommand(int index);                              // выбрана команда
    [[nodiscard]] std::string learnBlock(const Hero& hero, const Skill& spell) const;   // "" — может выучить
    void drawSpellInfo(const Skill& spell);
    void learnSpell(int heroIndex);
    void restAtInn();
    const ShopDef* shop = nullptr;
    ShopPhase shopPhase = ShopPhase::Command;
    bool shopSelling = false;                                  // в Quantity: продаём, а не покупаем
    int shopCommand = 0, shopSel = 0, shopScroll = 0, shopCount = 1;
    std::string shopLine;                                      // что сейчас говорит продавец

    void drawConfig();
    [[nodiscard]] SDL_Rect configRowRect(int row) const;
    [[nodiscard]] SDL_Rect volumeBarRect(int row) const;
    void changeVolume(int row, int level);
    [[nodiscard]] std::vector<std::string> optionLabels(int row) const;   // варианты строки Encounters/Minimap
    [[nodiscard]] int optionValue(int row) const;
    void setOption(int row, int index);
    [[nodiscard]] SDL_Rect optionRect(int row, int index) const;
    void drawMinimap();
    void drawProp(const Prop& p, int camX, int camY);
    int lastCamX = 0, lastCamY = 0;
    std::string mapNotice;                                     // короткая надпись над картой (H: бои вкл/выкл)
    Uint32 mapNoticeTicks = 0;                             // камера последнего кадра карты: рамка на мини-карте
    int gameMenuSelected = 0;
    int configSelected = 0;                                    // 0 музыка, 1 звуки, 2 «Back»
    bool draggingVolume = false;

    std::array<int, 4> pickedClass{0, 1, 4, 5};
    std::array<SDL_Rect, 4> heroSlotRect{}, remoteSlotRect{};
    std::array<SDL_Rect, MAX_ENEMY_COUNT> enemySlotRect{};
    std::array<Uint32, MAX_ENEMY_COUNT> enemyHitAt{}, enemyDeathAt{}, enemyLungeAt{};   // 0 — не было
    std::array<std::string, 4> autoName;

    Action        pendingAction;
    UIChooseState uiState    { UIChooseState::ChooseAction };
    UIChooseState prevUIState{ UIChooseState::ChooseAction };
    int           currentHeroIndex    = 0;
    int           selectedTargetIndex = 0;
    TargetType    currentType         { TargetType::Enemy };

    SDL_Rect screen;
    std::map<std::string, Scene> scenes;

    Visualizer&      visualizer;
    AudioManager& audioManager;
    ResourceManager& resourceManager;
    Game&            game;
    Player&          player;
};

#endif