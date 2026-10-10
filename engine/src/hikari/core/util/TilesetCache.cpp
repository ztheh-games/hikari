#include "hikari/core/util/TilesetCache.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include "hikari/core/util/Log.hpp"

namespace hikari {

    TilesetCache::TilesetCache(TilesetLoader &loader)
        : loader(loader) 
    {

    }

    TilesetCache::Resource TilesetCache::loadResource(const std::string &fileName) {
        HIKARI_LOG(debug) << "Caching tileset: " << fileName;

        return loader.load(fileName);
    }

}