#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/StateTransition.hpp"





#include <memory>

namespace hikari {

    class SliceStateTransition : public StateTransition {
    public:
        enum SliceDirection {
            SLICE_LEFT = 0,
            SLICE_RIGHT = 1
        };

    private:
        static std::unique_ptr<hikari::gfx::RenderTexture> exitingStateTexture;
        static std::unique_ptr<hikari::gfx::RenderTexture> enteringStateTexture;

        SliceDirection direction;
        const float duration;
        float accumulator;
        gfx::Sprite exitingStateSpriteLayerTop;
        gfx::Sprite exitingStateSpriteLayerMiddle;
        gfx::Sprite exitingStateSpriteLayerBottom;
        gfx::Sprite enteringStateSpriteLayer;

    public:
        SliceStateTransition(SliceDirection direction, float duration);
        virtual ~SliceStateTransition();

        static void createSharedTextures();
        static void destroySharedTextures();

        virtual void render(hikari::gfx::RenderTarget &target);
        virtual void update(float dt);
    };

} // hikari
