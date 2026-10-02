#ifndef HIKARI_CORE_GAME_SPRITEANIMATOR
#define HIKARI_CORE_GAME_SPRITEANIMATOR

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include "hikari/core/game/Animator.hpp"

namespace hikari {

    class HIKARI_API SpriteAnimator : public Animator {
    private:
        bool invertXOffset;
        bool invertYOffset;
        hikari::gfx::Sprite &sprite;
        hikari::gfx::IntRect sourceRectangle;

    public:
        SpriteAnimator(hikari::gfx::Sprite &sprite);
        virtual ~SpriteAnimator();
        void setInvertXOffset(const bool flip);
        void setInvertYOffset(const bool flip);
        virtual void update(float delta);
    };

} // hikari

#endif // HIKARI_CORE_GAME_SPRITEANIMATOR