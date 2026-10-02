#ifndef HIKARI_CORE_GUI_BOSSMENU
#define HIKARI_CORE_GUI_BOSSMENU

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/client/gui/Widget.hpp"



#include <vector>

namespace hikari {

    class BossMenu : public Widget {
    private:
        int selectedIndex;
        std::vector<bool> validIndicies;
        gfx::Sprite background;
        gfx::Sprite frame;
    };

} // hikari

#endif // HIKARI_CORE_GUI_BOSSMENU