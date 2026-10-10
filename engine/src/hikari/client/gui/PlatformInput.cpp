#include "hikari/client/platform/Events.hpp"
#include "hikari/client/gui/PlatformInput.hpp"

#include "guichan/exception.hpp"
#include <iostream>

namespace gcn
{
    PlatformInput::PlatformInput()
    {
        mMouseInWindow = true;
        mMouseDown = false;
    }

    bool PlatformInput::isKeyQueueEmpty()
    {
        return mKeyInputQueue.empty();
    }

    KeyInput PlatformInput::dequeueKeyInput()
    {
        KeyInput keyInput;

        if (mKeyInputQueue.empty())
        {
            throw GCN_EXCEPTION("The queue is empty.");
        }

        keyInput = mKeyInputQueue.front();
        mKeyInputQueue.pop();

        return keyInput;
    }

    bool PlatformInput::isMouseQueueEmpty()
    {
        return mMouseInputQueue.empty();
    }

    MouseInput PlatformInput::dequeueMouseInput()
    {
        MouseInput mouseInput;

        if (mMouseInputQueue.empty())
        {
            throw GCN_EXCEPTION("The queue is empty.");
        }

        mouseInput = mMouseInputQueue.front();
        mMouseInputQueue.pop();

        return mouseInput;
    }

    void PlatformInput::pushInput(const hikari::platform::Event& event)
    {
        KeyInput keyInput;
        MouseInput mouseInput;

        if (const auto* keyPressed = event.getIf<hikari::platform::Event::KeyPressed>())
        {
                int value = convertKeyToGuichanKeyValue(keyPressed->code);

                if (value == -1)
                {
                    value = static_cast<int>(keyPressed->code);
                }

                keyInput.setKey(Key(value));
                keyInput.setType(KeyInput::Pressed);
                keyInput.setShiftPressed(keyPressed->shift);
                keyInput.setControlPressed(keyPressed->control);
                keyInput.setAltPressed(keyPressed->alt);
                keyInput.setMetaPressed(keyPressed->system);
                keyInput.setNumericPad(keyPressed->code >= hikari::platform::Keyboard::Key::Numpad0
                                       && keyPressed->code <= hikari::platform::Keyboard::Key::Numpad9);

                mKeyInputQueue.push(keyInput);
        }
        else if (const auto* keyReleased = event.getIf<hikari::platform::Event::KeyReleased>())
        {
                int value = convertKeyToGuichanKeyValue(keyReleased->code);

                if (value == -1)
                {
                    value = static_cast<int>(keyReleased->code);
                }

                keyInput.setKey(Key(value));
                keyInput.setType(KeyInput::Released);
                keyInput.setShiftPressed(keyReleased->shift);
                keyInput.setControlPressed(keyReleased->control);
                keyInput.setAltPressed(keyReleased->alt);
                keyInput.setMetaPressed(keyReleased->system);
                keyInput.setNumericPad(keyReleased->code >= hikari::platform::Keyboard::Key::Numpad0
                                       && keyReleased->code <= hikari::platform::Keyboard::Key::Numpad9);

                mKeyInputQueue.push(keyInput);
        }
        else if (const auto* mousePressed = event.getIf<hikari::platform::Event::MouseButtonPressed>())
        {
                const auto normalizedCoords = mousePressed->position;

                mMouseDown = true;
                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(convertMouseButton(mousePressed->button));
                mouseInput.setType(MouseInput::Pressed);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                mMouseInputQueue.push(mouseInput);
        }
        else if (const auto* mouseReleased = event.getIf<hikari::platform::Event::MouseButtonReleased>())
        {
                const auto normalizedCoords = mouseReleased->position;

                mMouseDown = false;
                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(convertMouseButton(mouseReleased->button));
                mouseInput.setType(MouseInput::Released);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                mMouseInputQueue.push(mouseInput);
        }
        else if (const auto* mouseMoved = event.getIf<hikari::platform::Event::MouseMoved>())
        {
                const auto normalizedCoords = mouseMoved->position;

                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(MouseInput::Empty);
                mouseInput.setType(MouseInput::Moved);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                mMouseInputQueue.push(mouseInput);
        }
        else if (const auto* mouseWheel = event.getIf<hikari::platform::Event::MouseWheelScrolled>();
                 mouseWheel && mouseWheel->wheel == hikari::platform::Mouse::Wheel::Vertical)
        {
                const auto normalizedCoords = mouseWheel->position;

                mMouseDown = true;
                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(MouseInput::Empty);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                if (mouseWheel->delta > 0)
                {
                    mouseInput.setType(MouseInput::WheelMovedUp);
                    mMouseInputQueue.push(mouseInput);
                }
                else if (mouseWheel->delta < 0)
                {
                    mouseInput.setType(MouseInput::WheelMovedDown);
                    mMouseInputQueue.push(mouseInput);
                }

                // We don't enqueue it unless it's up or down, or else it's an
                // unknown movement type.
        }
        else if (event.is<hikari::platform::Event::FocusLost>())
        {
                /*
                 * This occurs when the mouse leaves the window and the Guichan
                 * application loses its mousefocus.
                 */
                mMouseInWindow = false;

                if (!mMouseDown)
                {
                    mouseInput.setX(-1);
                    mouseInput.setY(-1);
                    mouseInput.setButton(MouseInput::Empty);
                    mouseInput.setType(MouseInput::Moved);

                    mMouseInputQueue.push(mouseInput);
                }
        }
        else if (event.is<hikari::platform::Event::FocusGained>())
        {
                mMouseInWindow = true;
        }
    }

