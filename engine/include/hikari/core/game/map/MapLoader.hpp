#ifndef HIKARI_CORE_GAME_MAP_MAPLOADER
#define HIKARI_CORE_GAME_MAP_MAPLOADER

#include "hikari/core/Platform.hpp"
#include <memory>
#include <string>

namespace hikari {

    class AnimationSetCache;
    class TilesetCache;
    class Map;
    typedef std::shared_ptr<Map> MapPtr;

    class HIKARI_API MapLoader {
        AnimationSetCache & animationSetCache;
        TilesetCache & tilesetCache;

    public:
        MapLoader(AnimationSetCache & animationSetCache, TilesetCache & tilesetCache);
        virtual ~MapLoader();
        MapPtr load(const std::string &fileName) const;
    };

}

#endif
