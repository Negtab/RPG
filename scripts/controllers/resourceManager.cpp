#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <SDL_image.h>

#include "resourceManager.h"

#include <algorithm>

std::unordered_map<std::string, SDL_Texture*> ResourceManager::textCache;
std::unordered_map<std::string, SDL_Texture*> ResourceManager::textures;
std::unordered_map<std::string, Mix_Music*> ResourceManager::music;
std::unordered_map<std::string, Mix_Chunk*> ResourceManager::sounds;
std::unordered_map<std::string, SDL_Surface*> ResourceManager::images;
std::unordered_map<std::string, std::vector<SDL_Texture*>> ResourceManager::animations;
std::unordered_map<std::string, TTF_Font*> ResourceManager::fonts;
SDL_Renderer* ResourceManager::renderer = nullptr; 

SDL_Texture *ResourceManager::getTextureFromImage(SDL_Surface *surface) { return SDL_CreateTextureFromSurface(renderer, surface); }
SDL_Surface *ResourceManager::getImage(const std::string &name) { return images[name]; }
Mix_Chunk *ResourceManager::getSound(const std::string &name) { return sounds[name]; }
Mix_Music *ResourceManager::getMusic(const std::string &name) { return music[name]; }
SDL_Texture *ResourceManager::getTexture(const std::string &name) { return textures[name]; }
TTF_Font *ResourceManager::getFont(const std::string &name) { return fonts[name]; }
std::vector<SDL_Texture*>& ResourceManager::getAnimation(const std::string& name) {return animations.at(name);}


bool ResourceManager::addFont(const std::string &path, int size)
{
    if (fonts.contains(getName(path)))
        return false;

    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font)
    {
        SDL_Log("Ошибка загрузки шрифта: %s", TTF_GetError());
        return false;
    }
    fonts[getName(path)] = font;
    return true;
}

bool ResourceManager::addTexture(const std::string& path)
{
    if (textures.contains(getName(path)))
        return false;

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) throw std::runtime_error("Failed to load image: " + path);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    SDL_FreeSurface(surface);
    textures[getName(path)] = texture;
    return texture;
}

