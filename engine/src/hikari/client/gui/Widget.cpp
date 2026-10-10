#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/gui/Widget.hpp"

namespace hikari {
namespace gui {

    const hikari::gfx::Vector2i& Widget::getPosition() const {
        return position;
    }

    void Widget::setPosition(const hikari::gfx::Vector2i &newPosition) {
        position = newPosition;
    }
    
    const bool& Widget::isVisible() const {
        return visible;
    }

    void Widget::setVisible(const bool &isVisible) {
        visible = isVisible;
    }

} // hikari::gui
} // hikari