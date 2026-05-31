//
// Created by User on 31.05.2026.
//

#ifndef PROJECT_NAME_ANIMPLAYER_H
#define PROJECT_NAME_ANIMPLAYER_H

#include <map>
#include <queue>
#include <string>
#include <vector>
#include <SDL_render.h>

#include "../controllers/visualizer.h"

enum class AnimPlayMode
{
    OneShot,
    Loop
};

class AnimPlayer
{
    struct QueuedAnim {
        std::string id;
        bool restart;
    };

public:
    struct Anim
    {
        float frameTime;
        std::vector<SDL_Texture*> frames;
        std::vector<SDL_Rect> rects;
        SDL_Rect frameRect;

        size_t currentFrame = 0;
        float accumulator = 0.0f;
        bool finished = false;

        AnimPlayMode mode = AnimPlayMode::OneShot;
    };

    std::string currentAnimationId;
    std::map<std::string, Anim> animations;
    std::queue<QueuedAnim> animationQueue;

    void play(const std::string& id, bool restart = false);
    void stopLoop();
    void update(float deltaTime);
    void draw(Visualizer& v);
    void draw(Visualizer& v, const SDL_Rect& screen);
};


#endif //PROJECT_NAME_ANIMPLAYER_H