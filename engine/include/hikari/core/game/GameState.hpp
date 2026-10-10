#ifndef HIKARI_CORE_GAME_GAMESTATE
#define HIKARI_CORE_GAME_GAMESTATE

#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"

#include "hikari/core/Platform.hpp"
#include <string>



namespace hikari {

    class HIKARI_API GameState {
    public:
        virtual ~GameState() {}

        virtual void handleEvent(hikari::platform::Event &event) = 0;
        virtual void render(hikari::gfx::RenderTarget &target) = 0;

        /**
            Updates this state's logic, given that dt milliseconds have elapsed
            since it's last update. 

            @return false if this state should keep running, true if it is time
            to transition to a new state
        */
        virtual bool update(float dt) = 0;
        virtual void onEnter() = 0;
        virtual void onExit() = 0;
        virtual const std::string &getName() const = 0;
    };

} // hikari

#endif // HIKARI_CORE_GAME_GAMESTATE