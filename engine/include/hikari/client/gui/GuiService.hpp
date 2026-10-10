#ifndef HIKARI_CLIENT_GUI_GUISERVICE
#define HIKARI_CLIENT_GUI_GUISERVICE

#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"

#include "hikari/core/util/NonCopyable.hpp"

#include <unordered_map>
#include <memory>
#include <string>

namespace gcn {
    class Graphics;
    class Container;
    class Image;
    class ImageLoader;
    class PlatformInput;
    class GpuGraphics;
    class FixedImageFont;
    class Font;
    class Gui;
    class Widget;
}

namespace Json {
    class Value;
}



namespace hikari {

    class ImageCache;

    class GuiService : public NonCopyable {
    public:
        static const std::string DEFAULT_FONT_NAME;
        
    private:
        class GlobalRegistration;
        hikari::gfx::RenderTarget & renderTarget;
        std::unique_ptr<gcn::Gui> gui; 
        std::unique_ptr<gcn::GpuGraphics> graphics;
        std::unique_ptr<gcn::PlatformInput> input;
        std::unique_ptr<gcn::ImageLoader> imageLoader;
        std::unique_ptr<gcn::Container> rootWidget;
        std::unique_ptr<gcn::Container> rootContainer;
        std::unique_ptr<gcn::Container> hudContainer;
        std::unordered_map<std::string, std::shared_ptr<gcn::Image>> fontImageMap;
        std::unordered_map<std::string, std::shared_ptr<gcn::Font>> fontMap;
        std::unique_ptr<GlobalRegistration> globals;

        void buildFontMap(const Json::Value & fontConfig);

    public:
        explicit GuiService(const Json::Value & config, ImageCache & imageCache, hikari::gfx::RenderTarget & renderTarget);
        virtual ~GuiService();

        void processEvent(hikari::platform::Event & evt);

        gcn::Gui & getGui();
        gcn::Container & getHudContainer();
        gcn::Container & getRootContainer();

        std::shared_ptr<gcn::Font> getFontByName(const std::string & fontName) const;

        void renderRootContainer();
        void renderRootContainer(hikari::gfx::RenderTarget & target);
        void renderAsTop(gcn::Widget * widget, hikari::gfx::RenderTarget & target);
        void renderHudContainer();
        void renderHudContainer(gfx::RenderTarget & target);
    };

} // hikari

#endif // HIKARI_CLIENT_GUI_GUISERVICE
