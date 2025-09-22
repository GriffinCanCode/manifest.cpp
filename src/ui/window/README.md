# Window Management System

A sophisticated, extensible window management system for the Manifest Engine, designed with strong typing, testability, and minimal tech debt.

## Key Features

### 🎯 **Extensible Architecture**
- Abstract interfaces for maximum testability  
- Factory pattern for runtime backend selection
- Clean separation of concerns
- Strong typing throughout

### 🔧 **Production Ready**
- GLFW backend with full feature support
- Mock backend for comprehensive testing
- Vulkan and OpenGL surface integration
- Cross-platform compatibility (Windows, macOS, Linux)

### 🧪 **Testing First**
- Mock implementations for unit testing
- Dependency injection support
- Comprehensive test coverage
- Isolated components for reliable testing

### ⚡ **High Performance**  
- Memory-efficient design
- RAII resource management
- Minimal overhead abstractions
- Modern C++23 features

## Quick Start

```cpp
#include "ui/window/Manager.hpp"
#include "ui/window/Surface.hpp"

using namespace Manifest::UI::Window;

int main() {
    // Initialize window manager
    Manager manager;
    FactoryConfig config{Backend::GLFW};
    manager.initialize(config);
    
    // Create a window
    WindowDesc desc{
        .size = {1280, 720},
        .title = "Manifest Engine",
        .resizable = true
    };
    
    auto window_handle = manager.create_window(desc);
    Window* window = manager.get_window(*window_handle);
    
    // Set up event handling
    manager.set_global_event_callback([](WindowHandle handle, const Event& event) {
        std::visit([handle](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, WindowCloseEvent>) {
                std::cout << "Window " << handle.value() << " close requested\\n";
            }
            // Handle other events...
        }, event);
    });
    
    // Main loop
    while (!manager.should_quit()) {
        manager.poll_all_events();
        // Render your content...
        manager.swap_all_buffers();
    }
    
    return 0;
}
```

## Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│    Manager      │────│   Window        │────│  EventSystem    │
│                 │    │   (Abstract)    │    │  (Abstract)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ WindowFactory   │    │   GLFW/Mock     │    │ GLFW/MockEvents │
│  (Abstract)     │    │ Implementation  │    │ Implementation  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ FactoryRegistry │    │ SurfaceFactory  │    │ Integration     │
│                 │    │ (Render Bridge) │    │ Utilities       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Core Components

### 🪟 **Window Interface**
Pure abstract interface for all window operations:
- Lifecycle management (create, destroy, poll events)
- State queries (size, position, properties)  
- State modifications (resize, move, retitle)
- Renderer integration (Vulkan/OpenGL surfaces)

### 🏭 **Factory System**
Extensible factory pattern for backend selection:
- `WindowFactory` - Abstract factory interface
- `FactoryRegistry` - Runtime backend discovery
- Backend implementations (GLFW, Mock, future: SDL2, Native)

### 📊 **Manager**
Central coordination for multiple windows:
- Multi-window support with handles
- Global event routing
- Primary window management
- Bulk operations (poll all, swap all)

### 🎮 **Event System**
Type-safe event handling with std::variant:
```cpp
using Event = std::variant<
    WindowCloseEvent, WindowResizeEvent, WindowMoveEvent,
    WindowFocusEvent, KeyEvent, MouseButtonEvent,
    MouseMoveEvent, ScrollEvent, DropEvent
>;
```

### 🎨 **Surface Integration**  
Seamless renderer backend integration:
- Vulkan surface creation
- OpenGL context management
- Required extension enumeration
- API support detection

## Testing Support

### Mock Backend
Full-featured mock implementation for testing:
```cpp
// Setup mock backend for testing
FactoryConfig config{Backend::Mock};
Manager manager;
manager.initialize(config);

auto handle = manager.create_window(desc);
MockWindow* mock = dynamic_cast<MockWindow*>(manager.get_window(handle));

// Test interactions
mock->simulate_close();
mock->simulate_resize({1920, 1080});

// Verify behavior
auto calls = mock->method_calls();
EXPECT_TRUE(std::find(calls.begin(), calls.end(), "close") != calls.end());
```

