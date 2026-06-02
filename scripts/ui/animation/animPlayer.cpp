#include "animPlayer.h"

#include "SDL_log.h"

void AnimPlayer::play(const std::string& id, bool restart)
{
    if (!animations.contains(id))
        return;

    Anim& anim = animations[id];

    if (!currentAnimationId.empty())
    {
        // ❗ loop перебивает loop сразу
        if (anim.mode == AnimPlayMode::Loop)
        {
            currentAnimationId = id;
        }
        else
        {
            animationQueue.push({id, restart});
            return;
        }
    }
    else
    {
        currentAnimationId = id;
    }

    if (restart || anim.finished)
    {
        anim.currentFrame = 0;
        anim.accumulator = 0.0f;
        anim.finished = false;
    }
}


void AnimPlayer::stopLoop()
{
    if (currentAnimationId.empty())
        return;

    if (animations[currentAnimationId].mode == AnimPlayMode::Loop)
        currentAnimationId.clear();
}


void AnimPlayer::update(float dt)
{
    if (currentAnimationId.empty())
        return;

    Anim& anim = animations[currentAnimationId];
    anim.accumulator += dt;

    while (anim.accumulator >= anim.frameTime)
    {
        anim.accumulator -= anim.frameTime;
        anim.currentFrame++;

        if (anim.currentFrame >= anim.frames.size())
        {
            if (anim.mode == AnimPlayMode::Loop)
            {
                anim.currentFrame = 0;
            }
            else // OneShot
            {
                anim.currentFrame = anim.frames.size() - 1;
                anim.finished = true;
                break;
            }
        }
    }

    if (anim.finished && anim.mode == AnimPlayMode::OneShot)
    {
        if (!animationQueue.empty())
        {
            QueuedAnim next = animationQueue.front();
            animationQueue.pop();

            currentAnimationId = next.id;
            Anim& nextAnim = animations[next.id];
            nextAnim.currentFrame = 0;
            nextAnim.accumulator = 0.0f;
            nextAnim.finished = false;
        }
        else
        {
            currentAnimationId.clear();
        }
    }
}

void AnimPlayer::draw(Visualizer& v, const SDL_Rect& screen) const
{
    if (currentAnimationId.empty()) return;

    auto it = animations.find(currentAnimationId);
    if (it == animations.end()) return;

    const Anim& anim = it->second;
    if (anim.frames.empty()) return;
    if (anim.currentFrame >= anim.frames.size()) return;

    SDL_Texture* frame = anim.frames[anim.currentFrame];
    if (!frame) return;

    SDL_Rect rect;
    if (!anim.rects.empty() && anim.currentFrame < anim.rects.size())
    {
        // Анимация создана с vector<SDL_Rect> — используем напрямую
        rect = anim.rects[anim.currentFrame];
    }
    else
    {
        // Анимация создана с frameRect — центрируем на экране
        rect = {
            screen.w / 2 - anim.frameRect.w / 2,
            screen.h / 2 - anim.frameRect.h / 2,
            anim.frameRect.w,
            anim.frameRect.h
        };
    }

    v.drawTexture(frame, rect.x, rect.y, rect.w, rect.h);
}

void AnimPlayer::draw(Visualizer& v) const
{
    // Вызываем с пустым screen — для анимаций без frameRect
    draw(v, {0, 0, 0, 0});
}