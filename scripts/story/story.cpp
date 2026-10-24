#include "story.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <SDL_log.h>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "../main/game.h"
#include "../main/lang.h"

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
        {"partyName", luaPartyName}};
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
                lua_pushboolean(thread, *answer);
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
