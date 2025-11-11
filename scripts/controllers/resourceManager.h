#ifndef PROJECT_NAME_RESOURCEMANAGER_H
#define PROJECT_NAME_RESOURCEMANAGER_H

#include <string>
#include <map>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include "../player.h"

class ResourceManager
{
public:
    explicit ResourceManager(SDL_Renderer& renderer) : renderer(renderer) {}
    ~ResourceManager() = default;
    
    [[nodiscard]] SDL_Texture *getTextureFromImage(SDL_Surface *surface) const;
    [[nodiscard]] SDL_Surface *getImage(const std::string& name);
    [[nodiscard]] Mix_Music *getMusic(const std::string& name);
    [[nodiscard]] Mix_Chunk *getSound(const std::string& name);
    [[nodiscard]] SDL_Texture *getTexture(const std::string& name);
    [[nodiscard]] TTF_Font *getFont(const std::string& name);
    [[nodiscard]] std::vector<SDL_Texture*>* getAnimation(const std::string& name);
    SDL_Texture* getTextTexture(const std::string& text, TTF_Font* font, SDL_Color color);

    void initialize();

    void loadPLayer(const std::string& path, Player &player);
    void savePLayer(const std::string& path, Player &player);

    bool addTexture(const std::string& path);
    bool addTextTexture(const std::string& name, const std::string& text, TTF_Font* font, SDL_Color color);
    bool addImage(const std::string& path);
    bool addSound(const std::string& path);
    bool addMusic(const std::string& path);
    bool addFont(const std::string& path);
    bool addAnimation(const std::string& path);
private:
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, Mix_Music*> music;
    std::map<std::string, Mix_Chunk*> sounds;
    std::map<std::string, SDL_Surface*> images;
    std::map<std::string, std::vector<SDL_Texture*>> animations;
    std::map<std::string, TTF_Font*> fonts;

    static bool isFilePNG(const std::string& path);
    static bool isFileJPG(const std::string& path);
    static bool isFileMP3(const std::string& path);
    static bool isFileWAV(const std::string& path);
    static bool isFileTTF(const std::string& path);
    std::string getName(const std::string& path);

    SDL_Renderer& renderer;
};


#endif //PROJECT_NAME_RESOURCEMANAGER_H