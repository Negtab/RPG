#ifndef PROJECT_NAME_STORY_H
#define PROJECT_NAME_STORY_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct lua_State;
class Game;

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

private:
    enum class Wait { None, Dialog, Choice, Battle, Fade };

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

    int pause(lua_State* L, Wait what);
    void resume(int nargs);
    void finish();

    Game& game;
    lua_State* L = nullptr;
    lua_State* thread = nullptr;                      // идущая сцена
    int threadRef = -1;                               // ссылка в реестре Lua, чтобы корутину не собрал GC
    Wait wait = Wait::None;
    bool battleSeen = false;                          // для Wait::Battle: бой уже начался

    std::set<std::string> flags;
    std::vector<Quest> quests;                        // в порядке появления: так и показывает журнал
    std::unordered_map<std::string, std::string> textsEn, textsRu;
};

#endif //PROJECT_NAME_STORY_H
