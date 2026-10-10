#ifndef HIKARI_CORE_GAME_TILEANIMATOR
#define HIKARI_CORE_GAME_TILEANIMATOR

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include "hikari/core/game/Animator.hpp"

#include <vector>

namespace hikari {

    class HIKARI_API TileAnimator : public Animator {
    private:
        int tileIndex;
        std::vector<hikari::gfx::IntRect> &tiles;

    public:
        TileAnimator(std::vector<hikari::gfx::IntRect> &tiles, unsigned int tileIndex);
        TileAnimator& operator=(const TileAnimator & other);
        const int& getUpdatedTileIndex() const;
        virtual ~TileAnimator() { }
        virtual void update(float delta);
        void update(float delta, hikari::gfx::IntRect &tileRect);
    };

} // hikari

#endif // HIKARI_CORE_GAME_TILEANIMATOR