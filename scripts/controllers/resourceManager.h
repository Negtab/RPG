#ifndef PROJECT_NAME_RESOURCEMANAGER_H
#define PROJECT_NAME_RESOURCEMANAGER_H

#include <string>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <unordered_map>

#include "../main/player.h"

struct TextBlock {
    SDL_Texture* texture = nullptr;
    int w = 0, h = 0;
};

struct TextSize {
    int w, h;
};

class ResourceManager
{
public:
    explicit ResourceManager(SDL_Renderer& newRenderer) {renderer = &newRenderer;}
    ~ResourceManager() = default;
    
    [[nodiscard]] static SDL_Texture *getTextureFromImage(SDL_Surface *surface);
    [[nodiscard]] static SDL_Surface *getImage(const std::string& name);
    [[nodiscard]] static Mix_Music *getMusic(const std::string& name);
    [[nodiscard]] static Mix_Chunk *getSound(const std::string& name);
    [[nodiscard]] static SDL_Texture *getTexture(const std::string& name);
    [[nodiscard]] static TTF_Font *getFont(const std::string& name);
    [[nodiscard]] static std::vector<SDL_Texture*>& getAnimation(const std::string& name);
    static SDL_Texture* getTextTexture(const std::string& text, TTF_Font* font, SDL_Color color, uint32_t wrapWidth = 0);

    void initialize();

    [[nodiscard]] bool isPlayerNew(const std::string& path) const;
    void savePlayer(const std::string &path, const Player &player);
    void loadPlayer(const std::string &path, Player &player);

    bool addTexture(const std::string& path);
    bool addTextTexture(const std::string& name, const std::string& text, TTF_Font* font, SDL_Color color);
    bool addImage(const std::string& path);
    bool addSound(const std::string& path);
    bool addMusic(const std::string& path);
    bool addFont(const std::string& path);
    bool addAnimation(const std::string& path);

    static TextSize measureText(const std::string& text, TTF_Font* font);
    static TextBlock renderMultiline(SDL_Renderer* renderer, const std::string& text, TTF_Font* font, SDL_Color color, int maxWidth = 0);
private:
    static std::unordered_map<std::string, SDL_Texture*> textCache;
    static std::unordered_map<std::string, SDL_Texture*> textures;
    static std::unordered_map<std::string, Mix_Music*> music;
    static std::unordered_map<std::string, Mix_Chunk*> sounds;
    static std::unordered_map<std::string, SDL_Surface*> images;
    static std::unordered_map<std::string, std::vector<SDL_Texture*>> animations;
    static std::unordered_map<std::string, TTF_Font*> fonts;

    static bool isFilePNG(const std::string& path);
    static bool isFileJPG(const std::string& path);
    static bool isFileMP3(const std::string& path);
    static bool isFileWAV(const std::string& path);
    static bool isFileTTF(const std::string& path);
    std::string getName(const std::string& path);

    static SDL_Renderer* renderer;
};


#endif //PROJECT_NAME_RESOURCEMANAGER_H