### Event Testing
```cpp  
MockEventSystem event_system;
event_system.queue_event(KeyEvent{KeyCode::Space, 0, Action::Press, Modifier::None});
event_system.process_queued_events(); // Dispatches to callbacks
```

## Strong Typing

The system uses strong typing throughout to prevent common errors:

```cpp
// Type-safe handles prevent mix-ups
WindowHandle window_handle{42};
TextureHandle texture_handle{42}; // Different type - won't compile if mixed

// Strongly-typed enums prevent invalid values
WindowState state = WindowState::Maximized; // Clear intent
API render_api = API::Vulkan; // Type-safe API selection

// Result types for error handling
Result<WindowHandle> result = manager.create_window(desc);
if (result) {
    WindowHandle handle = *result; // Safe access
} else {
    WindowError error = result.error(); // Explicit error handling
}
```

## Integration with Renderers

The window system integrates seamlessly with both Vulkan and OpenGL renderers:

### Vulkan Integration
```cpp
// Get required extensions
auto extensions = Integration::get_required_vulkan_extensions();

// Create Vulkan surface
auto surface_result = Integration::create_surface_for_renderer(
    *window, API::Vulkan, vulkan_instance);
```

### OpenGL Integration  
```cpp
// Make OpenGL context current
auto result = Integration::create_surface_for_renderer(
    *window, API::OpenGL);
    
// Now safe to call OpenGL functions
glClear(GL_COLOR_BUFFER_BIT);
```

## File Organization

```
src/ui/window/
├── Window.hpp          # Core window interface
├── Events.hpp          # Event system types  
├── Factory.hpp         # Factory interfaces
├── Manager.hpp         # Window manager
├── Surface.hpp         # Renderer integration
├── Keys.hpp            # Keyboard codes
├── Factory.cpp         # Factory implementation
├── Manager.cpp         # Manager implementation
├── Surface.cpp         # Surface utilities
├── backends/
│   ├── GLFW.hpp/.cpp   # GLFW backend
│   └── Mock.hpp/.cpp   # Testing backend
└── Example.cpp         # Usage demonstration
```

## Memory Management

All resources use RAII and smart pointers:
- Windows automatically cleaned up on destruction
- No manual memory management required
- Exception-safe resource handling
- Leak-free design with clear ownership

## Thread Safety

The window system is designed for single-threaded use (typical for most UI frameworks):
- All operations should occur on the main thread
- Events are dispatched synchronously during `poll_events()`
- No internal threading or synchronization

For multi-threaded applications, coordinate access through your application's threading model.

## Performance Considerations

- ✅ **Efficient**: Minimal overhead abstractions
- ✅ **Scalable**: Supports hundreds of windows without performance loss  
- ✅ **Memory-conscious**: Small memory footprint per window
- ✅ **Cache-friendly**: Contiguous data structures where possible

## Future Extensions

The system is designed for easy extension:
- **SDL2 Backend**: Drop-in replacement for game-focused applications
- **Native Backends**: Platform-specific implementations for minimal overhead
- **Web Backend**: WebGL/WebAssembly support for browser applications
- **VR/AR Support**: Integration with OpenXR for immersive applications

## Error Handling

All operations return `Result<T, Error>` types for explicit error handling:
```cpp
auto window_result = manager.create_window(desc);
if (!window_result) {
    switch (window_result.error()) {
        case WindowError::InitializationFailed:
            // Handle init failure
            break;
        case WindowError::InvalidSize:
            // Handle invalid size  
            break;
        // ...
    }
}
```

This prevents silent failures and makes error conditions explicit and testable.

---

*The window management system is designed to be the foundation for sophisticated UI applications while maintaining simplicity and testability. Every design decision prioritizes reducing technical debt and improving long-term maintainability.*
