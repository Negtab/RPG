#ifndef PROJECT_NAME_STORY_H
#define PROJECT_NAME_STORY_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../main/types.h"

struct lua_State;
class Game;
class NotGamePerson;

// Сюжет: сцены на Lua (resources/Story/*.lua), флаги, журнал заданий и тексты на двух языках.
// Сцена — функция npc.<имя>() или event.<имя>(); она идёт корутиной и встаёт на паузу на say, ask,
// giveItem, battle, teleport, пока игрок читает, отвечает или дерётся. Флаги и журнал — общие для отряда:
// в кооперативе их рассылает тот, кто изменил, остальные принимают
class Story
{
public:
    enum class QuestState : uint8_t { Active = 1, Done = 2 };
    struct Quest { std::string id; QuestState state; };

    explicit Story(Game& game);
    ~Story();
    Story(const Story&) = delete;
    Story& operator=(const Story&) = delete;

    void load(const std::filesystem::path& dir);     // все *.lua и text_<язык>.json
    void reset();                                     // новая игра: флаги и журнал пусты

    [[nodiscard]] bool has(const char* table, const std::string& name) const;   // есть ли npc.<name> / event.<name>
    bool start(const char* table, const std::string& name);                       // false — такой сцены нет
    [[nodiscard]] bool busy() const noexcept { return thread != nullptr; }
    void update();                                    // каждый кадр: продолжить сцену, если её ожидание закончилось

    [[nodiscard]] bool flag(const std::string& name) const { return flags.contains(name); }
    void setFlag(const std::string& name, bool on, bool broadcast = true);
    [[nodiscard]] const std::set<std::string>& getFlags() const noexcept { return flags; }
    void setQuest(const std::string& id, QuestState state, bool broadcast = true);
    [[nodiscard]] const std::vector<Quest>& getQuests() const noexcept { return quests; }

    [[nodiscard]] std::string text(const std::string& id) const;   // на текущем языке; нет — английский, нет — сам id
    [[nodiscard]] bool hasText(const std::string& id) const { return textsEn.contains(id) || textsRu.contains(id); }
    [[nodiscard]] std::vector<std::string> endings() const;        // все концовки: id, у которых есть ending.<id>.title

private:
    enum class Wait { None, Dialog, Choice, Battle, Fade, Timer, Walk, Ending };

    static Story& self(lua_State* L);
    static int luaSay(lua_State* L);
    static int luaAsk(lua_State* L);
    static int luaFlag(lua_State* L);
    static int luaSetFlag(lua_State* L);
    static int luaHasItem(lua_State* L);
    static int luaGiveItem(lua_State* L);
    static int luaTakeItem(lua_State* L);
    static int luaGiveGold(lua_State* L);
    static int luaQuest(lua_State* L);
    static int luaBattle(lua_State* L);
    static int luaTeleport(lua_State* L);
    static int luaHealParty(lua_State* L);
    static int luaPartyName(lua_State* L);
    static int luaWait(lua_State* L);
    static int luaNpcWalk(lua_State* L);
    static int luaNpcFace(lua_State* L);
    static int luaSound(lua_State* L);
    static int luaPartySize(lua_State* L);
    static int luaHeroClass(lua_State* L);
    static int luaHeroLevel(lua_State* L);
    static int luaHeroAlive(lua_State* L);
    static int luaGold(lua_State* L);
    static int luaTakeGold(lua_State* L);
    static int luaHere(lua_State* L);
    static int luaChoose(lua_State* L);
    static int luaEnding(lua_State* L);

    int pause(lua_State* L, Wait what);
    void updateWalk();
    [[nodiscard]] NotGamePerson* npcHere(const std::string& script) const;   // житель текущей карты по имени сцены
    void resume(int nargs);
    void finish();

    Game& game;
    lua_State* L = nullptr;
    lua_State* thread = nullptr;                      // идущая сцена
    int threadRef = -1;                               // ссылка в реестре Lua, чтобы корутину не собрал GC
    Wait wait = Wait::None;
    bool battleSeen = false;                          // для Wait::Battle: бой уже начался
    bool yesNo = false;                               // для Wait::Choice: ask отвечает true/false, choose — номером
    uint64_t waitUntil = 0;                           // для Wait::Timer: SDL_GetTicks64
    std::string walker;                               // для Wait::Walk: кто идёт и куда
    std::vector<Direction> walkPath;
    size_t walkStep = 0;
    uint64_t walkBlockedSince = 0;                    // путь загорожен: подождём немного, потом сдадимся

    std::set<std::string> flags;
    std::vector<Quest> quests;                        // в порядке появления: так и показывает журнал
    std::unordered_map<std::string, std::string> textsEn, textsRu;
};

#endif //PROJECT_NAME_STORY_H
