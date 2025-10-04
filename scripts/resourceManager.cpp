#include "resourceManager.h"
#include <filesystem>
#include <fstream>
#include <SDL_image.h>

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
    return std::filesystem::path(path).filename().string();
}

void ResourceManager::initialize()
{
    std::filesystem::path mainPath {std::filesystem::current_path().remove_filename()};
    //addImage(mainPath.string() + "/resources/Images/Characters/");
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