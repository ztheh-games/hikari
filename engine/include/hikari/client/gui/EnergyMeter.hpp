#ifndef HIKARI_CORE_GUI_ENERGYMETER
#define HIKARI_CORE_GUI_ENERGYMETER

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/client/gui/Widget.hpp"





namespace hikari {
namespace gui {

    class EnergyMeter : public Widget {
    private:
        static const float HORIZONTAL_ROTATION_ANGLE;
        static const float VERTICAL_ROTATION_ANGLE;
        static const float HIGHLIGHT_OFFSET_X;

        float value;
        float maximumValue;
        int orientation;

        gfx::Sprite overlay;
        hikari::gfx::RectangleShape primaryBackground;
        hikari::gfx::RectangleShape secondaryBackground;
        hikari::gfx::RectangleShape foreground;
        hikari::gfx::Color primaryColor;
        hikari::gfx::Color secondaryColor;
        hikari::gfx::Color fillColor;

        void updateFill();
        void updateOrientation();

    public:
        static const int HORIZONTAL_ORIENTATION;
        static const int VERTICAL_ORIENTATION;

        static const hikari::gfx::Color DEFAULT_FILL_COLOR;
        static const hikari::gfx::Color DEFAULT_PRIMARY_COLOR;
        static const hikari::gfx::Color DEFAULT_SECONDARY_COLOR;

        EnergyMeter(const hikari::gfx::Sprite &overlay, const float &maximumValue);
        virtual ~EnergyMeter() {}

        void setOrientation(const int &orientation);
        virtual void setPosition(const hikari::gfx::Vector2i &newPosition);

        const float& getValue() const;
        const float& getMaximumValue() const;
        const hikari::gfx::Color& getFillColor() const;
        const hikari::gfx::Color& getPrimaryColor() const;
        const hikari::gfx::Color& getSecondaryColor() const;
        const int& getOrientation() const;

        void setValue(const float &newValue);
        void setMaximumValue(const float &newValue);
        void setFillColor(const hikari::gfx::Color &newColor);
        void setPrimaryColor(const hikari::gfx::Color &newColor);
        void setSecondaryColor(const hikari::gfx::Color &newColor);

        virtual void render(hikari::gfx::RenderTarget &target);
        virtual void update(const float &delta);
    };

} // hikari::gui
} // hikari

#endif // HIKARI_CORE_GUI_WIDGET