#ifndef HIKARI_CLIENT_GAME_GUITESTSTATE
#define HIKARI_CLIENT_GAME_GUITESTSTATE

#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"

#include "hikari/core/game/GameState.hpp"
#include "hikari/core/gui/ImageFont.hpp"
#include "hikari/client/gui/EnergyMeter.hpp"

#include <memory>
#include <string>



namespace hikari {

    class GuiTestState : public GameState {
    private:
        std::string name;
        hikari::gfx::Texture energyMeterImage;
        std::shared_ptr<hikari::ImageFont> font;
        std::shared_ptr<hikari::gui::EnergyMeter> energyMeter;
        float ups;
    
    public:
        GuiTestState(const std::string &name, const std::shared_ptr<hikari::ImageFont> &font);
        
        virtual ~GuiTestState() {}

        virtual void handleEvent(hikari::platform::Event &event);
        virtual void render(hikari::gfx::RenderTarget &target);
        virtual bool update(float dt);
        virtual void onEnter();
        virtual void onExit();
        virtual const std::string &getName() const;
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_GUITESTSTATE