    int PlatformInput::convertMouseButton(hikari::platform::Mouse::Button button)
    {
        switch (button)
        {
        case hikari::platform::Mouse::Button::Left:
              return MouseInput::Left;
              break;
        case hikari::platform::Mouse::Button::Right:
              return MouseInput::Right;
              break;
        case hikari::platform::Mouse::Button::Middle:
              return MouseInput::Middle;
              break;
          default:
              // We have an unknown mouse type which is ignored.
              return static_cast<int>(button);
        }
    }

    int PlatformInput::convertKeyToGuichanKeyValue(hikari::platform::Keyboard::Key key)
    {
        int value = -1;

        switch(key)
        {
            case hikari::platform::Keyboard::Key::Unknown:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::A:
                value = 'A';
                break;
            case hikari::platform::Keyboard::Key::B:
                value = 'B';
                break;
            case hikari::platform::Keyboard::Key::C:
                value = 'C';
                break;
            case hikari::platform::Keyboard::Key::D:
                value = 'D';
                break;
            case hikari::platform::Keyboard::Key::E:
                value = 'E';
                break;
            case hikari::platform::Keyboard::Key::F:
                value = 'F';
                break;
            case hikari::platform::Keyboard::Key::G:
                value = 'G';
                break;
            case hikari::platform::Keyboard::Key::H:
                value = 'H';
                break;
            case hikari::platform::Keyboard::Key::I:
                value = 'I';
                break;
            case hikari::platform::Keyboard::Key::J:
                value = 'J';
                break;
            case hikari::platform::Keyboard::Key::K:
                value = 'K';
                break;
            case hikari::platform::Keyboard::Key::L:
                value = 'L';
                break;
            case hikari::platform::Keyboard::Key::M:
                value = 'M';
                break;
            case hikari::platform::Keyboard::Key::N:
                value = 'N';
                break;
            case hikari::platform::Keyboard::Key::O:
                value = 'O';
                break;
            case hikari::platform::Keyboard::Key::P:
                value = 'P';
                break;
            case hikari::platform::Keyboard::Key::Q:
                value = 'Q';
                break;
            case hikari::platform::Keyboard::Key::R:
                value = 'R';
                break;
            case hikari::platform::Keyboard::Key::S:
                value = 'S';
                break;
            case hikari::platform::Keyboard::Key::T:
                value = 'T';
                break;
            case hikari::platform::Keyboard::Key::U:
                value = 'U';
                break;
            case hikari::platform::Keyboard::Key::V:
                value = 'V';
                break;
            case hikari::platform::Keyboard::Key::W:
                value = 'W';
                break;
            case hikari::platform::Keyboard::Key::X:
                value = 'X';
                break;
            case hikari::platform::Keyboard::Key::Y:
                value = 'Y';
                break;
            case hikari::platform::Keyboard::Key::Z:
                value = 'Z';
                break;
            case hikari::platform::Keyboard::Key::Num0:
                value = '0';
                break;
            case hikari::platform::Keyboard::Key::Num1:
                value = '1';
                break;
            case hikari::platform::Keyboard::Key::Num2:
                value = '2';
                break;
            case hikari::platform::Keyboard::Key::Num3:
                value = '3';
                break;
            case hikari::platform::Keyboard::Key::Num4:
                value = '4';
                break;
            case hikari::platform::Keyboard::Key::Num5:
                value = '5';
                break;
            case hikari::platform::Keyboard::Key::Num6:
                value = '6';
                break;
            case hikari::platform::Keyboard::Key::Num7:
                value = '7';
                break;
            case hikari::platform::Keyboard::Key::Num8:
                value = '8';
                break;
            case hikari::platform::Keyboard::Key::Num9:
                value = '9';
                break;
            case hikari::platform::Keyboard::Key::Escape:
                value = Key::Escape;
                break;
            case hikari::platform::Keyboard::Key::LControl:
                value = Key::LeftControl;
                break;
            case hikari::platform::Keyboard::Key::LShift:
                value = Key::LeftShift;
                break;
            case hikari::platform::Keyboard::Key::LAlt:
                value = Key::LeftAlt;
                break;
            case hikari::platform::Keyboard::Key::LSystem:
                value = Key::LeftMeta;
                break;
            case hikari::platform::Keyboard::Key::RControl:
                value = Key::RightControl;
                break;
            case hikari::platform::Keyboard::Key::RShift:
                value = Key::RightShift;
                break;
            case hikari::platform::Keyboard::Key::RAlt:
                value = Key::RightAlt;
                break;
            case hikari::platform::Keyboard::Key::RSystem:
                value = Key::RightMeta;
                break;
            case hikari::platform::Keyboard::Key::Menu:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::LBracket:
                value = '[';
                break;
            case hikari::platform::Keyboard::Key::RBracket:
                value = ']';
                break;
            case hikari::platform::Keyboard::Key::Semicolon:
                value = ';';
                break;
            case hikari::platform::Keyboard::Key::Comma:
                value = ',';
                break;
            case hikari::platform::Keyboard::Key::Period:
                value = '.';
                break;
            case hikari::platform::Keyboard::Key::Apostrophe:
                value = '\'';
                break;
            case hikari::platform::Keyboard::Key::Slash:
                value = '/';
                break;
            case hikari::platform::Keyboard::Key::Backslash:
                value = '\\';
                break;
            case hikari::platform::Keyboard::Key::Grave:
                value = '~';
                break;
            case hikari::platform::Keyboard::Key::Equal:
                value = '=';
                break;
            case hikari::platform::Keyboard::Key::Hyphen:
                value = '-';
                break;
            case hikari::platform::Keyboard::Key::Space:
                value = Key::Space;
                break;
            case hikari::platform::Keyboard::Key::Enter:
                value = Key::Enter;
                break;
            case hikari::platform::Keyboard::Key::Backspace:
                value = Key::Backspace;
                break;
            case hikari::platform::Keyboard::Key::Tab:
                value = Key::Tab;
                break;
            case hikari::platform::Keyboard::Key::PageUp:
                value = Key::PageUp;
                break;
            case hikari::platform::Keyboard::Key::PageDown:
                value = Key::PageDown;
                break;
            case hikari::platform::Keyboard::Key::End:
                value = Key::End;
                break;
            case hikari::platform::Keyboard::Key::Home:
                value = Key::Home;
                break;
            case hikari::platform::Keyboard::Key::Insert:
                value = Key::Insert;
                break;
            case hikari::platform::Keyboard::Key::Delete:
                value = Key::Delete;
                break;
            case hikari::platform::Keyboard::Key::Add:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Subtract:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Multiply:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Divide:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Left:
                value = Key::Left;
                break;
            case hikari::platform::Keyboard::Key::Right:
                value = Key::Right;
                break;
            case hikari::platform::Keyboard::Key::Up:
                value = Key::Up;
                break;
            case hikari::platform::Keyboard::Key::Down:
                value = Key::Down;
                break;
            case hikari::platform::Keyboard::Key::Numpad0:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad1:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad2:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad3:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad4:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad5:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad6:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad7:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad8:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::Numpad9:
                value = -1;
                break;
            case hikari::platform::Keyboard::Key::F1:
                value = Key::F1;
                break;
            case hikari::platform::Keyboard::Key::F2:
                value = Key::F2;
                break;
            case hikari::platform::Keyboard::Key::F3:
                value = Key::F3;
                break;
            case hikari::platform::Keyboard::Key::F4:
                value = Key::F4;
                break;
            case hikari::platform::Keyboard::Key::F5:
                value = Key::F5;
                break;
            case hikari::platform::Keyboard::Key::F6:
                value = Key::F6;
                break;
            case hikari::platform::Keyboard::Key::F7:
                value = Key::F7;
                break;
            case hikari::platform::Keyboard::Key::F8:
                value = Key::F8;
                break;
            case hikari::platform::Keyboard::Key::F9:
                value = Key::F9;
                break;
            case hikari::platform::Keyboard::Key::F10:
                value = Key::F10;
                break;
            case hikari::platform::Keyboard::Key::F11:
                value = Key::F11;
                break;
            case hikari::platform::Keyboard::Key::F12:
                value = Key::F12;
                break;
            case hikari::platform::Keyboard::Key::F13:
                value = Key::F13;
                break;
            case hikari::platform::Keyboard::Key::F14:
                value = Key::F14;
                break;
            case hikari::platform::Keyboard::Key::F15:
                value = Key::F15;
                break;
            case hikari::platform::Keyboard::Key::Pause:
                value = Key::Pause;
                break;
            default:
                break;
        }

        return value;
    }
}
