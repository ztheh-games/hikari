#ifndef HIKARI_CORE_GAME_MAP_TILESETLOADER
#define HIKARI_CORE_GAME_MAP_TILESETLOADER

#include "hikari/core/Platform.hpp"
#include <memory>
#include <json/value.h>
#include <string>

#if (_WIN32 && _MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif

namespace hikari {

    class AnimationLoader;
    class ImageCache;
    class Tileset;
    typedef std::shared_ptr<Tileset> TileDataPtr;

    class HIKARI_API TilesetLoader {
    private:
        static const char* PROPERTY_NAME_SURFACE;
        static const char* PROPERTY_NAME_SIZE;
        static const char* PROPERTY_NAME_TILES;
        static const char* PROPERTY_NAME_X;
        static const char* PROPERTY_NAME_Y;
        static const char* PROPERTY_NAME_ANIMATION;
        static const char* PROPERTY_NAME_VERSION;

        AnimationLoader & animationLoader;
        ImageCache & imageCache;

        bool isValidTilesetJson(const Json::Value &json) const;
        bool isValidTileJson(const Json::Value &json) const;
        bool isTileAnimated(const Json::Value &json) const;

        TileDataPtr constructTileset(const Json::Value &json) const;

    public:
        TilesetLoader(ImageCache &imageCache,
                AnimationLoader &animationLoader);
        TileDataPtr loadFromJson(const Json::Value &json);
    };

} // hikari

#if (_WIN32 && _MSC_VER)
    #pragma warning(pop)
#endif

#endif // HIKARI_CORE_GAME_MAP_TILESETLOADER
