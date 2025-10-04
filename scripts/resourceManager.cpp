#include "resourceManager.h"
#include <filesystem>
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