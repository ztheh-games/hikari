#include "guichan/sfml/sfmlinput.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

#include "guichan/exception.hpp"
#include <iostream>

namespace gcn
{
    SFMLInput::SFMLInput()
    {
        mMouseInWindow = true;
        mMouseDown = false;
    }

    bool SFMLInput::isKeyQueueEmpty()
    {
        return mKeyInputQueue.empty();
    }

    KeyInput SFMLInput::dequeueKeyInput()
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

    bool SFMLInput::isMouseQueueEmpty()
    {
        return mMouseInputQueue.empty();
    }

    MouseInput SFMLInput::dequeueMouseInput()
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

    void SFMLInput::pushInput(const sf::Event& event, const sf::RenderTarget& target)
    {
        KeyInput keyInput;
        MouseInput mouseInput;

        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
        {
                int value = convertSFMLKeyToGuichanKeyValue(keyPressed->code);

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
                keyInput.setNumericPad(keyPressed->code >= sf::Keyboard::Key::Numpad0
                                       && keyPressed->code <= sf::Keyboard::Key::Numpad9);

                mKeyInputQueue.push(keyInput);
        }
        else if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>())
        {
                int value = convertSFMLKeyToGuichanKeyValue(keyReleased->code);

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
                keyInput.setNumericPad(keyReleased->code >= sf::Keyboard::Key::Numpad0
                                       && keyReleased->code <= sf::Keyboard::Key::Numpad9);

                mKeyInputQueue.push(keyInput);
        }
        else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
                sf::Vector2f normalizedCoords = target.mapPixelToCoords(mousePressed->position);

