#ifndef PROJECT_NAME_RESOURCEMANAGER_H
#define PROJECT_NAME_RESOURCEMANAGER_H

#include <string>
#include <map>
#include <SDL.h>
#include <SDL_mixer.h>

#include "player.h"

class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    [[nodiscard]] SDL_Texture *getTextureFromImage(SDL_Renderer *renderer, SDL_Surface *surface) const;
    [[nodiscard]] SDL_Surface *getImage(const std::string& name);
    [[nodiscard]] Mix_Music *getMusic(const std::string& name);
    [[nodiscard]] Mix_Chunk *getSound(const std::string& name);
    [[nodiscard]] SDL_Texture *getTexture(const std::string& name);
    void initialize();

    void loadPLayer(const std::string& path, Player &player);
    void savePLayer(const std::string& path, Player &player);

    bool addTexture(const std::string& path, SDL_Renderer* renderer);
    bool addImage(const std::string& path);
    bool addSound(const std::string& path);
    bool addMusic(const std::string& path);

private:
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, Mix_Music*> music;
    std::map<std::string, Mix_Chunk*> sounds;
    std::map<std::string, SDL_Surface*> images;

    bool isFilePNG(const std::string& path);
    bool isFileJPG(const std::string& path);
    bool isFileMP3(const std::string& path);
    bool isFileWAV(const std::string& path);
    std::string getName(const std::string& path);
};


#endif //PROJECT_NAME_RESOURCEMANAGER_H