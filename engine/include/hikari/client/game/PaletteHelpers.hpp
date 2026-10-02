#ifndef HIKARI_CLIENT_GAME_PALETTEHELPERS
#define HIKARI_CLIENT_GAME_PALETTEHELPERS

#include "hikari/core/graphics/Graphics.hpp"

#include <string>
#include <vector>

namespace hikari {

namespace PaletteHelpers {

    /**
     * Loads a palette from the specified file and returns it as a vector of
     * palette entries.
     */
    std::vector<std::vector<hikari::gfx::Color>> loadPaletteFile(const std::string & filePath);

} // hikari::PaletteHelpers
} // hikari

#endif // HIKARI_CLIENT_GAME_PALETTEHELPERS