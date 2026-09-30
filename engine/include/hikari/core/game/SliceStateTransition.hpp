#include "hikari/core/game/StateTransition.hpp"
#include "hikari/core/util/SfmlResources.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <memory>

namespace hikari {

    class SliceStateTransition : public StateTransition {
    public:
        enum SliceDirection {
            SLICE_LEFT = 0,
            SLICE_RIGHT = 1
        };

    private:
        static std::unique_ptr<sf::RenderTexture> exitingStateTexture;
        static std::unique_ptr<sf::RenderTexture> enteringStateTexture;

        SliceDirection direction;
        const float duration;
        float accumulator;
        SfmlResources::DefaultSprite exitingStateSpriteLayerTop;
        SfmlResources::DefaultSprite exitingStateSpriteLayerMiddle;
        SfmlResources::DefaultSprite exitingStateSpriteLayerBottom;
        SfmlResources::DefaultSprite enteringStateSpriteLayer;

    public:
        SliceStateTransition(SliceDirection direction, float duration);
        virtual ~SliceStateTransition();

        static void createSharedTextures();
        static void destroySharedTextures();

        virtual void render(sf::RenderTarget &target);
        virtual void update(float dt);
    };

} // hikari
