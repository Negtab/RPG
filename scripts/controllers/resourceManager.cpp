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


bool ResourceManager::addFont(const std::string &path)
{
    if (fonts.contains(getName(path)))
        return false;

    TTF_Font* font = TTF_OpenFont(path.c_str(), 24);
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
    SDL_Surface* textSurface = TTF_RenderText_Solid( font, text.c_str(), color );
    if (!textSurface)
        SDL_Log("Failed to render text: ", TTF_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(this->renderer, textSurface);
    SDL_FreeSurface(textSurface);
    textures[name] = textTexture;
    return textTexture;
}

SDL_Texture* ResourceManager::getTextTexture(const std::string& text, TTF_Font* font, SDL_Color color, uint32_t wrapWidth)
{
    if (textCache.contains(text))
        return textCache[text];

    SDL_Surface* textSurface;
    if (wrapWidth <= 0)
        textSurface = TTF_RenderText_Solid( font, text.c_str(), color);
    else
        textSurface = TTF_RenderUTF8_Blended_Wrapped( font, text.c_str(), color, wrapWidth);
    if (!textSurface)
        SDL_Log("Failed to render text: ", TTF_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!textCache.contains(text))
        textCache[text] = textTexture;

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

void ResourceManager::initialize()
{
    std::filesystem::path mainPath {std::filesystem::current_path().remove_filename()};
    std::filesystem::path pathToMusic {mainPath.string() + R"(resources\Music\)"};
    std::filesystem::path pathToSounds {mainPath.string() + R"(resources\Sounds\)"};
    std::filesystem::path pathToEnemies {mainPath.string() + R"(resources\Images\Enemies\)"};
    std::filesystem::path pathToCharacters {mainPath.string() + R"(resources\Images\Characters\)"};
    std::filesystem::path pathToBackgrounds {mainPath.string() + R"(resources\Images\Backgrounds\)"};
    std::filesystem::path pathToEnvironment {mainPath.string() + R"(resources\Images\Environment\)"};
    std::filesystem::path pathToAnimations {mainPath.string() + R"(resources\Animations\)"};

    addFont(mainPath.string() + "resources\\Fonts\\RetroByte.ttf");
    addMusic(pathToMusic.string() + "MenuTheme.mp3");
    addMusic(pathToMusic.string() + "MapTheme.mp3");
    addMusic(pathToMusic.string() + "BattleTheme.mp3");
    addMusic(pathToMusic.string() + "OptionsTheme.mp3");
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

    addTexture(pathToEnemies.string() + "EvilMagician.png");
    addTexture(pathToEnemies.string() + "EvilWarrior.png");
    addTexture(pathToEnemies.string() + "Gargoyle.png");
    addTexture(pathToEnemies.string() + "Gargoyle(Strong).png");
    addTexture(pathToEnemies.string() + "Ghost.png");
    addTexture(pathToEnemies.string() + "Ghost(Strong).png");
    addTexture(pathToEnemies.string() + "Golem.png");
    addTexture(pathToEnemies.string() + "Knight.png");
    addTexture(pathToEnemies.string() + "Knight(Royle).png");
    addTexture(pathToEnemies.string() + "Ogr.png");
    addTexture(pathToEnemies.string() + "Lich.png");

    addTexture(pathToBackgrounds.string() + "MapBackground.png");
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

static void writeString(std::ofstream& out, const std::string& str)
{
    uint64_t size = str.size();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(str.data(), size);
}

static std::string readString(std::ifstream& in)
{
    uint64_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));

    std::string str(size, '\0');
    in.read(str.data(), size);
    return str;
}

bool ResourceManager::isPlayerNew(const std::string& path) const {
    std::ifstream in(path);
    if (!in || !in.is_open())
        return true;
    return false;
}


void ResourceManager::savePlayer(const std::string& path, const Player& player)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot open file for saving player: " + path);

    // --- Header ---
    uint32_t magic = 0x504C5952; // 'PLYR'
    uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // --- Base data ---
    writeString(out, player.getPlayerName());

    uint32_t gold = player.getPlayerGold();
    out.write(reinterpret_cast<const char*>(&gold), sizeof(gold));

    int32_t speed = player.getPlayerSpeed();
    out.write(reinterpret_cast<const char*>(&speed), sizeof(speed));

    Point coords = player.getPlayerCoords();
    out.write(reinterpret_cast<const char*>(&coords), sizeof(coords));

    // --- Enemies ---
    const auto enemies = player.getDiscoveredEnemies();
    uint64_t enemyCount = enemies.size();
    out.write(reinterpret_cast<const char*>(&enemyCount), sizeof(enemyCount));
    for (const auto& e : enemies)
        writeString(out, e);

    // --- Items ---
    const auto items = player.getAvailableItems(); // vector<int>
    uint64_t itemCount = items.size();
    out.write(reinterpret_cast<const char*>(&itemCount), sizeof(itemCount));
    for (int id : items)
    {
        uint8_t count = 1; // если нет доступа к map — минимум
        out.write(reinterpret_cast<const char*>(&id), sizeof(id));
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    }
}


void ResourceManager::loadPlayer(const std::string& path, Player& player)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open file for loading player: " + path);

    uint32_t magic, version;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != 0x504C5952)
        throw std::runtime_error("Invalid save file");

    if (version != 1)
        throw std::runtime_error("Unsupported save version");

    // --- Base data ---
    player.setPlayerName(readString(in));

    uint32_t gold;
    in.read(reinterpret_cast<char*>(&gold), sizeof(gold));
    player.setPlayerGold(gold);

    int32_t speed;
    in.read(reinterpret_cast<char*>(&speed), sizeof(speed));
    player.setPlayerSpeed(speed);

    Point coords;
    in.read(reinterpret_cast<char*>(&coords), sizeof(coords));
    player.setPlayerCoords(coords);

    // --- Enemies ---
    uint64_t enemyCount;
    in.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
    for (uint64_t i = 0; i < enemyCount; ++i)
        player.addDiscoveredEnemy(readString(in));

    // --- Items ---
    uint64_t itemCount;
    in.read(reinterpret_cast<char*>(&itemCount), sizeof(itemCount));
    for (uint64_t i = 0; i < itemCount; ++i)
    {
        int id;
        uint8_t count;
        in.read(reinterpret_cast<char*>(&id), sizeof(id));
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        player.addItem(id, count);
    }
}
