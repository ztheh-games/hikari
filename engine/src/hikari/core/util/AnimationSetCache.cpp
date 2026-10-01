#include "hikari/core/util/AnimationSetCache.hpp"
#include "hikari/core/game/AnimationLoader.hpp"
#include "hikari/core/util/Log.hpp"

namespace hikari {

    AnimationSetCache::AnimationSetCache(AnimationLoader & loader)
        : loader(loader)
    {

    }

    AnimationSetCache::Resource AnimationSetCache::loadResource(const std::string &fileName) {
        HIKARI_LOG(debug) << "Caching animation set: " << fileName;

        return loader.loadSet(fileName);
    }

} // hikari