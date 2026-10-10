#ifndef HIKARI_CORE_GAME_MAP_TILESETLOADER
#define HIKARI_CORE_GAME_MAP_TILESETLOADER

#include "hikari/core/Platform.hpp"
#include <memory>
#include <string>

namespace hikari {

    class ImageCache;
    class Tileset;
    typedef std::shared_ptr<Tileset> TileDataPtr;

    class HIKARI_API TilesetLoader {
        ImageCache & imageCache;
    public:
        explicit TilesetLoader(ImageCache & imageCache);
        TileDataPtr load(const std::string &fileName);
    };

}

#endif
