#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/StateTransition.hpp"



namespace hikari {

    class SliceStateTransition : public StateTransition {
    public:
        enum SliceDirection {
            SLICE_LEFT = 0,
            SLICE_RIGHT = 1
        };

    private:
        SliceDirection direction;
        const float duration;
        float accumulator;
        hikari::gfx::RectangleShape overlay;
        hikari::gfx::RenderTexture exitingStateTexture;
        hikari::gfx::RenderTexture enteringStateTexture;
        hikari::gfx::Sprite exitingStateSpriteLayer;
        hikari::gfx::Sprite enteringStateSpriteLayer;

    public:
        SliceStateTransition(SliceDirection direction, float duration);
        virtual ~SliceStateTransition();

        virtual void render(hikari::gfx::RenderTarget &target);
        virtual void update(float dt);
    };

} // hikari
