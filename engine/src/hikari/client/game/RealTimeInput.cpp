#include "hikari/client/platform/Events.hpp"
#include "hikari/client/game/RealTimeInput.hpp"

namespace hikari {

    RealTimeInput::RealTimeInput()
        : currentState()
        , previousState()
        , keybindings()
    {
        // Set up defaut key bindings
        bindKey(Input::BUTTON_UP,    hikari::platform::Keyboard::Key::Up);
        bindKey(Input::BUTTON_RIGHT, hikari::platform::Keyboard::Key::Right);
        bindKey(Input::BUTTON_DOWN,  hikari::platform::Keyboard::Key::Down);
        bindKey(Input::BUTTON_LEFT,  hikari::platform::Keyboard::Key::Left);
        bindKey(Input::BUTTON_SHOOT, hikari::platform::Keyboard::Key::A);
        bindKey(Input::BUTTON_JUMP,  hikari::platform::Keyboard::Key::S);
        bindKey(Input::BUTTON_START, hikari::platform::Keyboard::Key::Enter);
        bindKey(Input::BUTTON_SELECT, hikari::platform::Keyboard::Key::RShift);
        bindKey(Input::BUTTON_CANCEL, hikari::platform::Keyboard::Key::Escape);
    }

    const bool RealTimeInput::isUp(const Button &button) const {
        return !isDown(button);
    }

    const bool RealTimeInput::isDown(const Button &button) const {
        switch(button) {
            case Input::BUTTON_UP:
                return currentState.buttonUp;
                break;
            case Input::BUTTON_RIGHT:
                return currentState.buttonRight;
                break;
            case Input::BUTTON_DOWN:
                return currentState.buttonDown;
                break;
            case Input::BUTTON_LEFT:
                return currentState.buttonLeft;
                break;
            case Input::BUTTON_SHOOT:
                return currentState.buttonShoot;
                break;
            case Input::BUTTON_JUMP:
                return currentState.buttonJump;
                break;
            case Input::BUTTON_START:
                return currentState.buttonStart;
                break;
            case Input::BUTTON_SELECT:
                return currentState.buttonSelect;
                break;
            case Input::BUTTON_CANCEL:
                return currentState.buttonCancel;
                break;
            default:
                return !BUTTON_PUSHED;
                break;
        }
    }

    const bool RealTimeInput::isHeld(const Button &button) const {
        switch(button) {
            case Input::BUTTON_UP:
                return (currentState.buttonUp & previousState.buttonUp);
                break;
            case Input::BUTTON_RIGHT:
                return (currentState.buttonRight & previousState.buttonRight);
                break;
            case Input::BUTTON_DOWN:
                return (currentState.buttonDown & previousState.buttonDown);
                break;
            case Input::BUTTON_LEFT:
                return (currentState.buttonLeft & previousState.buttonLeft);
                break;
            case Input::BUTTON_SHOOT:
                return (currentState.buttonShoot & previousState.buttonShoot);
                break;
            case Input::BUTTON_JUMP:
                return (currentState.buttonJump & previousState.buttonJump);
                break;
            case Input::BUTTON_START:
                return (currentState.buttonStart & previousState.buttonStart);
                break;
            case Input::BUTTON_SELECT:
                return (currentState.buttonSelect & previousState.buttonSelect);
                break;
            case Input::BUTTON_CANCEL:
                return (currentState.buttonCancel & previousState.buttonCancel);
                break;
            default:
                return !BUTTON_PUSHED;
                break;
        }
    }

    const bool RealTimeInput::wasPressed(const Button &button) const {
        switch(button) {
            case Input::BUTTON_UP:
                return (currentState.buttonUp == BUTTON_PUSHED) && (previousState.buttonUp != BUTTON_PUSHED);
                break;
            case Input::BUTTON_RIGHT:
                return (currentState.buttonRight == BUTTON_PUSHED) && (previousState.buttonRight != BUTTON_PUSHED);
                break;
            case Input::BUTTON_DOWN:
                return (currentState.buttonDown == BUTTON_PUSHED) && (previousState.buttonDown != BUTTON_PUSHED);
                break;
            case Input::BUTTON_LEFT:
                return (currentState.buttonLeft == BUTTON_PUSHED) && (previousState.buttonLeft != BUTTON_PUSHED);
                break;
            case Input::BUTTON_SHOOT:
                return (currentState.buttonShoot && !previousState.buttonShoot);
                break;
            case Input::BUTTON_JUMP:
                return (currentState.buttonJump && !previousState.buttonJump);
                break;
            case Input::BUTTON_START:
                return (currentState.buttonStart && !previousState.buttonStart);
                break;
            case Input::BUTTON_SELECT:
                return (currentState.buttonSelect && !previousState.buttonSelect);
                break;
            case Input::BUTTON_CANCEL:
                return (currentState.buttonCancel && !previousState.buttonCancel);
                break;
            default:
                return !BUTTON_PUSHED;
                break;
        }
    }

    const bool RealTimeInput::wasReleased(const Button &button) const {
        switch(button) {
            case Input::BUTTON_UP:
                return (currentState.buttonUp != BUTTON_PUSHED) && (previousState.buttonUp == BUTTON_PUSHED);
                break;
            case Input::BUTTON_RIGHT:
                return (currentState.buttonRight != BUTTON_PUSHED) && (previousState.buttonRight == BUTTON_PUSHED);
                break;
            case Input::BUTTON_DOWN:
                return (currentState.buttonDown != BUTTON_PUSHED) && (previousState.buttonDown == BUTTON_PUSHED);
                break;
            case Input::BUTTON_LEFT:
                return (currentState.buttonLeft != BUTTON_PUSHED) && (previousState.buttonLeft == BUTTON_PUSHED);
                break;
            case Input::BUTTON_SHOOT:
                return (!currentState.buttonShoot && previousState.buttonShoot);
                break;
            case Input::BUTTON_JUMP:
                return (!currentState.buttonJump && previousState.buttonJump);
                break;
            case Input::BUTTON_START:
                return (!currentState.buttonStart && previousState.buttonStart);
                break;
            case Input::BUTTON_SELECT:
                return (!currentState.buttonSelect && previousState.buttonSelect);
                break;
            case Input::BUTTON_CANCEL:
                return (!currentState.buttonCancel && previousState.buttonCancel);
                break;
            default:
                return !BUTTON_PUSHED;
                break;
        }
    }

    void RealTimeInput::update(float dt) {
        previousState = currentState;

        currentState.buttonUp     = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_UP]);
        currentState.buttonRight  = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_RIGHT]);
        currentState.buttonDown   = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_DOWN]);
        currentState.buttonLeft   = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_LEFT]);
        currentState.buttonShoot  = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_SHOOT]);
        currentState.buttonJump   = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_JUMP]);
        currentState.buttonStart  = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_START]);
        currentState.buttonSelect = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_SELECT]);
        currentState.buttonCancel = hikari::platform::Keyboard::isKeyPressed(keybindings[Input::BUTTON_CANCEL]);
    }

    void RealTimeInput::bindKey(const Button & button, hikari::platform::Keyboard::Key key) {
        keybindings[button] = key;
    }

} // hikari