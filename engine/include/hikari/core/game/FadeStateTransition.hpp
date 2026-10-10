#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/StateTransition.hpp"
#include "hikari/client/game/FadeColorTask.hpp"


namespace hikari {

    class FadeStateTransition : public StateTransition {
    public:
        enum FadeDirection {
            FADE_OUT = 0,
            FADE_IN = 1
        };

    private:
        FadeDirection direction;
        hikari::gfx::RectangleShape overlay;
        FadeColorTask fadeTask;

    public:
        FadeStateTransition(FadeDirection direction, hikari::gfx::Color color, float duration);
        virtual ~FadeStateTransition();

        virtual void render(hikari::gfx::RenderTarget &target);
        virtual void update(float dt);
    };

} // hikari
