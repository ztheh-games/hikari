#ifndef GCN_SDLINPUT_HPP
#define GCN_SDLINPUT_HPP

#include "hikari/client/platform/Events.hpp"

#include <queue>

#include "guichan/input.hpp"
#include "guichan/keyinput.hpp"
#include "guichan/mouseinput.hpp"
#include "guichan/platform.hpp"




namespace gcn
{
    class Key;

    /**
     * Engine platform implementation of Input.
     */
    class GCN_EXTENSION_DECLSPEC PlatformInput : public Input
    {
    public:

        /**
         * Constructor.
         */
        PlatformInput();

        /**
         * Pushes a platform event. Call this during fixed-step event polling to
         * update input with user input. The RenderTarget should be provided in order
         * to calculate coordinates of mouse events accurately.
         *
         * @param event an engine platform event.
         */
        virtual void pushInput(const hikari::platform::Event& event);

        /**
         * Polls all input. It exists for input driver compatibility.
         */
        virtual void _pollInput() { }


        // Inherited from Input

        virtual bool isKeyQueueEmpty();

        virtual KeyInput dequeueKeyInput();

        virtual bool isMouseQueueEmpty();

        virtual MouseInput dequeueMouseInput();

    protected:
        /**
         * Converts an engine mouse button to a Guichan mouse button.
         * representation.
         *
         * @param button an engine mouse button.
         * @return a Guichan mouse button.
         */
        int convertMouseButton(hikari::platform::Mouse::Button button);
                
        /**
         * Converts an engine key to a Guichan key value.
         *
         * @param key The engine key to convert.
         * @return A Guichan key value. -1 if no conversion took place.
         * @see Key
         */
        int convertKeyToGuichanKeyValue(hikari::platform::Keyboard::Key key);

        std::queue<KeyInput> mKeyInputQueue;
        std::queue<MouseInput> mMouseInputQueue;

        bool mMouseDown;
        bool mMouseInWindow;

        hikari::platform::Clock mClock;
    };
}

#endif // end HIKARI_GPUINPUT_HPP
