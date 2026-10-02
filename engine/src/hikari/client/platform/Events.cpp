#include "hikari/client/platform/Events.hpp"
#include "hikari/core/util/Log.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <stdexcept>

namespace hikari::platform {
namespace {
using Key = Keyboard::Key;
struct KeyPair { Key key; SDL_Keycode code; };
const KeyPair keys[]{
    {Key::A,SDLK_A},{Key::B,SDLK_B},{Key::C,SDLK_C},{Key::D,SDLK_D},{Key::E,SDLK_E},{Key::F,SDLK_F},
    {Key::G,SDLK_G},{Key::H,SDLK_H},{Key::I,SDLK_I},{Key::J,SDLK_J},{Key::K,SDLK_K},{Key::L,SDLK_L},
    {Key::M,SDLK_M},{Key::N,SDLK_N},{Key::O,SDLK_O},{Key::P,SDLK_P},{Key::Q,SDLK_Q},{Key::R,SDLK_R},
    {Key::S,SDLK_S},{Key::T,SDLK_T},{Key::U,SDLK_U},{Key::V,SDLK_V},{Key::W,SDLK_W},{Key::X,SDLK_X},
    {Key::Y,SDLK_Y},{Key::Z,SDLK_Z},
    {Key::Num0,SDLK_0},{Key::Num1,SDLK_1},{Key::Num2,SDLK_2},{Key::Num3,SDLK_3},{Key::Num4,SDLK_4},
    {Key::Num5,SDLK_5},{Key::Num6,SDLK_6},{Key::Num7,SDLK_7},{Key::Num8,SDLK_8},{Key::Num9,SDLK_9},
    {Key::Escape,SDLK_ESCAPE},{Key::Enter,SDLK_RETURN},{Key::Space,SDLK_SPACE},{Key::Backspace,SDLK_BACKSPACE},
    {Key::Tab,SDLK_TAB},{Key::Up,SDLK_UP},{Key::Down,SDLK_DOWN},{Key::Left,SDLK_LEFT},{Key::Right,SDLK_RIGHT},
    {Key::LShift,SDLK_LSHIFT},{Key::RShift,SDLK_RSHIFT},{Key::LControl,SDLK_LCTRL},{Key::RControl,SDLK_RCTRL},
    {Key::LAlt,SDLK_LALT},{Key::RAlt,SDLK_RALT},{Key::LSystem,SDLK_LGUI},{Key::RSystem,SDLK_RGUI},
    {Key::Menu,SDLK_MENU},{Key::LBracket,SDLK_LEFTBRACKET},{Key::RBracket,SDLK_RIGHTBRACKET},
    {Key::Semicolon,SDLK_SEMICOLON},{Key::Comma,SDLK_COMMA},{Key::Period,SDLK_PERIOD},
    {Key::Apostrophe,SDLK_APOSTROPHE},{Key::Slash,SDLK_SLASH},{Key::Backslash,SDLK_BACKSLASH},
    {Key::Grave,SDLK_GRAVE},{Key::Equal,SDLK_EQUALS},{Key::Hyphen,SDLK_MINUS},
    {Key::PageUp,SDLK_PAGEUP},{Key::PageDown,SDLK_PAGEDOWN},{Key::End,SDLK_END},{Key::Home,SDLK_HOME},
    {Key::Insert,SDLK_INSERT},{Key::Delete,SDLK_DELETE},{Key::Add,SDLK_KP_PLUS},{Key::Subtract,SDLK_KP_MINUS},
    {Key::Multiply,SDLK_KP_MULTIPLY},{Key::Divide,SDLK_KP_DIVIDE},
    {Key::Numpad0,SDLK_KP_0},{Key::Numpad1,SDLK_KP_1},{Key::Numpad2,SDLK_KP_2},{Key::Numpad3,SDLK_KP_3},
    {Key::Numpad4,SDLK_KP_4},{Key::Numpad5,SDLK_KP_5},{Key::Numpad6,SDLK_KP_6},{Key::Numpad7,SDLK_KP_7},
    {Key::Numpad8,SDLK_KP_8},{Key::Numpad9,SDLK_KP_9},
    {Key::F1,SDLK_F1},{Key::F2,SDLK_F2},{Key::F3,SDLK_F3},{Key::F4,SDLK_F4},{Key::F5,SDLK_F5},
    {Key::F6,SDLK_F6},{Key::F7,SDLK_F7},{Key::F8,SDLK_F8},{Key::F9,SDLK_F9},{Key::F10,SDLK_F10},
    {Key::F11,SDLK_F11},{Key::F12,SDLK_F12},{Key::F13,SDLK_F13},{Key::F14,SDLK_F14},{Key::F15,SDLK_F15},
    {Key::Pause,SDLK_PAUSE}
};
Key keyFor(SDL_Keycode code) {
    if (code == SDLK_KP_ENTER) return Key::Enter;
    for (const auto & pair : keys) if (pair.code == code) return pair.key;
    return Key::Unknown;
}
std::optional<Mouse::Button> mouseButton(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT: return Mouse::Button::Left;
        case SDL_BUTTON_RIGHT: return Mouse::Button::Right;
        case SDL_BUTTON_MIDDLE: return Mouse::Button::Middle;
        case SDL_BUTTON_X1: return Mouse::Button::Extra1;
        case SDL_BUTTON_X2: return Mouse::Button::Extra2;
        default: HIKARI_LOG(debug4) << "Ignoring unsupported SDL mouse button " << static_cast<int>(button); return std::nullopt;
    }
}
void require(bool success, const char * operation) {
    if (!success) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
}
bool Keyboard::isKeyPressed(Key key) {
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) return false;
    for (const auto & pair : keys) if (pair.key == key) {
        const auto scan = SDL_GetScancodeFromKey(pair.code,nullptr);
        int count = 0; const bool * state = SDL_GetKeyboardState(&count);
        return scan != SDL_SCANCODE_UNKNOWN && scan < count && state[scan];
    }
    return false;
}
Session::Session() { require(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO),"Initialize SDL"); }
Session::~Session() { SDL_Quit(); }
struct Window::Impl {
    SDL_Window * window = nullptr;
    std::unique_ptr<gfx::Renderer> renderer;
    ~Impl() { renderer.reset(); if (window) SDL_DestroyWindow(window); }
};
Window::Window() : impl(std::make_unique<Impl>()) {}
Window::~Window() = default;
void Window::create(gfx::Vector2u size, const std::string & title, bool fullscreen, bool vsync) {
    if (impl->window) throw std::logic_error("Game window already created");
    impl->window = SDL_CreateWindow(title.c_str(),static_cast<int>(size.x),static_cast<int>(size.y),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    require(impl->window != nullptr,"Create game window");
    if (fullscreen) {
        const auto display = SDL_GetDisplayForWindow(impl->window);
        int count = 0;
        auto ** modes = SDL_GetFullscreenDisplayModes(display,&count);
        require(modes != nullptr,"Enumerate fullscreen modes");
        const SDL_DisplayMode * selected = nullptr;
        for (int i = 0; i < count; ++i)
            if (modes[i]->w == static_cast<int>(size.x) && modes[i]->h == static_cast<int>(size.y)) { selected = modes[i]; break; }
        if (!selected) { selected = SDL_GetDesktopDisplayMode(display); HIKARI_LOG(warning) << "Requested fullscreen size unavailable; using desktop display mode."; }
        const bool modeSet = selected && SDL_SetWindowFullscreenMode(impl->window,selected);
        SDL_free(modes); require(modeSet,"Select fullscreen mode");
        require(SDL_SetWindowFullscreen(impl->window,true),"Enter fullscreen");
    }
    impl->renderer = std::make_unique<gfx::Renderer>(impl->window,vsync);
}
gfx::Vector2i Window::toLogical(gfx::Vector2i point) const {
    int width = 0,height = 0;
    require(SDL_GetWindowSize(impl->window,&width,&height),"Get window coordinates");
    if (!width || !height) return {-1,-1};
    return {static_cast<int>(point.x * 256.0 / width),static_cast<int>(point.y * 240.0 / height)};
}
std::optional<Event> Window::pollEvent() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: case SDL_EVENT_WINDOW_CLOSE_REQUESTED: return Event(Event::Closed{});
            case SDL_EVENT_WINDOW_FOCUS_LOST: return Event(Event::FocusLost{});
            case SDL_EVENT_WINDOW_FOCUS_GAINED: return Event(Event::FocusGained{});
            case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: {
                if (event.key.repeat) break;
                const bool alt = (event.key.mod & SDL_KMOD_ALT) != 0, control = (event.key.mod & SDL_KMOD_CTRL) != 0;
                const bool shift = (event.key.mod & SDL_KMOD_SHIFT) != 0, system = (event.key.mod & SDL_KMOD_GUI) != 0;
                const auto key = keyFor(SDL_GetKeyFromScancode(event.key.scancode,SDL_KMOD_NONE,false));
                if (event.type == SDL_EVENT_KEY_DOWN) return Event(Event::KeyPressed{key,alt,control,shift,system});
                return Event(Event::KeyReleased{key,alt,control,shift,system});
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (const auto button = mouseButton(event.button.button))
                    return Event(Event::MouseButtonPressed{*button,toLogical({static_cast<int>(event.button.x),static_cast<int>(event.button.y)})});
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (const auto button = mouseButton(event.button.button))
                    return Event(Event::MouseButtonReleased{*button,toLogical({static_cast<int>(event.button.x),static_cast<int>(event.button.y)})});
                break;
            case SDL_EVENT_MOUSE_MOTION:
                return Event(Event::MouseMoved{toLogical({static_cast<int>(event.motion.x),static_cast<int>(event.motion.y)})});
            case SDL_EVENT_MOUSE_WHEEL: {
                const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
                return Event(Event::MouseWheelScrolled{Mouse::Wheel::Vertical,event.wheel.y * direction,
                    toLogical({static_cast<int>(event.wheel.mouse_x),static_cast<int>(event.wheel.mouse_y)})});
            }
            default: break;
        }
    }
    return std::nullopt;
}
void Window::present(const gfx::Texture & texture) { impl->renderer->present(texture); }
}
