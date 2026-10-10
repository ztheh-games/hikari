#ifndef HIKARI_CLIENT_GAME_ENDINGSTATE
#define HIKARI_CLIENT_GAME_ENDINGSTATE

#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"

#include "hikari/core/game/GameState.hpp"


#include <json/value.h>
#include <string>

namespace hikari {
namespace client {
namespace game {

    class EndingState : public hikari::core::game::GameState {
    private:
        std::string name;
        hikari::gfx::RenderTarget &target;
        hikari::gfx::View view;

    public:
        EndingState(const Json::Value &params);
        virtual ~EndingState() {}

        virtual void handleEvent(hikari::platform::Event &event);
        virtual void render(hikari::gfx::RenderTarget &target);
        virtual bool update(const float &dt);
        virtual void onEnter();
        virtual void onExit();
        virtual const std::string &getName() const;
    };

} // hikari.client.game
} // hikari.client
} // hikari

#endif // HIKARI_CLIENT_GAME_ENDINGSTATE