bool ResourceManager::addTextTexture(const std::string& name, const std::string& text, TTF_Font* font, SDL_Color color)
{
    if (textures.contains(name))
        return false;
    SDL_Surface* textSurface = TTF_RenderUTF8_Solid( font, text.c_str(), color );
    if (!textSurface)
        SDL_Log("Failed to render text: ", TTF_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(this->renderer, textSurface);
    SDL_FreeSurface(textSurface);
    textures[name] = textTexture;
    return textTexture;
}

SDL_Texture* ResourceManager::getTextTexture(const std::string& text, TTF_Font* font, SDL_Color color, uint32_t wrapWidth)
{
    // Один и тот же текст бывает разного цвета (например, серое недоступное умение), поэтому цвет входит в ключ
    const std::string key = text + '\x1f' + std::to_string(color.r) + ',' + std::to_string(color.g) + ','
                            + std::to_string(color.b) + ',' + std::to_string(color.a) + ',' + std::to_string(wrapWidth);
    if (auto it = textCache.find(key); it != textCache.end())
        return it->second;

    SDL_Surface* textSurface;
    if (wrapWidth <= 0)
        textSurface = TTF_RenderUTF8_Solid( font, text.c_str(), color);
    else
        textSurface = TTF_RenderUTF8_Blended_Wrapped( font, text.c_str(), color, wrapWidth);
    if (!textSurface)
        SDL_Log("Failed to render text: ", TTF_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    textCache[key] = textTexture;

    return textTexture;
}


bool ResourceManager::addImage(const std::string &path)
{
    if (!std::filesystem::exists(path) && (!isFilePNG(path) || !isFileJPG(path)))
        return false;

    images[getName(path)] = IMG_Load(path.c_str());
    return images[getName(path)] != nullptr;
}

bool ResourceManager::addMusic(const std::string &path)
{
    if (!std::filesystem::exists(path) || !isFileMP3(path))
        return false;
    music[getName(path)] = Mix_LoadMUS(path.c_str());
    return music[getName(path)] != nullptr;
}

bool ResourceManager::addSound(const std::string &path)
{
    if (!std::filesystem::exists(path) || !isFileWAV(path))
        return false;
    sounds[getName(path)] = Mix_LoadWAV(path.c_str());
    return sounds[getName(path)] != nullptr;
}

bool ResourceManager::addAnimation(const std::string& path)
{
    const std::string name = getName(path);
    if (animations.contains(name))
        return false;

    try
    {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(path))
            files.push_back(entry.path());

        std::sort(files.begin(), files.end());

        for (const auto& file : files)
        {
            SDL_Surface* surface = IMG_Load(file.string().c_str());
            if (!surface)
            {
                SDL_Log("IMG_Load failed: %s", IMG_GetError());
                continue;
            }

            SDL_Texture* tex = SDL_CreateTextureFromSurface(this->renderer, surface);
            SDL_FreeSurface(surface);

            if (tex)
                animations[name].push_back(tex);
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

void ResourceManager::addTexturesFromDirectory(const std::string& dir)
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
        if (entry.is_regular_file() && isFilePNG(entry.path().string()))
            addTexture(entry.path().string());          // имя в таблице = имя файла без расширения
}

bool ResourceManager::isFileTTF(const std::string &path)
{
    if (path.size() < 5 || path.substr(path.size() - 4) != ".ttf")
        return false;
    return true;
}


bool ResourceManager::isFileJPG(const std::string &path)
{
    if (path.size() < 5 || path.substr(path.size() - 4) != ".jpg")
        return false;
    return true;
}

bool ResourceManager::isFilePNG(const std::string &path)
{
    if (path.size() < 5 || path.substr(path.size() - 4) != ".png")
        return false;
    return true;
}

bool ResourceManager::isFileMP3(const std::string &path)
{
    if (path.size() < 5 || path.substr(path.size() - 4) != ".mp3")
        return false;
    return true;
}

bool ResourceManager::isFileWAV(const std::string &path)
{
    if (path.size() < 5 || path.substr(path.size() - 4) != ".wav")
        return false;
    return true;
}

std::string ResourceManager::getName(const std::string &path)
{
    return std::filesystem::path(path).stem().string();
}

TextSize ResourceManager::measureText(const std::string& text, TTF_Font* font) {
    TextSize size{0, 0};
    if (!font || text.empty()) return size;
    TTF_SizeUTF8(font, text.c_str(), &size.w, &size.h);
    return size;
}
TextBlock ResourceManager::renderMultiline(SDL_Renderer* renderer, const std::string& text, TTF_Font* font, SDL_Color color, int maxWidth)   // 0 = без переноса
{
    // TTF_RenderUTF8_Blended_Wrapped делает это за нас
    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(
        font, text.c_str(), color,
        maxWidth > 0 ? maxWidth : 10000
    );
    if (!surface) return {};

    TextBlock block;
    block.w = surface->w;
    block.h = surface->h;
    block.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return block;
}

void ResourceManager::releaseImage(const std::string& name)
{
    auto it = images.find(name);
    if (it == images.end()) return;
    SDL_FreeSurface(it->second);
    images.erase(it);
}

void ResourceManager::initialize()
{
    std::filesystem::path mainPath {std::filesystem::current_path().remove_filename()};

    /*char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    mainPath = std::filesystem::path(buffer).parent_path();*/

    std::filesystem::path pathToMusic {mainPath.string() + R"(\resources\Music\)"};
    std::filesystem::path pathToSounds {mainPath.string() + R"(\resources\Sounds\)"};
    std::filesystem::path pathToEnemies {mainPath.string() + R"(\resources\Images\Enemies\)"};
    std::filesystem::path pathToCharacters {mainPath.string() + R"(\resources\Images\Characters\)"};
    std::filesystem::path pathToBackgrounds {mainPath.string() + R"(\resources\Images\Backgrounds\)"};
    std::filesystem::path pathToEnvironment {mainPath.string() + R"(\resources\Images\Environment\)"};
    std::filesystem::path pathToAnimations {mainPath.string() + R"(\resources\Animations\)"};

    addFont((mainPath / "resources" / "Fonts" / (std::string(kUiFont) + ".ttf")).string(), kUiFontSize);
    addMusic(pathToMusic.string() + "MenuTheme.mp3");
    addMusic(pathToMusic.string() + "MapTheme.mp3");
    addMusic(pathToMusic.string() + "BattleTheme.mp3");
    addMusic(pathToMusic.string() + "OptionsTheme.mp3");
    addMusic(pathToMusic.string() + "OnlineTheme.mp3");
    addSound(pathToSounds.string() + "Menu\\ButtonHover.wav");

    addTexture(pathToCharacters.string() + "MovingCharacter.png");
    addTexture(pathToCharacters.string() + "Image1.png");
    addTexture(pathToCharacters.string() + "Image2.png");
    addTexture(pathToCharacters.string() + "Image3.png");
    addTexture(pathToCharacters.string() + "Image4.png");
    addTexture(pathToCharacters.string() + "Image1Dead.png");
    addTexture(pathToCharacters.string() + "Image2Dead.png");
    addTexture(pathToCharacters.string() + "Image3Dead.png");
    addTexture(pathToCharacters.string() + "Image4Dead.png");

    addTexturesFromDirectory(pathToEnemies.string());
    addTexturesFromDirectory((mainPath / "resources" / "Images" / "Vehicles").string());   // Ship, Canoe, Airship
    addTexturesFromDirectory((mainPath / "resources" / "Images" / "Props").string());      // замки, башни, мосты
    addTexturesFromDirectory((mainPath / "resources" / "Images" / "Overworld").string());
    addTexturesFromDirectory((mainPath / "resources" / "Images" / "Npcs").string());       // Npc<Имя>: жители городов  // Walk<набор>: герой на карте
    addTexturesFromDirectory((mainPath / "resources" / "Images" / "Effects").string());    // боевые эффекты и замахи
    addTexturesFromDirectory((mainPath / "resources" / "Images" / "BattleBackgrounds").string());   // задники боя BB<имя>

    addTexture(pathToBackgrounds.string() + "WhiteBox.jpg");
    addTexture(pathToBackgrounds.string() + "MenuBackground.png");
    addTexture(pathToBackgrounds.string() + "OptionsBackground.png");
    addTexture(pathToBackgrounds.string() + "BattleImage0.png");
    addTexture(pathToBackgrounds.string() + "BattleImage1.png");
    addTexture(pathToBackgrounds.string() + "BattleImage2.png");
    addTexture(pathToBackgrounds.string() + "BattleTile.png");

    addTexture(pathToEnvironment.string() + "ActorSelector.png");
    addTexture(pathToEnvironment.string() + "TargetSelector.png");
    addImage(pathToEnvironment.string() + "Sword.png");

    addAnimation(pathToAnimations.string() + "Hero1Animation");
    addAnimation(pathToAnimations.string() + "Hero2Animation");
    addAnimation(pathToAnimations.string() + "Hero3Animation");
    addAnimation(pathToAnimations.string() + "Hero4Animation");

    addAnimation(pathToAnimations.string() + "MoveDownAnimation");
    addAnimation(pathToAnimations.string() + "MoveUpAnimation");
    addAnimation(pathToAnimations.string() + "MoveLeftAnimation");
    addAnimation(pathToAnimations.string() + "MoveRightAnimation");
}

constexpr uint32_t kSaveMagic   = 0x504C5952;   // 'PLYR'
constexpr uint32_t kSaveVersion = 7;            // 2: координаты в клетках + mapId; 3: герои; 4: локация, время игры, дата; 5: экипировка; 6: транспорт; 7: посещённые места
constexpr uint64_t kMaxString   = 1024;
constexpr uint64_t kMaxCount    = 100000;

template <class T>
void writePod(std::ostream& out, const T& v) { out.write(reinterpret_cast<const char*>(&v), sizeof(T)); }

template <class T>
void readPod(std::istream& in, T& v)
{
    in.read(reinterpret_cast<char*>(&v), sizeof(T));
    if (!in) throw std::runtime_error("save file is truncated");
}

void writeString(std::ostream& out, const std::string& s)
{
    writePod<uint64_t>(out, s.size());
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
}

std::string readString(std::istream& in)
{
    uint64_t size = 0;
    readPod(in, size);
    if (size > kMaxString) throw std::runtime_error("save file is corrupted (string size)");

    std::string s(static_cast<size_t>(size), '\0');
    in.read(s.data(), static_cast<std::streamsize>(size));
    if (!in) throw std::runtime_error("save file is truncated");
    return s;
}

namespace {
// Содержимое файла целиком. Сначала читаем всё сюда, применяем только если ошибок не было
struct SaveData
{
    struct Hero
    {
        uint8_t cls = 0, level = 1; uint32_t exp = 0, hp = 0, mp = 0; std::string name;
        std::array<int32_t, kEquipSlots> equipment{};
        bool hasEquipment = false;                       // до версии 5 экипировки не было: выдадим стартовую
    };

    std::string name;
    uint32_t gold = 0;
    int32_t cx = 0, cy = 0;
    uint16_t mapId = 0;
    bool hasPosition = false;
    std::vector<std::string> enemies;
    std::vector<std::pair<int, uint8_t>> items;
    std::vector<Hero> heroes;
    std::string location;
    uint32_t playSeconds = 0;
    int64_t savedAt = 0;
    uint8_t riding = 0;
    std::array<VehicleState, kVehicleKinds> vehicles{};
    bool hasVehicles = false;
    std::vector<std::string> visited;                    // 7: где отряд побывал                            // до версии 6 транспорта не было: Game расставит его сам
};

// Бросает std::runtime_error, если файл не открыть или он испорчен
SaveData readSaveFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open save file: " + path);

    uint32_t magic = 0, version = 0;
    readPod(in, magic);
    readPod(in, version);
    if (magic != kSaveMagic)
        throw std::runtime_error("not a save file");
    if (version < 1 || version > kSaveVersion)
        throw std::runtime_error("unsupported save version " + std::to_string(version));

    SaveData d;
    d.name = readString(in);
    readPod(in, d.gold);

    readPod(in, d.cx);
    readPod(in, d.cy);
    if (version >= 2)
        readPod(in, d.mapId);
    d.hasPosition = version >= 2;

    uint64_t enemyCount = 0;
    readPod(in, enemyCount);
    if (enemyCount > kMaxCount)
        throw std::runtime_error("save file is corrupted (enemy count)");
    for (uint64_t i = 0; i < enemyCount; ++i)
        d.enemies.push_back(readString(in));

    uint64_t itemCount = 0;
    readPod(in, itemCount);
    if (itemCount > kMaxCount)
        throw std::runtime_error("save file is corrupted (item count)");
    for (uint64_t i = 0; i < itemCount; ++i)
    {
        int32_t id = 0;
        uint8_t count = 0;
        readPod(in, id);
        readPod(in, count);
        d.items.emplace_back(id, count);
    }

    if (version >= 3)
    {
        uint8_t count = 0;
        readPod(in, count);
        if (count > 4) throw std::runtime_error("save file is corrupted (hero count)");
        for (int i = 0; i < count; ++i)
        {
            SaveData::Hero s;
            readPod(in, s.cls); readPod(in, s.level); readPod(in, s.exp);
            readPod(in, s.hp);  readPod(in, s.mp);
            s.name = sanitizeHeroName(readString(in));
            if (version >= 5)
            {
                for (auto& id : s.equipment) readPod(in, id);
                s.hasEquipment = true;
            }
            if (s.cls > static_cast<uint8_t>(SpecializationNames::Thief) || s.level < 1 || s.level > 99)
                throw std::runtime_error("save file is corrupted (hero data)");
            d.heroes.push_back(std::move(s));
        }
    }

    if (version >= 4)
    {
        d.location = readString(in);
        readPod(in, d.playSeconds);
        readPod(in, d.savedAt);
    }

    if (version >= 6)
    {
        readPod(in, d.riding);
        if (d.riding > static_cast<uint8_t>(Vehicle::Airship)) throw std::runtime_error("save file is corrupted (vehicle)");
        for (auto& v : d.vehicles)
        {
            uint8_t available = 0, dir = 0;
            int32_t x = 0, y = 0;
            readPod(in, available); readPod(in, v.mapId); readPod(in, x); readPod(in, y); readPod(in, dir);
            if (dir > static_cast<uint8_t>(Direction::Right)) throw std::runtime_error("save file is corrupted (vehicle)");
            v.available = available != 0;
            v.cell = {x, y};
            v.dir = static_cast<Direction>(dir);
        }
        d.hasVehicles = true;
    }

    if (version >= 7)
    {
        uint16_t count = 0;
        readPod(in, count);
        for (int i = 0; i < count; ++i) d.visited.push_back(readString(in));
    }
    return d;
}
}


bool ResourceManager::isPlayerNew(const std::string& path) const
{
    std::error_code ec;
    return !std::filesystem::is_regular_file(path, ec) || std::filesystem::file_size(path, ec) == 0;
}

bool ResourceManager::savePlayer(const std::string& path, const Player& player, const SaveMeta& meta)
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);   // папка saves/ при первом сохранении

    const std::string tmpPath = path + ".tmp";   // пишем во временный файл: сбой посреди записи не убьёт старое сохранение
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            SDL_Log("Cannot open %s for saving", tmpPath.c_str());
            return false;
        }

        writePod(out, kSaveMagic);
        writePod(out, kSaveVersion);

        writeString(out, player.getPlayerName());
        writePod<uint32_t>(out, player.getPlayerGold());

        const MoveState& m = player.getMove();
        writePod<int32_t>(out, m.cell.x);
        writePod<int32_t>(out, m.cell.y);
        writePod<uint16_t>(out, m.mapId);

        const auto enemies = player.getDiscoveredEnemies();
        writePod<uint64_t>(out, enemies.size());
        for (const auto& e : enemies)
            writeString(out, e);

        const auto& items = player.getItemCounts();   // реальные количества, а не «1 на всё»
        writePod<uint64_t>(out, items.size());
        for (const auto& [id, count] : items)
        {
            writePod<int32_t>(out, id);
            writePod<uint8_t>(out, count);
        }

        const auto& heroes = player.getHeroes();
        writePod<uint8_t>(out, static_cast<uint8_t>(std::min<size_t>(heroes.size(), 4)));
        for (size_t i = 0; i < heroes.size() && i < 4; ++i)
        {
            const Hero& h = heroes[i];
            writePod<uint8_t>(out, static_cast<uint8_t>(h.getClass()));
            writePod<uint8_t>(out, h.getLevel());
            writePod<uint32_t>(out, h.getExperience());
            writePod<uint32_t>(out, h.getCurrentHp());
            writePod<uint32_t>(out, h.getCurrentMp());
            writeString(out, h.getName());
            for (int id : h.getEquipment())
                writePod<int32_t>(out, id);
        }

        writeString(out, meta.location.substr(0, kMaxString));
        writePod<uint32_t>(out, meta.playSeconds);
        writePod<int64_t>(out, static_cast<int64_t>(std::time(nullptr)));

        writePod<uint8_t>(out, static_cast<uint8_t>(player.getRiding()));
        for (const auto& v : player.getVehicles())
        {
            writePod<uint8_t>(out, v.available ? 1 : 0);
            writePod<uint16_t>(out, v.mapId);
            writePod<int32_t>(out, v.cell.x);
            writePod<int32_t>(out, v.cell.y);
            writePod<uint8_t>(out, static_cast<uint8_t>(v.dir));
        }

        writePod<uint16_t>(out, static_cast<uint16_t>(player.getVisited().size()));
        for (const auto& place : player.getVisited())
            writeString(out, place);

        out.flush();
        if (!out)
        {
            SDL_Log("Failed to write save file");
            return false;
        }
    }   // файл закрыт

    std::filesystem::rename(tmpPath, path, ec);
    if (ec)                                           // на некоторых тулчейнах rename не заменяет существующий файл
    {
        std::filesystem::remove(path, ec);
        std::filesystem::rename(tmpPath, path, ec);
    }
    if (ec)
    {
        SDL_Log("Cannot replace save file: %s", ec.message().c_str());
        return false;
    }
    return true;
}

