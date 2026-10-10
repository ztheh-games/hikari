#ifndef HIKARI_CORE_UTIL_IMAGECACHE
#define HIKARI_CORE_UTIL_IMAGECACHE

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include "hikari/core/util/ResourceCache.hpp"


namespace hikari {

    class HIKARI_API ImageCache : public ResourceCache<hikari::gfx::Texture> {
    public:
        static const bool USE_SMOOTHING;
        static const bool NO_SMOOTHING;
        static const bool USE_MASKING;
        static const bool NO_MASKING;

    private:
        bool enableSmoothing;
        bool enableMask;
        hikari::gfx::Color maskColor;

    protected:
        virtual ImageCache::Resource loadResource(const std::string &fileName);

    public:
        ImageCache(bool smoothing, bool masking, const hikari::gfx::Color &mask = hikari::gfx::Color(255, 0, 255));

        virtual ~ImageCache() { }
    };

} // hikari

#endif // HIKARI_CORE_UTIL_IMAGECACHE