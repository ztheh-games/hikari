#ifndef HIKARI_CORE_GUI_WIDGET
#define HIKARI_CORE_GUI_WIDGET

#include "hikari/core/graphics/Graphics.hpp"

#include <memory>

namespace hikari {
namespace gui {

    class Widget {
    private:
        int id;
        bool visible;
        hikari::gfx::Vector2i position;

        std::weak_ptr<Widget> parent;
    public:
        virtual ~Widget() {}

        const std::weak_ptr<Widget> & getParent() const;
        void setParent(const std::weak_ptr<Widget> & newParent);

        virtual const hikari::gfx::Vector2i& getPosition() const;
        virtual void setPosition(const hikari::gfx::Vector2i &newPosition);

        const bool& isVisible() const;
        void setVisible(const bool &isVisible);

        virtual void render(hikari::gfx::RenderTarget &target) = 0;
        virtual void update(const float &delta) = 0;
    };

    typedef std::shared_ptr<Widget> WidgetPtr;

} // hikari::gui
} // hikari

#endif // HIKARI_CORE_GUI_WIDGET