                mMouseDown = true;
                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(convertMouseButton(mousePressed->button));
                mouseInput.setType(MouseInput::Pressed);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                mMouseInputQueue.push(mouseInput);
        }
        else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
        {
                sf::Vector2f normalizedCoords = target.mapPixelToCoords(mouseReleased->position);

                mMouseDown = false;
                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(convertMouseButton(mouseReleased->button));
                mouseInput.setType(MouseInput::Released);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                mMouseInputQueue.push(mouseInput);
        }
        else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
        {
                sf::Vector2f normalizedCoords = target.mapPixelToCoords(mouseMoved->position);

                mouseInput.setX(static_cast<int>(normalizedCoords.x));
                mouseInput.setY(static_cast<int>(normalizedCoords.y));
                mouseInput.setButton(MouseInput::Empty);
                mouseInput.setType(MouseInput::Moved);
                mouseInput.setTimeStamp(mClock.getElapsedTime().asMilliseconds());

                mMouseInputQueue.push(mouseInput);
        }
        else if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>();
                 mouseWheel && mouseWheel->wheel == sf::Mouse::Wheel::Vertical)
        {
                sf::Vector2f normalizedCoords = target.mapPixelToCoords(mouseWheel->position);

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
        else if (event.is<sf::Event::FocusLost>())
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
        else if (event.is<sf::Event::FocusGained>())
        {
                mMouseInWindow = true;
        }
    }

    int SFMLInput::convertMouseButton(sf::Mouse::Button button)
    {
        switch (button)
        {
        case sf::Mouse::Button::Left:
              return MouseInput::Left;
              break;
        case sf::Mouse::Button::Right:
              return MouseInput::Right;
              break;
        case sf::Mouse::Button::Middle:
              return MouseInput::Middle;
              break;
          default:
              // We have an unknown mouse type which is ignored.
              return static_cast<int>(button);
        }
    }

    int SFMLInput::convertSFMLKeyToGuichanKeyValue(sf::Keyboard::Key key)
    {
        int value = -1;

        switch(key)
        {
            case sf::Keyboard::Key::Unknown:
                value = -1;
                break;
            case sf::Keyboard::Key::A:
                value = 'A';
                break;
            case sf::Keyboard::Key::B:
                value = 'B';
                break;
            case sf::Keyboard::Key::C:
                value = 'C';
                break;
            case sf::Keyboard::Key::D:
                value = 'D';
                break;
            case sf::Keyboard::Key::E:
                value = 'E';
                break;
            case sf::Keyboard::Key::F:
                value = 'F';
                break;
            case sf::Keyboard::Key::G:
                value = 'G';
                break;
            case sf::Keyboard::Key::H:
                value = 'H';
                break;
            case sf::Keyboard::Key::I:
                value = 'I';
                break;
            case sf::Keyboard::Key::J:
                value = 'J';
                break;
            case sf::Keyboard::Key::K:
                value = 'K';
                break;
            case sf::Keyboard::Key::L:
                value = 'L';
                break;
            case sf::Keyboard::Key::M:
                value = 'M';
                break;
            case sf::Keyboard::Key::N:
                value = 'N';
                break;
            case sf::Keyboard::Key::O:
                value = 'O';
                break;
            case sf::Keyboard::Key::P:
                value = 'P';
                break;
            case sf::Keyboard::Key::Q:
                value = 'Q';
                break;
            case sf::Keyboard::Key::R:
                value = 'R';
                break;
            case sf::Keyboard::Key::S:
                value = 'S';
                break;
            case sf::Keyboard::Key::T:
                value = 'T';
                break;
            case sf::Keyboard::Key::U:
                value = 'U';
                break;
            case sf::Keyboard::Key::V:
                value = 'V';
                break;
            case sf::Keyboard::Key::W:
                value = 'W';
                break;
            case sf::Keyboard::Key::X:
                value = 'X';
                break;
            case sf::Keyboard::Key::Y:
                value = 'Y';
                break;
            case sf::Keyboard::Key::Z:
                value = 'Z';
                break;
            case sf::Keyboard::Key::Num0:
                value = '0';
                break;
            case sf::Keyboard::Key::Num1:
                value = '1';
                break;
            case sf::Keyboard::Key::Num2:
                value = '2';
                break;
            case sf::Keyboard::Key::Num3:
                value = '3';
                break;
            case sf::Keyboard::Key::Num4:
                value = '4';
                break;
            case sf::Keyboard::Key::Num5:
                value = '5';
                break;
            case sf::Keyboard::Key::Num6:
                value = '6';
                break;
            case sf::Keyboard::Key::Num7:
                value = '7';
                break;
            case sf::Keyboard::Key::Num8:
                value = '8';
                break;
            case sf::Keyboard::Key::Num9:
                value = '9';
                break;
            case sf::Keyboard::Key::Escape:
                value = Key::Escape;
                break;
            case sf::Keyboard::Key::LControl:
                value = Key::LeftControl;
                break;
            case sf::Keyboard::Key::LShift:
                value = Key::LeftShift;
                break;
            case sf::Keyboard::Key::LAlt:
                value = Key::LeftAlt;
                break;
            case sf::Keyboard::Key::LSystem:
                value = Key::LeftMeta;
                break;
            case sf::Keyboard::Key::RControl:
                value = Key::RightControl;
                break;
            case sf::Keyboard::Key::RShift:
                value = Key::RightShift;
                break;
            case sf::Keyboard::Key::RAlt:
                value = Key::RightAlt;
                break;
            case sf::Keyboard::Key::RSystem:
                value = Key::RightMeta;
                break;
            case sf::Keyboard::Key::Menu:
                value = -1;
                break;
            case sf::Keyboard::Key::LBracket:
                value = '[';
                break;
            case sf::Keyboard::Key::RBracket:
                value = ']';
                break;
            case sf::Keyboard::Key::Semicolon:
                value = ';';
                break;
            case sf::Keyboard::Key::Comma:
                value = ',';
                break;
            case sf::Keyboard::Key::Period:
                value = '.';
                break;
            case sf::Keyboard::Key::Apostrophe:
                value = '\'';
                break;
            case sf::Keyboard::Key::Slash:
                value = '/';
                break;
            case sf::Keyboard::Key::Backslash:
                value = '\\';
                break;
            case sf::Keyboard::Key::Grave:
                value = '~';
                break;
            case sf::Keyboard::Key::Equal:
                value = '=';
                break;
            case sf::Keyboard::Key::Hyphen:
                value = '-';
                break;
            case sf::Keyboard::Key::Space:
                value = Key::Space;
                break;
            case sf::Keyboard::Key::Enter:
                value = Key::Enter;
                break;
            case sf::Keyboard::Key::Backspace:
                value = Key::Backspace;
                break;
            case sf::Keyboard::Key::Tab:
                value = Key::Tab;
                break;
            case sf::Keyboard::Key::PageUp:
                value = Key::PageUp;
                break;
            case sf::Keyboard::Key::PageDown:
                value = Key::PageDown;
                break;
            case sf::Keyboard::Key::End:
                value = Key::End;
                break;
            case sf::Keyboard::Key::Home:
                value = Key::Home;
                break;
            case sf::Keyboard::Key::Insert:
                value = Key::Insert;
                break;
            case sf::Keyboard::Key::Delete:
                value = Key::Delete;
                break;
            case sf::Keyboard::Key::Add:
                value = -1;
                break;
            case sf::Keyboard::Key::Subtract:
                value = -1;
                break;
            case sf::Keyboard::Key::Multiply:
                value = -1;
                break;
            case sf::Keyboard::Key::Divide:
                value = -1;
                break;
            case sf::Keyboard::Key::Left:
                value = Key::Left;
                break;
            case sf::Keyboard::Key::Right:
                value = Key::Right;
                break;
            case sf::Keyboard::Key::Up:
                value = Key::Up;
                break;
            case sf::Keyboard::Key::Down:
                value = Key::Down;
                break;
            case sf::Keyboard::Key::Numpad0:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad1:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad2:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad3:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad4:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad5:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad6:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad7:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad8:
                value = -1;
                break;
            case sf::Keyboard::Key::Numpad9:
                value = -1;
                break;
            case sf::Keyboard::Key::F1:
                value = Key::F1;
                break;
            case sf::Keyboard::Key::F2:
                value = Key::F2;
                break;
            case sf::Keyboard::Key::F3:
                value = Key::F3;
                break;
            case sf::Keyboard::Key::F4:
                value = Key::F4;
                break;
            case sf::Keyboard::Key::F5:
                value = Key::F5;
                break;
            case sf::Keyboard::Key::F6:
                value = Key::F6;
                break;
            case sf::Keyboard::Key::F7:
                value = Key::F7;
                break;
            case sf::Keyboard::Key::F8:
                value = Key::F8;
                break;
            case sf::Keyboard::Key::F9:
                value = Key::F9;
                break;
            case sf::Keyboard::Key::F10:
                value = Key::F10;
                break;
            case sf::Keyboard::Key::F11:
                value = Key::F11;
                break;
            case sf::Keyboard::Key::F12:
                value = Key::F12;
                break;
            case sf::Keyboard::Key::F13:
                value = Key::F13;
                break;
            case sf::Keyboard::Key::F14:
                value = Key::F14;
                break;
            case sf::Keyboard::Key::F15:
                value = Key::F15;
                break;
            case sf::Keyboard::Key::Pause:
                value = Key::Pause;
                break;
            default:
                break;
        }

        return value;
    }
}
