#include "story.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <SDL_log.h>
#include <SDL_timer.h>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "../main/game.h"
#include "../main/lang.h"
#include "scriptArgs.h"

namespace {
    void loadTexts(const std::filesystem::path& path, std::unordered_map<std::string, std::string>& out)
    {
        out.clear();
        std::ifstream f(path);
        if (!f) { SDL_Log("Story: no %s", path.string().c_str()); return; }
        try
        {
            const auto j = nlohmann::json::parse(f, nullptr, true, true);
            for (const auto& [key, value] : j.items())
                if (value.is_string()) out[key] = value.get<std::string>();
        }
        catch (const nlohmann::json::exception& e)
        {
            SDL_Log("Story: bad %s: %s", path.string().c_str(), e.what());
        }
    }
}

Story::Story(Game& game) : game(game) {}

Story::~Story()
{
    if (L) lua_close(L);
}

void Story::load(const std::filesystem::path& dir)
{
    if (L) lua_close(L);
    L = luaL_newstate();
    luaL_openlibs(L);
    thread = nullptr;
    threadRef = -1;
    wait = Wait::None;

    const std::pair<const char*, lua_CFunction> api[] = {
        {"say", luaSay}, {"ask", luaAsk}, {"flag", luaFlag}, {"setFlag", luaSetFlag},
        {"hasItem", luaHasItem}, {"giveItem", luaGiveItem}, {"takeItem", luaTakeItem}, {"giveGold", luaGiveGold},
        {"quest", luaQuest}, {"battle", luaBattle}, {"teleport", luaTeleport}, {"healParty", luaHealParty},
        {"partyName", luaPartyName}, {"wait", luaWait}, {"npcWalk", luaNpcWalk}, {"npcFace", luaNpcFace},
        {"sound", luaSound}, {"partySize", luaPartySize}, {"heroClass", luaHeroClass}, {"heroLevel", luaHeroLevel},
        {"heroAlive", luaHeroAlive}, {"gold", luaGold}, {"takeGold", luaTakeGold}, {"here", luaHere},
        {"choose", luaChoose}, {"ending", luaEnding}};
    for (const auto& [name, fn] : api)
    {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, fn, 1);
        lua_setglobal(L, name);
    }
    luaL_dostring(L, "npc = {} event = {}");

    std::vector<std::filesystem::path> files;                    // по имени: порядок загрузки предсказуем
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(dir, ec))
        if (e.path().extension() == ".lua") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const auto& f : files)
        if (luaL_dofile(L, f.string().c_str()) != LUA_OK)
        {
            SDL_Log("Story: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }

    loadTexts(dir / "text_en.json", textsEn);
    loadTexts(dir / "text_ru.json", textsRu);
    SDL_Log("Story: %d scripts, %d/%d texts", static_cast<int>(files.size()),
            static_cast<int>(textsEn.size()), static_cast<int>(textsRu.size()));
}

void Story::reset()
{
    if (thread) finish();                                         // сцена оборвалась (Game Over): не продолжаем её
    flags.clear();
    quests.clear();
}

std::string Story::text(const std::string& id) const
{
    if (currentLanguage() == Language::Ru)
        if (const auto it = textsRu.find(id); it != textsRu.end()) return it->second;
    if (const auto it = textsEn.find(id); it != textsEn.end()) return it->second;
    return id;
}

std::vector<std::string> Story::endings() const
{
    std::set<std::string> ids;                                    // концовка может быть описана только на одном языке
    for (const auto* texts : {&textsEn, &textsRu})
        for (const auto& [key, value] : *texts)
            if (key.starts_with("ending.") && key.ends_with(".title") && key.size() > 13)
                ids.insert(key.substr(7, key.size() - 13));
    return {ids.begin(), ids.end()};
}

bool Story::has(const char* table, const std::string& name) const
{
    if (!L || name.empty()) return false;
    lua_getglobal(L, table);
    const bool found = lua_istable(L, -1) && lua_getfield(L, -1, name.c_str()) == LUA_TFUNCTION;
    lua_settop(L, 0);
    return found;
}

bool Story::start(const char* table, const std::string& name)
{
    if (busy() || !has(table, name)) return false;
    thread = lua_newthread(L);
    threadRef = luaL_ref(L, LUA_REGISTRYINDEX);                  // держим корутину, пока сцена не кончится
    lua_getglobal(thread, table);
    lua_getfield(thread, -1, name.c_str());
    lua_remove(thread, -2);
    wait = Wait::None;
    resume(0);
    return true;
}

void Story::resume(int nargs)
{
    wait = Wait::None;
    int results = 0;
    const int status = lua_resume(thread, L, nargs, &results);
    if (status == LUA_YIELD)
    {
        lua_pop(thread, results);
        return;                                                   // ждём то, что назначила pause()
    }
    if (status != LUA_OK)
        SDL_Log("Story: %s", lua_tostring(thread, -1));
    finish();
}

void Story::finish()
{
    if (threadRef != -1) luaL_unref(L, LUA_REGISTRYINDEX, threadRef);
    thread = nullptr;
    threadRef = -1;
    wait = Wait::None;
}

void Story::update()
{
    if (!thread) return;
    UIManager& ui = game.getUI();
    switch (wait)
    {
        case Wait::Dialog:
            if (!ui.isDialogOpen()) resume(0);
            break;
        case Wait::Choice:
            if (const auto answer = ui.takeChoice())
            {
                if (yesNo) lua_pushboolean(thread, *answer == 0);
                else       lua_pushinteger(thread, *answer + 1);
                resume(1);
            }
            break;
        case Wait::Battle:
            if (game.getGameState() == GameState::Battle) battleSeen = true;
            else if (battleSeen && game.getGameState() == GameState::Map)
            {
                lua_pushboolean(thread, game.getBattle()->wasWon());
                resume(1);
            }
            break;
        case Wait::Fade:
            if (!game.isFading()) resume(0);
            break;
        case Wait::Timer:
            if (SDL_GetTicks64() >= waitUntil) resume(0);
            break;
        case Wait::Walk:
            updateWalk();
            break;
        case Wait::Ending:                                        // сцена кончилась вместе с игрой: дальше не идёт
            break;
        case Wait::None:
            break;
    }
}

void Story::setFlag(const std::string& name, bool on, bool broadcast)
{
    if (on == flag(name)) return;
    if (on) flags.insert(name);
    else    flags.erase(name);
    game.onStoryChanged();
    if (broadcast) game.sendStoryState(0, name, on ? 1 : 0);
}

void Story::setQuest(const std::string& id, QuestState state, bool broadcast)
{
    const auto it = std::find_if(quests.begin(), quests.end(), [&](const Quest& q) { return q.id == id; });
    if (it == quests.end()) quests.push_back({id, state});
    else if (it->state == state) return;
    else it->state = state;
    if (broadcast) game.sendStoryState(1, id, static_cast<uint8_t>(state));
}

// ─────────────────────────────────────────
// Функции для сценариев
// ─────────────────────────────────────────

Story& Story::self(lua_State* L)
{
    return *static_cast<Story*>(lua_touserdata(L, lua_upvalueindex(1)));
}

int Story::pause(lua_State* L, Wait what)
{
    if (!lua_isyieldable(L)) return luaL_error(L, "this function only works inside a scene");
    wait = what;
    battleSeen = false;
    return lua_yield(L, 0);
}

// say("king", "king.intro.1", "king.intro.2") — реплики по одной странице; говорящий тоже id текста
int Story::luaSay(lua_State* L)
{
    Story& s = self(L);
    const std::string speaker = luaL_checkstring(L, 1);
    std::vector<std::string> pages;
    for (int i = 2; i <= lua_gettop(L); ++i) pages.push_back(s.text(luaL_checkstring(L, i)));
    s.game.showDialog(speaker.empty() ? "" : s.text("name." + speaker), pages);
    return s.pause(L, Wait::Dialog);
}

// ask("king", "king.question") -> true/false: вопрос и «Да / Нет»
int Story::luaAsk(lua_State* L)
{
    Story& s = self(L);
    const std::string speaker = luaL_checkstring(L, 1);
    s.game.getUI().showChoice(speaker.empty() ? "" : s.text("name." + speaker), s.text(luaL_checkstring(L, 2)));
    s.yesNo = true;
    return s.pause(L, Wait::Choice);
}

int Story::luaFlag(lua_State* L)
{
    lua_pushboolean(L, self(L).flag(luaL_checkstring(L, 1)));
    return 1;
}

int Story::luaSetFlag(lua_State* L)
{
    self(L).setFlag(luaL_checkstring(L, 1), lua_isnone(L, 2) || lua_toboolean(L, 2));
    return 0;
}

int Story::luaHasItem(lua_State* L)
{
    lua_pushboolean(L, self(L).game.getLocalPlayer().hasItem(static_cast<int>(luaL_checkinteger(L, 1))));
    return 1;
}

// giveItem(id[, count]) — «Получено: Лютня» и пауза, пока игрок не прочтёт
int Story::luaGiveItem(lua_State* L)
{
    Story& s = self(L);
    const int id = static_cast<int>(luaL_checkinteger(L, 1));
    const int count = static_cast<int>(luaL_optinteger(L, 2, 1));
    const Item* item = s.game.findItem(id);
    if (!item) return luaL_error(L, "giveItem: unknown item %d", id);
    s.game.localPlayer().addItem(id, static_cast<uint8_t>(std::clamp(count, 1, 99)));
    s.game.showDialog("", {trf("Received {0}!", {item->name})});
    return s.pause(L, Wait::Dialog);
}

int Story::luaTakeItem(lua_State* L)
{
    self(L).game.localPlayer().removeItem(static_cast<int>(luaL_checkinteger(L, 1)),
                                          static_cast<uint8_t>(luaL_optinteger(L, 2, 1)));
    return 0;
}

int Story::luaGiveGold(lua_State* L)
{
    Player& p = self(L).game.localPlayer();
    p.setPlayerGold(p.getPlayerGold() + static_cast<uint32_t>(std::max<lua_Integer>(0, luaL_checkinteger(L, 1))));
    return 0;
}

// quest("save_princess", "active" | "done"): запись в журнале; заголовок — текст quest.<id>
int Story::luaQuest(lua_State* L)
{
    const std::string state = luaL_checkstring(L, 2);
    self(L).setQuest(luaL_checkstring(L, 1), state == "done" ? QuestState::Done : QuestState::Active);
    return 0;
}

// battle({12}, "Temple") -> true, если победили. Сбежать из такого боя нельзя
int Story::luaBattle(lua_State* L)
{
    Story& s = self(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    std::vector<int> enemies;
    for (lua_Integer i = 1, n = luaL_len(L, 1); i <= n; ++i)
    {
        lua_geti(L, 1, i);
        enemies.push_back(static_cast<int>(lua_tointeger(L, -1)));
        lua_pop(L, 1);
    }
    s.game.startStoryBattle(enemies, luaL_optstring(L, 2, ""));
    return s.pause(L, Wait::Battle);
}

// teleport("cornelia_castle_2f", x, y) — затемнение и другая карта
int Story::luaTeleport(lua_State* L)
{
    Story& s = self(L);
    s.game.storyTeleport(luaL_checkstring(L, 1), static_cast<int>(luaL_checkinteger(L, 2)),
                         static_cast<int>(luaL_checkinteger(L, 3)));
    return s.pause(L, Wait::Fade);
}

int Story::luaHealParty(lua_State* L)
{
    for (auto& h : self(L).game.localPlayer().getLinkTOHeroes())
        if (h.getIsAlive()) { h.setCurrentHp(h.getMaxHp()); h.setCurrentMp(h.getMaxMp()); }
    return 0;
}

// partyName(i) — имя героя отряда (1-4): для реплик вроде «Спасибо, <имя>!»
int Story::luaPartyName(lua_State* L)
{
    const auto& heroes = self(L).game.getLocalPlayer().getHeroes();
    const lua_Integer i = luaL_optinteger(L, 1, 1);
    lua_pushstring(L, i >= 1 && i <= static_cast<lua_Integer>(heroes.size()) ? heroes[i - 1].getName().c_str() : "");
    return 1;
}

// ─────────────────────────────────────────
// Постановка сцен и вопросы об отряде
// ─────────────────────────────────────────

NotGamePerson* Story::npcHere(const std::string& script) const
{
    for (const auto& n : game.npcsHere())
        if (n->getScript() == script && game.npcVisible(*n)) return n.get();
    return nullptr;
}

// wait(0.5) — пауза в секундах: между репликами, шагами, звуками
int Story::luaWait(lua_State* L)
{
    Story& s = self(L);
    const double seconds = std::clamp(luaL_checknumber(L, 1), 0.0, 10.0);
    s.waitUntil = SDL_GetTicks64() + static_cast<uint64_t>(seconds * 1000.0);
    return s.pause(L, Wait::Timer);
}

// npcWalk("elf_guard", "l3 u") — житель (по имени сцены из Tiled) идёт по клеткам; сцена ждёт, пока дойдёт.
// Место не сохраняется: после ухода с карты житель снова стоит, где его поставили в Tiled
int Story::luaNpcWalk(lua_State* L)
{
    Story& s = self(L);
    const std::string who = luaL_checkstring(L, 1);
    const auto path = parseWalkPath(luaL_checkstring(L, 2));
    if (!path) return luaL_error(L, "npcWalk: bad path '%s' (use u d l r and step counts, like \"l3 u\")",
                                 lua_tostring(L, 2));
    if (!s.npcHere(who)) return luaL_error(L, "npcWalk: no '%s' on this map", who.c_str());
    s.walker = who;
    s.walkPath = *path;
    s.walkStep = 0;
    s.walkBlockedSince = 0;
    return s.pause(L, Wait::Walk);
}

void Story::updateWalk()
{
    NotGamePerson* n = npcHere(walker);
    if (!n) { resume(0); return; }                                // карта сменилась или житель исчез по флагу
    if (n->isStepping()) return;
    if (walkStep >= walkPath.size()) { resume(0); return; }

    const Direction dir = walkPath[walkStep];
    const Point to = stepTo(n->getCell(), dir);
    if (game.getMap().isWalkable(to.x, to.y) && game.npcCellFree(to))
    {
        n->walk(to);
        ++walkStep;
        walkBlockedSince = 0;
        return;
    }
    n->turn(dir);                                                 // стоит и смотрит туда, куда не может пройти
    const uint64_t now = SDL_GetTicks64();
    if (walkBlockedSince == 0) walkBlockedSince = now;
    else if (now - walkBlockedSince > 1500)
    {
        SDL_Log("Story: npcWalk '%s' blocked at (%d, %d)", walker.c_str(), to.x, to.y);
        resume(0);
    }
}

// npcFace("elf_guard", "down")
int Story::luaNpcFace(lua_State* L)
{
    Story& s = self(L);
    const auto dir = parseDirection(luaL_checkstring(L, 2));
    if (!dir) return luaL_error(L, "npcFace: bad direction '%s'", lua_tostring(L, 2));
    if (NotGamePerson* n = s.npcHere(luaL_checkstring(L, 1))) n->turn(*dir);
    return 0;
}

// sound("fx_heal") — звук из resources/Sounds по имени файла без расширения
int Story::luaSound(lua_State* L)
{
    UIManager::uiSound(luaL_checkstring(L, 1));
    return 0;
}

int Story::luaPartySize(lua_State* L)
{
    lua_pushinteger(L, static_cast<lua_Integer>(self(L).game.getLocalPlayer().getHeroes().size()));
    return 1;
}

namespace {
    // Герой i (1-4) для heroClass / heroLevel / heroAlive; нет такого — nullptr
    const Hero* heroArg(const Game& game, lua_State* L)
    {
        const auto& heroes = game.getLocalPlayer().getHeroes();
        const lua_Integer i = luaL_checkinteger(L, 1);
        return i >= 1 && i <= static_cast<lua_Integer>(heroes.size()) ? &heroes[i - 1] : nullptr;
    }
}

// heroClass(1) -> "warrior" | "thief" | "monk" | "red_mage" | "white_mage" | "black_mage"
int Story::luaHeroClass(lua_State* L)
{
    const Hero* h = heroArg(self(L).game, L);
    lua_pushstring(L, h ? classScriptName(h->getClass()) : "");
    return 1;
}

int Story::luaHeroLevel(lua_State* L)
{
    const Hero* h = heroArg(self(L).game, L);
    lua_pushinteger(L, h ? h->getLevel() : 0);
    return 1;
}

int Story::luaHeroAlive(lua_State* L)
{
    const Hero* h = heroArg(self(L).game, L);
    lua_pushboolean(L, h && h->getIsAlive());
    return 1;
}

int Story::luaGold(lua_State* L)
{
    lua_pushinteger(L, self(L).game.getLocalPlayer().getPlayerGold());
    return 1;
}

// takeGold(500) -> true и списывает; не хватает — false, золото не трогает
int Story::luaTakeGold(lua_State* L)
{
    Player& p = self(L).game.localPlayer();
    const lua_Integer cost = std::max<lua_Integer>(0, luaL_checkinteger(L, 1));
    const bool enough = p.getPlayerGold() >= cost;
    if (enough) p.setPlayerGold(p.getPlayerGold() - static_cast<uint32_t>(cost));
    lua_pushboolean(L, enough);
    return 1;
}

// here() -> имя текущей карты, как в teleport: "elfheim", "marsh_cave_b1"
int Story::luaHere(lua_State* L)
{
    lua_pushstring(L, self(L).game.getMap().name.c_str());
    return 1;
}

// choose("elder", "elder.q", "elder.a.help", "elder.a.refuse") -> 1, 2, ... : вопрос и 2-4 ответа (id текстов)
int Story::luaChoose(lua_State* L)
{
    Story& s = self(L);
    const std::string speaker = luaL_checkstring(L, 1);
    const std::string question = luaL_checkstring(L, 2);
    const int answers = lua_gettop(L) - 2;
    if (answers < 2 || answers > 4) return luaL_error(L, "choose: needs 2 to 4 answers, got %d", answers);
    std::vector<std::string> options;
    for (int i = 3; i <= lua_gettop(L); ++i) options.push_back(s.text(luaL_checkstring(L, i)));
    s.game.getUI().showChoice(speaker.empty() ? "" : s.text("name." + speaker), s.text(question), std::move(options));
    s.yesNo = false;
    return s.pause(L, Wait::Choice);
}

// ending("peace") — финал: слайды ending.peace.1, .2, ..., титры и главное меню. Сцена дальше не идёт
int Story::luaEnding(lua_State* L)
{
    Story& s = self(L);
    const std::string id = luaL_checkstring(L, 1);
    if (!s.hasText("ending." + id + ".title"))
        return luaL_error(L, "ending: no text 'ending.%s.title'", id.c_str());
    s.game.startEnding(id);
    return s.pause(L, Wait::Ending);
}
