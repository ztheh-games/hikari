#ifndef HIKARI_CLIENT_GAME_WEAPONGETSTATE
#define HIKARI_CLIENT_GAME_WEAPONGETSTATE

#include "hikari/core/game/GameState.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/View.hpp>

#include <memory>
#include <queue>
#include <string>

namespace gcn {
    class Container;
    class LabelEx;
    class Icon;
}

namespace hikari {

    class AnimationSet;
    class AudioService;
    class GameConfig;
    class GameProgress;
    class GuiService;
    class ImageFont;
    class Input;
    class GameController;
    class Task;

    namespace gui {
        // Forward-declare any GUI classes here
        class Icon;
        class IconAnimator;
    }

    class WeaponGetState : public GameState {
    private:
        std::string name;
        GameController & controller;
        const GameConfig & gameConfig;
        sf::View view;
        GuiService & guiService;
        AudioService & audioService;
        GameProgress & gameProgress;
        Input & keyboardInput;
        std::queue<std::shared_ptr<Task>> taskQueue;
        bool goToNextState;

        std::unique_ptr<gcn::Container> guiContainer;
        std::unique_ptr<gcn::LabelEx> guiYouGotText;
        std::unique_ptr<gcn::LabelEx> guiWeaponGetText;
        std::unique_ptr<gui::Icon> guiBackground;
        std::unique_ptr<gui::Icon> guiRockman;

        void buildGui();

    public:
        WeaponGetState(const std::string & name, GameController & controller, const GameConfig & gameConfig, GuiService & guiService, AudioService & audioService, GameProgress & gameProgress, Input & keyboardInput);
        virtual ~WeaponGetState();

        virtual void handleEvent(sf::Event &event);
        virtual void render(sf::RenderTarget &target);
        virtual bool update(float dt);
        virtual void onEnter();
        virtual void onExit();
        virtual const std::string & getName() const;
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_WEAPONGETSTATE
