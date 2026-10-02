#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/SpriteAnimator.hpp"
#include "hikari/core/game/Animation.hpp"

namespace hikari {

    SpriteAnimator::SpriteAnimator(hikari::gfx::Sprite &sprite)
        : Animator()
        , invertXOffset(false)
        , invertYOffset(false)
        , sprite(sprite)
        , sourceRectangle(sprite.getTextureRect()) {

    }

    SpriteAnimator::~SpriteAnimator() {

    }

    void SpriteAnimator::setInvertXOffset(const bool flip) {
        this->invertXOffset = flip;
    }

    void SpriteAnimator::setInvertYOffset(const bool flip) {
        this->invertYOffset = flip;
    }

    void SpriteAnimator::update(float delta) {
        Animator::update(delta);

        if(const auto& animation = getAnimation()) {
            const auto& currentFrame = animation->getFrameAt(getCurrentFrameIndex());
            const auto& currentFrameRectangle = currentFrame.getSourceRectangle();

            sourceRectangle.position = {
                currentFrameRectangle.getLeft(),
                currentFrameRectangle.getTop()
            };
            sourceRectangle.size = {
                currentFrameRectangle.getWidth(),
                currentFrameRectangle.getHeight()
            };

            sprite.setTextureRect(sourceRectangle);
        
            sprite.setOrigin({
                static_cast<float>(currentFrame.getHotspot().getX()),
                static_cast<float>(currentFrame.getHotspot().getY())
            });

            if(invertXOffset) {
                sprite.setOrigin({
                    static_cast<float>(currentFrame.getSourceRectangle().getWidth() - currentFrame.getHotspot().getX()),
                    static_cast<float>(sprite.getOrigin().y)
                });
            } 

            if(invertYOffset) {
                sprite.setOrigin({
                    static_cast<float>(sprite.getOrigin().x),
                    static_cast<float>(currentFrame.getSourceRectangle().getHeight() - currentFrame.getHotspot().getY())
                });
            }
        }
    }

} // hikari