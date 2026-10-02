#ifndef HIKARI_CLIENT_GAME_FADE_COLOR_TASK
#define HIKARI_CLIENT_GAME_FADE_COLOR_TASK

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/client/game/BaseTask.hpp"


namespace hikari {

    /**
     * A Task which fades the color of an hikari::gfx::RectangleShape in or out. Fades to
     * or from transparent. Takes a duration after which the fade is complete and
     * the task is marked as complete.
     */
    class FadeColorTask : public BaseTask {
    public:
        enum FadeDirection {
            FADE_OUT = 0,
            FADE_IN = 1
        };

    private:
        FadeDirection direction;
        hikari::gfx::RectangleShape & rectangle;
        const float duration;
        float accumulator;

    public:
        FadeColorTask(FadeDirection direction, hikari::gfx::RectangleShape & rectangle,
            float duration);

        virtual ~FadeColorTask();

        virtual void update(float dt);
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_FADE_COLOR_TASK