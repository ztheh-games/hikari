#ifndef HIKARI_PLATFORM_EVENTS
#define HIKARI_PLATFORM_EVENTS

#include "hikari/core/graphics/Graphics.hpp"
#include <chrono>
#include <optional>
#include <variant>

namespace hikari::platform {

class Time {
    std::chrono::steady_clock::duration duration;
public:
    explicit Time(std::chrono::steady_clock::duration value = {}) : duration(value) {}
    float asSeconds() const { return std::chrono::duration<float>(duration).count(); }
    std::int64_t asMilliseconds() const { return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }
};
class Clock {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
public:
    Time getElapsedTime() const { return Time(std::chrono::steady_clock::now() - start); }
};

struct Keyboard {
    enum class Key {
        Unknown = -1, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Escape, LControl, LShift, LAlt, LSystem, RControl, RShift, RAlt, RSystem,
        Menu, LBracket, RBracket, Semicolon, Comma, Period, Apostrophe, Slash, Backslash, Grave, Equal, Hyphen,
        Space, Enter, Backspace, Tab, PageUp, PageDown, End, Home, Insert, Delete,
        Add, Subtract, Multiply, Divide, Left, Right, Up, Down,
        Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, Pause
    };
    static bool isKeyPressed(Key key);
};
struct Mouse {
    enum class Button { Left, Right, Middle, Extra1, Extra2 };
    enum class Wheel { Vertical, Horizontal };
};
class Event {
public:
    struct Closed {};
    struct FocusLost {};
    struct FocusGained {};
    struct KeyPressed { Keyboard::Key code; bool alt{}, control{}, shift{}, system{}; };
    struct KeyReleased { Keyboard::Key code; bool alt{}, control{}, shift{}, system{}; };
    struct MouseButtonPressed { Mouse::Button button; gfx::Vector2i position; };
    struct MouseButtonReleased { Mouse::Button button; gfx::Vector2i position; };
    struct MouseMoved { gfx::Vector2i position; };
    struct MouseWheelScrolled { Mouse::Wheel wheel; float delta; gfx::Vector2i position; };
    using Value = std::variant<Closed, FocusLost, FocusGained, KeyPressed, KeyReleased,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseWheelScrolled>;
private:
    Value value;
public:
    template<class T> explicit Event(T value) : value(value) {}
    template<class T> bool is() const { return std::holds_alternative<T>(value); }
    template<class T> const T * getIf() const { return std::get_if<T>(&value); }
};

class Session {
public:
    Session();
    ~Session();
    Session(const Session &) = delete;
    Session & operator=(const Session &) = delete;
};
class Window {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    Window();
    ~Window();
    void create(gfx::Vector2u size, const std::string & title, bool fullscreen, bool vsync);
    std::optional<Event> pollEvent();
    void present(const gfx::Texture & texture);
    gfx::Vector2i toLogical(gfx::Vector2i point) const;
};

}
#endif
