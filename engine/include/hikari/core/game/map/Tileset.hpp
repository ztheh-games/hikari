#ifndef HIKARI_CORE_GAME_MAP_TILESET
#define HIKARI_CORE_GAME_MAP_TILESET

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include "hikari/core/game/TileAnimator.hpp"


#include <memory>

#include <cstddef>
#include <string>
#include <vector>

#if (_WIN32 && _MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif

namespace hikari {

    namespace TileAttribute {
        enum TileAttribute {
            NO_ATTRIBUTES = 0,
            SOLID = (1 << 0),
            LADDER = (1 << 1),
            PLATFORM = (1 << 2),
            SPIKE = (1 << 3),
            ABYSS = (1 << 4),
            FLIP_HORIZONTAL = (1 << 5),
            FLIP_VERTICAL = (1 << 6),
            ROTATE_BY_90 = (1 << 7),
            WATER = (1 << 8)
        };

        bool hasAttribute(const int &tile, const TileAttribute &attr);
    }

    class HIKARI_API Tileset {
    private:
        size_t tileSize;
        std::vector<hikari::gfx::IntRect> tiles;
        std::vector<TileAnimator> tileAnimators;
        std::shared_ptr<hikari::gfx::Texture> texture;
    public:
        Tileset(
            const std::shared_ptr<hikari::gfx::Texture> &texture,
            const size_t& tileSize,
            const std::vector<hikari::gfx::IntRect> &tiles,
            const std::vector<TileAnimator> &tileAnimators
        );
        const size_t& getTileSize() const;
        const hikari::gfx::IntRect& getTileRect(const unsigned int &index) const;
        const std::shared_ptr<hikari::gfx::Texture> & getTexture() const;
        void update(float delta);
    };

} // hikari

#if (_WIN32 && _MSC_VER)
    #pragma warning(pop)
#endif

#endif // HIKARI_CORE_GAME_MAP_TILESET
