#ifndef HIKARI_CORE_GUI_IMAGEFONT
#define HIKARI_CORE_GUI_IMAGEFONT

#include "hikari/core/graphics/Graphics.hpp"

#include "hikari/core/Platform.hpp"




#include <memory>
#include <map>
#include <string>
namespace hikari {

    class HIKARI_API ImageFont {
    private:
        int glyphWidth;
        int glyphHeight;
        std::shared_ptr<hikari::gfx::Texture> glyphTexture;
        gfx::Sprite glyphSprite;
        std::string glyphs;
        std::map< char, hikari::gfx::IntRect > glyphMap;
    public:
        ImageFont(const std::shared_ptr<hikari::gfx::Texture> &glyphTexture, const std::string &glyphs, const int &glyphWidth, const int &glyphHeight);
        virtual ~ImageFont();
        
        const int& getGlyphWidth() const;
        const int& getGlyphHeight() const;

        /**
            Renders a string to an hikari::gfx::RenderTarget at a specified location
            optionally with a color. The color is applied as a filter to the
            source image.

            The newline character (\n) is supported and will reset the cursor
            to the position specified by the parameter x. The next line begins
            at the number of pixels specified by the glyph's height when the
            font was constructed.

            @param target the target to render to
            @param glyphs the text to be rendered
            @param x the x-coordinate where the text should be rendered
            @param y the y-coordinate where the text should be rendered
            @param color a color filter to apply to the text, transparent by default
        */
        void renderText(hikari::gfx::RenderTarget &target, const std::string &glyphs, const int &x, const int &y, const hikari::gfx::Color &color = hikari::gfx::Color::White);
    };

} // hikari

#endif // HIKARI_CORE_GUI_IMAGEFONT