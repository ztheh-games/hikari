#ifndef HIKARI_CORE_UTIL_SFMLRESOURCES
#define HIKARI_CORE_UTIL_SFMLRESOURCES

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace hikari {
namespace SfmlResources {

    inline const sf::Texture & placeholderTexture() {
        static const sf::Texture texture;
        return texture;
    }

    class DefaultSprite : public sf::Sprite {
    public:
        DefaultSprite()
            : sf::Sprite(placeholderTexture()) {
        }

        explicit DefaultSprite(const sf::Texture & texture)
            : sf::Sprite(texture) {
        }

        DefaultSprite(const sf::Sprite & sprite)
            : sf::Sprite(sprite) {
        }
    };

} // namespace SfmlResources
} // namespace hikari

#endif // HIKARI_CORE_UTIL_SFMLRESOURCES
