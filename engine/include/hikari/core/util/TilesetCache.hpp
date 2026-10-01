#ifndef HIKARI_CORE_UTIL_TILESETCACHE
#define HIKARI_CORE_UTIL_TILESETCACHE

#include "hikari/core/Platform.hpp"
#include "hikari/core/game/map/Tileset.hpp"
#include "hikari/core/game/map/TilesetLoader.hpp"
#include "hikari/core/util/ResourceCache.hpp"

namespace hikari {

    class HIKARI_API TilesetCache : public ResourceCache<Tileset> {
    private:
        TilesetLoader & loader;

    protected:
        virtual TilesetCache::Resource loadResource(const std::string &fileName);

    public:
        TilesetCache(TilesetLoader &loader);

        virtual ~TilesetCache() { }
    };

} // hikari

#endif // HIKARI_CORE_UTIL_TILESETCACHE