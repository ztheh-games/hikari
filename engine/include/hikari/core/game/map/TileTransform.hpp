#ifndef HIKARI_CORE_GAME_MAP_TILETRANSFORM
#define HIKARI_CORE_GAME_MAP_TILETRANSFORM

#include <cstdint>

namespace hikari {

    enum class TileTransform : std::uint8_t {
        None = 0,
        Horizontal = 1,
        Vertical = 2,
        Diagonal = 4
    };

    struct TileTransformComponents {
        int scaleX;
        int scaleY;
        int rotation;
    };

    inline TileTransformComponents decomposeTileTransform(TileTransform transform) {
        const auto flags = static_cast<std::uint8_t>(transform);
        const int horizontal = flags & 1 ? -1 : 1;
        const int vertical = flags & 2 ? -1 : 1;
        // Tiled swaps axes first, then reflects across the horizontal/vertical axes.
        return flags & 4
            ? TileTransformComponents{vertical, -horizontal, 90}
            : TileTransformComponents{horizontal, vertical, 0};
    }

}

#endif
