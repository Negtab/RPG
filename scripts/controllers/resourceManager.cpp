#include <filesystem>
#include <fstream>
#include <SDL_image.h>

#include "resourceManager.h"

SDL_Texture *ResourceManager::getTextureFromImage(SDL_Surface *surface) const { return SDL_CreateTextureFromSurface(&this->renderer, surface); }
SDL_Surface *ResourceManager::getImage(const std::string &name) { return images[name]; }
Mix_Chunk *ResourceManager::getSound(const std::string &name) { return sounds[name]; }
Mix_Music *ResourceManager::getMusic(const std::string &name) { return music[name]; }
SDL_Texture *ResourceManager::getTexture(const std::string &name) { return textures[name]; }
TTF_Font *ResourceManager::getFont(const std::string &name) { return fonts[name]; }
std::vector<SDL_Texture*>* ResourceManager::getAnimation(const std::string& name) {return &animations[name]; }


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
    SDL_Texture* texture = SDL_CreateTextureFromSurface(&this->renderer, surface);
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
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(&this->renderer, textSurface);
    SDL_FreeSurface(textSurface);
    textures[name] = textTexture;
    return textTexture;
}

SDL_Texture* ResourceManager::getTextTexture(const std::string& text, TTF_Font* font, SDL_Color color)
{
    SDL_Surface* textSurface = TTF_RenderText_Solid( font, text.c_str(), color );
    if (!textSurface)
        SDL_Log("Failed to render text: ", TTF_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(&this->renderer, textSurface);
    SDL_FreeSurface(textSurface);
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

bool ResourceManager::addAnimation(const std::string &path)
{
    if (animations.contains(getName(path)))
        return false;
    std::string path_to_folder = "путь_к_вашей_папке";
    int file_count = 0;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path_to_folder)) {
            file_count++;
        }
        //std::cout << "Количество файлов в папке: " << file_count << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        //std::cerr << "Ошибка: " << e.what() << std::endl;
    }

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
    std::filesystem::path pathToMusic {mainPath.string() + "resources\\Music\\"};
    std::filesystem::path pathToSounds {mainPath.string() + "resources\\Sounds\\"};
    std::filesystem::path pathToEnemies {mainPath.string() + "resources\\Images\\Enemies\\"};
    std::filesystem::path pathToCharacters {mainPath.string() + "resources\\Images\\Characters\\"};
    std::filesystem::path pathToBackgrounds {mainPath.string() + "resources\\Images\\Backgrounds\\"};

    addFont(mainPath.string() + "resources\\Fonts\\RetroByte.ttf");
    addMusic(pathToMusic.string() + "MenuTheme.mp3");
    addMusic(pathToMusic.string() + "MapTheme.mp3");
    addMusic(pathToMusic.string() + "BattleTheme.mp3");
    addSound(pathToSounds.string() + "Menu\\ButtonHover.wav");

    addTexture(pathToCharacters.string() + "MovingCharacter.png");
    addTexture(pathToCharacters.string() + "Image1.png");
    addTexture(pathToCharacters.string() + "Image2.png");
    addTexture(pathToCharacters.string() + "Image3.png");
    addTexture(pathToCharacters.string() + "Image4.png");

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
    addTexture(pathToBackgrounds.string() + "BattleImage0.png");
    addTexture(pathToBackgrounds.string() + "BattleImage1.png");
    addTexture(pathToBackgrounds.string() + "BattleImage2.png");
    addTexture(pathToBackgrounds.string() + "BattleTile.png");

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
    /*size_t enemyCount;
    in.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
    for (size_t i = 0; i < enemyCount; ++i)
    {
        EnemiesNames e;
        in.read(reinterpret_cast<char*>(&e), sizeof(e));
        player.addDiscoveredEnemy(e);
    }*/

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