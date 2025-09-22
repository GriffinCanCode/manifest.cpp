#pragma once

#include "../../core/types/Modern.hpp"
#include "../../core/math/Vector.hpp"
#include "Keys.hpp"
#include <functional>
#include <variant>
#include <cstdint>

namespace Manifest {
namespace UI {
namespace Window {

using namespace Core::Math;

// Input device types  
enum class MouseButton : std::uint8_t { Left, Right, Middle, Extra1, Extra2 };
enum class Action : std::uint8_t { Press, Release, Repeat };
enum class Modifier : std::uint8_t { None = 0, Shift = 1, Ctrl = 2, Alt = 4, Super = 8 };

// Event types using std::variant for type safety
struct WindowCloseEvent {};
struct WindowResizeEvent { Vec2i size; };
struct WindowMoveEvent { Vec2i position; };
struct WindowFocusEvent { bool focused; };
struct WindowIconifyEvent { bool iconified; };
struct WindowMaximizeEvent { bool maximized; };

struct KeyEvent {
    KeyCode key;
    std::uint32_t scancode;
    Action action;
    Modifier mods;
};

struct CharEvent {
    std::uint32_t codepoint;
};

struct MouseButtonEvent {
    MouseButton button;
    Action action;
    Modifier mods;
    Vec2d position;
};

struct MouseMoveEvent {
    Vec2d position;
    Vec2d delta;
};

struct ScrollEvent {
    Vec2d offset;
    Vec2d position;
};

struct DropEvent {
    Core::Modern::span<const char*> paths;
};

// Unified event type
using Event = std::variant<
    WindowCloseEvent,
    WindowResizeEvent, 
    WindowMoveEvent,
    WindowFocusEvent,
    WindowIconifyEvent,
    WindowMaximizeEvent,
    KeyEvent,
    CharEvent,
    MouseButtonEvent,
    MouseMoveEvent,
    ScrollEvent,
    DropEvent
>;

// Event callback types
using EventCallback = std::function<void(const Event&)>;

// Event system interface for dependency injection and testing
class EventSystem {
public:
    virtual ~EventSystem() = default;
    virtual void set_callback(EventCallback callback) = 0;
    virtual void dispatch(const Event& event) = 0;
    virtual void clear_callbacks() = 0;
};

}  // namespace Window
}  // namespace UI
}  // namespace Manifest
