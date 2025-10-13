#include <filesystem>
#include <fstream>
#include <SDL_image.h>

#include "resourceManager.h"

SDL_Texture *ResourceManager::getTextureFromImage(SDL_Renderer *renderer, SDL_Surface *surface) const { return SDL_CreateTextureFromSurface(renderer, surface); }
SDL_Surface *ResourceManager::getImage(const std::string &name) { return images[name]; }
Mix_Chunk *ResourceManager::getSound(const std::string &name) { return sounds[name]; }
Mix_Music *ResourceManager::getMusic(const std::string &name) { return music[name]; }
SDL_Texture *ResourceManager::getTexture(const std::string &name) { return textures[name]; }
TTF_Font *ResourceManager::getFont(const std::string &name) { return fonts[name]; }


bool ResourceManager::addFont(const std::string &path)
{
    if (fonts.contains(getName(path)))
        return fonts[getName(path)];

    TTF_Font* font = TTF_OpenFont(path.c_str(), 24);
    if (!font)
    {
        SDL_Log("Ошибка загрузки шрифта: %s", TTF_GetError());
        return 1;
    }
    fonts[getName(path)] = font;
    return 0;
}

bool ResourceManager::addTexture(const std::string& path, SDL_Renderer* renderer)
{
    if (textures.contains(getName(path)))
        return textures[getName(path)];

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) throw std::runtime_error("Failed to load image: " + path);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    textures[getName(path)] = texture;
    return texture;
}

bool ResourceManager::addTexture(const std::string& name, const std::string& text, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer)
{
    if (textures.contains(name))
        return textures[name];
    SDL_Surface* textSurface = TTF_RenderText_Solid( font, text.c_str(), color );
    if (!textSurface)
        SDL_Log("Failed to render text: ", TTF_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    textures[name] = textTexture;
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
    std::filesystem::path pathToImages {mainPath.string() + "resources\\Images\\"};
    std::filesystem::path pathToMusic {mainPath.string() + "resources\\Music\\"};
    addFont(mainPath.string() + "resources\\Fonts\\RetroByte.ttf");
    addMusic(pathToMusic.string() + "MenuTheme.mp3");
    addMusic(pathToMusic.string() + "MapTheme.mp3");
    addTexture(pathToImages.string() + "Backgrounds\\MapBackground.png", &this->renderer);
    addTexture(pathToImages.string() + "Backgrounds\\MenuBackground.png", &this->renderer);
    addTexture("ContinueButton", "Continue", this->getFont("RetroByte"),  {0, 0, 0, 255}, &this->renderer );
    addTexture("QuitButton", "Quit", this->getFont("RetroByte"),  {0, 0, 0, 255}, &this->renderer );
}

void ResourceManager::savePLayer(const std::string& path, Player& player)
{
    // Создаём папку, если её нет
    std::filesystem::path filePath(path);
    std::filesystem::create_directories(filePath.parent_path());

    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot open file for saving player: " + path);

    // -------- 1. Имя --------
    const std::string& name = player.getPlayerName();
    size_t nameSize = name.size();
    out.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize));
    out.write(name.data(), nameSize);

    // -------- 2. Простые данные --------
    uint32_t gold = player.getPlayerGold();
    out.write(reinterpret_cast<const char*>(&gold), sizeof(gold));

    int32_t speed = player.getPlayerSpeed();
    out.write(reinterpret_cast<const char*>(&speed), sizeof(speed));

    // Координаты (Point)
    const auto& coords = player.getPlayerCoords(); // если не нужно Game — просто coords
    out.write(reinterpret_cast<const char*>(&coords.x), sizeof(coords.x));
    out.write(reinterpret_cast<const char*>(&coords.y), sizeof(coords.y));

    // -------- 3. Известные враги --------
    const auto enemies = player.getDiscoveredEnemies();
    size_t enemyCount = enemies.size();
    out.write(reinterpret_cast<const char*>(&enemyCount), sizeof(enemyCount));
    for (auto e : enemies)
        out.write(reinterpret_cast<const char*>(&e), sizeof(e));

    // -------- 4. Предметы --------
    const auto items = player.getAvailableItems();
    size_t itemCount = items.size();
    out.write(reinterpret_cast<const char*>(&itemCount), sizeof(itemCount));
    for (auto id : items)
        out.write(reinterpret_cast<const char*>(&id), sizeof(id));

    out.close();
}

void ResourceManager::loadPLayer(const std::string& path, Player& player)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open file for loading player: " + path);

    // -------- 1. Имя --------
    size_t nameSize;
    in.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
    std::string name(nameSize, '\0');
    in.read(&name[0], nameSize);

    // -------- 2. Простые данные --------
    uint32_t gold;
    in.read(reinterpret_cast<char*>(&gold), sizeof(gold));
    player.setPlayerGold(gold);

    int32_t speed;
    in.read(reinterpret_cast<char*>(&speed), sizeof(speed));
    player.setPlayerSpeed(speed);

    Point coords{};
    in.read(reinterpret_cast<char*>(&coords.x), sizeof(coords.x));
    in.read(reinterpret_cast<char*>(&coords.y), sizeof(coords.y));
    // Если нужно — можешь добавить player.setCoords(coords);

    // -------- 3. Известные враги --------
    size_t enemyCount;
    in.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
    for (size_t i = 0; i < enemyCount; ++i)
    {
        EnemiesNames e;
        in.read(reinterpret_cast<char*>(&e), sizeof(e));
        player.addDiscoveredEnemy(e);
    }

    // -------- 4. Предметы --------
    size_t itemCount;
    in.read(reinterpret_cast<char*>(&itemCount), sizeof(itemCount));
    for (size_t i = 0; i < itemCount; ++i)
    {
        int id;
        in.read(reinterpret_cast<char*>(&id), sizeof(id));
        player.addItem(id, 1); // по умолчанию 1, можно записывать count
    }

    in.close();
}