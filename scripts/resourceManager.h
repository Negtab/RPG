#ifndef PROJECT_NAME_RESOURCEMANAGER_H
#define PROJECT_NAME_RESOURCEMANAGER_H

#include <string>
#include <map>
#include <memory>
#include <SDL.h>

class resourceManager
{
public:
    resourceManager() = default;
    ~resourceManager() = default;

    SDL_Texture::Texture& getTexture(const std::string& path);
    Mix_Music::Music& getMusic(const std::string& path);

private:
    std::map<std::string, std::unique_ptr<SDL>> textures;
    std::map<std::string, std::unique_ptr<sf::Music>> musics;
};


#endif //PROJECT_NAME_RESOURCEMANAGER_H