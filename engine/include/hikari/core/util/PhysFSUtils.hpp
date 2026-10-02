#ifndef HIKARI_CORE_UTIL_PHYSFSUTILS
#define HIKARI_CORE_UTIL_PHYSFSUTILS

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"
#include <json/value.h>
#include <string>



namespace hikari {

    class HIKARI_API PhysFSUtils {
    public:
        /**
         * Loads an engine image using PhysFS.
         * @param fileName  The PhysFS-style file path or name of the image to load.
         * @param image     The engine image receiving the decoded data.
         */
        static bool loadImage(const std::string &fileName, hikari::gfx::Texture &texture);

        static Json::Value loadJson(const std::string &fileName);

        /**
         * Opens a file and reads its contents into a string, returning the string.
         */
        static const std::string readFileAsString(const std::string &fileName);
    };

} // hikari

#endif // HIKARI_CORE_UTIL_PHYSFSUTILS