// Заменяет прогресс игрока содержимым файла. Возвращает true, если в файле есть позиция
// (в версии 1 она была в пикселях старого движка). При ошибке бросает std::runtime_error, игрок остаётся нетронутым
bool ResourceManager::loadPlayer(const std::string& path, Player& player, SaveMeta* meta)
{
    const SaveData d = readSaveFile(path);

    player.resetProgress();                           // иначе предметы и враги сложатся с текущей игрой
    player.setPlayerName(d.name);
    player.setPlayerGold(d.gold);
    if (d.hasPosition)
    {
        player.setPlayerCoords({d.cx, d.cy});
        player.getMove().mapId = d.mapId;
    }
    for (const auto& e : d.enemies)
        player.addDiscoveredEnemy(e);
    for (const auto& [id, count] : d.items)
        player.addItem(id, count);

    auto& heroes = player.getLinkTOHeroes();
    for (const auto& s : d.heroes)
    {
        Hero hero{Specialization(static_cast<SpecializationNames>(s.cls))};
        hero.setLevel(s.level);
        hero.setExperience(s.exp);
        hero.setCurrentHp(s.hp);
        hero.setCurrentMp(s.mp);
        if (!s.name.empty()) hero.setName(s.name);
        if (s.hasEquipment)
            for (int slot = 0; slot < kEquipSlots; ++slot)
                hero.equip(static_cast<EquipSlot>(slot), s.equipment[slot]);   // неизвестный id equip просто не наденет
        else
            hero.equipStarting();
        heroes.push_back(std::move(hero));
    }

    for (const auto& place : d.visited)
        player.markVisited(place);

    if (d.hasVehicles)
    {
        player.setRiding(static_cast<Vehicle>(d.riding));
        player.getVehicles() = d.vehicles;
    }

    if (meta)
    {
        meta->location = d.location;
        meta->playSeconds = d.playSeconds;
        meta->hasVehicles = d.hasVehicles;
    }
    return d.hasPosition;
}

std::optional<SaveSummary> ResourceManager::readSaveSummary(const std::string& path) const
{
    if (isPlayerNew(path)) return std::nullopt;

    SaveSummary s;
    try
    {
        const SaveData d = readSaveFile(path);
        s.location = d.location;
        s.gold = d.gold;
        s.playSeconds = d.playSeconds;
        s.savedAt = d.savedAt;
        for (const auto& h : d.heroes)
            s.heroes.push_back({static_cast<SpecializationNames>(h.cls), h.level, h.name});
    }
    catch (const std::exception& e)
    {
        SDL_Log("Save '%s' is unreadable: %s", path.c_str(), e.what());
        s.corrupted = true;
    }
    return s;
}
