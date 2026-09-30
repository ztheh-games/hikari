#ifndef HIKARI_CORE_GUI_BOSSMENU
#define HIKARI_CORE_GUI_BOSSMENU

#include "hikari/client/gui/Widget.hpp"
#include "hikari/core/util/SfmlResources.hpp"
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <vector>

namespace hikari {

    class BossMenu : public Widget {
    private:
        int selectedIndex;
        std::vector<bool> validIndicies;
        SfmlResources::DefaultSprite background;
        SfmlResources::DefaultSprite frame;
    };

} // hikari

#endif // HIKARI_CORE_GUI_BOSSMENU