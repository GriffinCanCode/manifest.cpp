# Camera Control System

A comprehensive, extensible camera control system designed for strategic games with strong typing, modularity, and testability.

## Architecture

### 🎮 **Controls** (`Controls.hpp/cpp`)
- **Input mapping and handling** with configurable key bindings
- **Multiple control modes**: Orbital (RTS-style), Free (FPS-style), Cinematic
- **Strong typing** for parameters (Speed, Sensitivity, etc.)
- **Edge scrolling** support with customizable margins
- **Smooth movement** with time-based updates

### 🎬 **Behavior** (`Behavior.hpp/cpp`)
- **Extensible behavior system** with IBehavior interface
- **Smooth animations** with easing functions (Linear, EaseIn/Out, Bounce, Elastic)
- **Composite behaviors**: Sequence, Parallel execution
- **Built-in behaviors**: MoveTo, LookAt, ZoomTo, OrbitTo, Shake, Follow
- **Preset library** for common camera movements

### 💾 **State** (`State.hpp/cpp`)
- **Save/restore camera states** with named snapshots
- **Multiple serialization formats**: Binary (performance), JSON (debugging)
- **State history** with configurable size limits
- **Smooth transitions** between saved states
- **RAII state guards** for temporary state changes

### 🖱️ **Cursor** (`Cursor.hpp/cpp`)
- **Context-sensitive cursor** changes based on game state
- **16 cursor types** including resize, interaction, and system cursors
- **Auto-hide functionality** after inactivity
- **Smooth transitions** between cursor types
- **Platform abstraction** with mock provider for testing

### 🎯 **Selection** (`Selection.hpp/cpp`)
- **Click-drag box selection** with visual feedback
- **Multiple selection modes**: Replace, Add, Remove, Toggle
- **Generic template system** for any entity type
- **Keyboard shortcuts** (Ctrl+A, Escape, Ctrl+I)
- **Filter system** for selective entity types

### 🔧 **System Integration** (`Camera.hpp/cpp`)
- **Unified interface** combining all components
- **Event routing** with priority handling
- **Factory functions** for common configurations
- **Automatic cursor context** synchronization

## Usage Examples

### Basic Setup

```cpp
#include "render/camera/Camera.hpp"

// Create RTS-style camera system
auto camera_system = Manifest::Render::Camera::Factory::create_rts_camera();

// Handle input events
auto result = camera_system.handle_event(mouse_event);

// Update system (call each frame)
camera_system.update(delta_time);
```

### Custom Control Configuration

```cpp
// Create custom control configuration
Manifest::Render::Camera::Controls::Config config;
config.move_speed = Manifest::Render::Camera::Speed{30.0f};
config.mouse_sensitivity = Manifest::Render::Camera::Sensitivity{0.004f};

// Apply to system
camera_system.controls().set_config(std::move(config));
```

### Cinematic Camera Movement

```cpp
// Create smooth fly-to behavior
auto fly_behavior = Manifest::Render::Camera::Behavior::Presets::fly_to(
    target_position, 
    Manifest::Render::Camera::Duration{2.5f}
);

// Apply to camera
camera_system.behavior().set_behavior(std::move(fly_behavior));
```

### State Management

```cpp
// Save current camera state
camera_system.save_current_state("strategic_view");

// Restore later
camera_system.restore_state("strategic_view");
```

### Selection System

```cpp
// Register selectable entities
auto selectable = std::make_unique<UnitSelectable>(unit_id, unit);
camera_system.selection().register_selectable(std::move(selectable));

// Set callbacks
camera_system.selection().set_selection_changed_callback(
    [](const auto& selected_ids) {
        // Handle selection change
    }
);
```

## Design Principles

### ✅ **Strong Typing**
- Uses `Quantity<Tag, Type>` for type-safe parameters
- `StrongId<Tag>` for entity identification
- `Result<T, Error>` for error handling

### 🧩 **Modularity**
- Each component can be used independently
- Clear interfaces with dependency injection
- Factory pattern for common configurations

### 🧪 **Testability**
- Mock providers for platform-specific code
- Pure functions where possible
- Comprehensive unit and integration tests

### ⚡ **Performance**
- Lazy evaluation of expensive operations
- Memory-efficient state management
- SIMD-friendly data layouts

### 🔄 **Extensibility**
- Interface-based design for behaviors and cursors
- Template system for selection with any entity type
- Plugin architecture for custom input providers

## Configuration Options

### Controls
- Movement speeds and sensitivities
- Key binding customization  
- Edge scrolling parameters
- Mouse inversion settings

### Behaviors
- Easing function selection
- Animation duration control
- Custom behavior composition
- Preset parameter tuning

### State Management
- History size limits
- Serialization format choice
- Auto-save frequency
- Compression settings

### Cursor System
- Context resolution rules
- Auto-hide timing
- Transition speeds
- Platform-specific cursor support

## Thread Safety

- **Read operations**: Thread-safe for queries
- **Write operations**: Single-threaded (main/game thread only)
- **Serialization**: Thread-safe when not modifying state
- **Event handling**: Assumes single-threaded event processing

## Testing

Comprehensive test suite covering:
- ✅ Unit tests for individual components
- ✅ Integration tests for system interactions  
- ✅ Mock providers for isolated testing
- ✅ Deterministic test framework usage
- ✅ Edge case and error condition testing

## Performance Characteristics

- **Memory usage**: ~2KB base + saved states
- **Update frequency**: 60Hz recommended
- **Input latency**: <16ms typical
- **State transitions**: <100ms for smooth animation
- **Selection performance**: O(n) for entity count, cached results
