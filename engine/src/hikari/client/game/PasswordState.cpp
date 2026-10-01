#include "hikari/client/game/PasswordState.hpp"
#include "hikari/client/game/Input.hpp"
#include "hikari/client/audio/AudioService.hpp"
#include "hikari/client/gui/Panel.hpp"
#include "hikari/client/gui/GuiService.hpp"
#include "hikari/core/game/GameController.hpp"
#include "hikari/core/util/Log.hpp"

#include <guichan/gui.hpp>
#include <guichan/widgets/label.hpp>
#include <json/reader.h>

namespace hikari {

    PasswordState::PasswordState(const std::string &name, const Json::Value &params, GameController & controller, GuiService & guiService, AudioService & audioService, Input & keyboardInput)
        : name(name)
        , controller(controller)
        , audioService(audioService)
        , keyboardInput(keyboardInput)
        , passwordGrid(new gui::Panel())
        , guiWrapper(new gcn::Container())
        , testLabel(new gcn::Label())
        , guiService(guiService)
        , goToNextState(false)
    {
        guiWrapper->setWidth(256);
        guiWrapper->setHeight(240);
        guiWrapper->setOpaque(true);
        guiWrapper->setBaseColor(gcn::Color(12, 56, 130));

        testLabel->setCaption("Password!");
        testLabel->adjustSize();

        passwordGrid->setWidth(100);
        passwordGrid->setHeight(100);

        guiWrapper->add(testLabel.get(), 100, 4);
        guiWrapper->add(passwordGrid.get(), 10, 20);
    }

    PasswordState::~PasswordState() {

    }

    void PasswordState::handleEvent(sf::Event &event) {

    }

    void PasswordState::render(sf::RenderTarget &target) {
        guiService.renderAsTop(guiWrapper.get(), target);
    }

    bool PasswordState::update(float dt) {
        if(keyboardInput.wasPressed(Input::BUTTON_CANCEL)) {
            controller.requestStateChange(controller.getPreviousStateName());
            goToNextState = true;
        }

        return goToNextState;
    }

    void PasswordState::onEnter() {
        auto & topContainer = guiService.getRootContainer();
        topContainer.add(guiWrapper.get(), 0, 0);
        guiWrapper->setEnabled(true);

        audioService.playMusic("Password (MM3)");

        goToNextState = false;
    }

    void PasswordState::onExit() {
        auto & topContainer = guiService.getRootContainer();
        topContainer.remove(guiWrapper.get());
        guiWrapper->setEnabled(false);
        
        audioService.stopMusic();
    }

    const std::string & PasswordState::getName() const {
        return name;
    }

} // hikari