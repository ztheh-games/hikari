#ifndef HIKARI_CLIENT_GAME_PASSWORDSTATE
#define HIKARI_CLIENT_GAME_PASSWORDSTATE

#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"

#include "hikari/core/game/GameState.hpp"


#include <memory>
#include <string>

namespace gcn {
    class Container;
    class Label;
}

namespace Json {
    class Value;
}

namespace hikari {

    class GuiService;
    class Input;
    class AudioService;
    
    namespace gui {
        class Panel;
    }

    class GameController;

    class PasswordState : public GameState {
    private:
        std::string name;
        GameController & controller;
        AudioService & audioService;
        Input & keyboardInput;
        std::unique_ptr<gui::Panel> passwordGrid;
        std::unique_ptr<gcn::Container> guiWrapper;
        std::unique_ptr<gcn::Label> testLabel;
        GuiService & guiService;

        bool goToNextState;

    public:
        PasswordState(const std::string &name, const Json::Value &params, GameController & controller, GuiService & guiService, AudioService & audioService, Input & keyboardInput);
        virtual ~PasswordState();

        virtual void handleEvent(hikari::platform::Event &event);
        virtual void render(hikari::gfx::RenderTarget &target);
        virtual bool update(float dt);
        virtual void onEnter();
        virtual void onExit();
        virtual const std::string &getName() const;
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_PASSWORDSTATE