#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/TileAnimator.hpp"
#include "hikari/core/game/Animation.hpp"

namespace hikari {

    TileAnimator::TileAnimator(unsigned int tileIndex)
        : Animator()
        , tileIndex(tileIndex)
        {

    }

    TileAnimator & TileAnimator::operator=(const TileAnimator & other) {
        // Call base-class op first!
        Animator::operator = (other);

        tileIndex = other.tileIndex;
        return *this;
    }

    const int& TileAnimator::getUpdatedTileIndex() const {
        return tileIndex;
    }

    void TileAnimator::update(float delta) {
        Animator::update(delta);
    }

    void TileAnimator::update(float delta, hikari::gfx::IntRect &tileRect) {
        update(delta);

        const AnimationFrame &currentFrame = getAnimation()->getFrameAt(getCurrentFrameIndex());
        const Rectangle2D<int> &currentFrameRectangle = currentFrame.getSourceRectangle();

        tileRect.position = {
            currentFrameRectangle.getLeft(),
            currentFrameRectangle.getTop()
        };
        tileRect.size = {
            currentFrameRectangle.getWidth(),
            currentFrameRectangle.getHeight()
        };
    }

} // hikari