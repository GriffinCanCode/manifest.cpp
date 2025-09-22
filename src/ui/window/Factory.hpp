#pragma once

#include "Window.hpp"
#include "Events.hpp"
#include <memory>
#include <string_view>

namespace Manifest {
namespace UI {
namespace Window {

// Backend types
enum class Backend : std::uint8_t {
    GLFW,      // Cross-platform, mature
    SDL2,      // Gaming-focused
    Native,    // Platform-specific
    Mock       // For testing
};

// Factory configuration
struct FactoryConfig {
    Backend preferred_backend{Backend::GLFW};
    bool enable_debug_layers{false};
    std::string_view backend_hints{};
};

// Abstract factory interface for testability and extensibility
class WindowFactory {
public:
    virtual ~WindowFactory() = default;
    
    [[nodiscard]] virtual Result<std::unique_ptr<Window>> 
    create_window(const WindowDesc& desc) const = 0;
    
    [[nodiscard]] virtual Result<std::unique_ptr<EventSystem>> 
    create_event_system() const = 0;
    
    [[nodiscard]] virtual Backend backend() const = 0;
    [[nodiscard]] virtual bool is_available() const = 0;
    
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
};

// Factory registry for runtime backend selection
class FactoryRegistry {
    std::vector<std::unique_ptr<WindowFactory>> factories_;
    
public:
    static FactoryRegistry& instance();
    
    void register_factory(std::unique_ptr<WindowFactory> factory);
    
    [[nodiscard]] Result<std::unique_ptr<WindowFactory>> 
    create_factory(const FactoryConfig& config = {}) const;
    
    [[nodiscard]] std::vector<Backend> available_backends() const;
};

// Convenience functions
[[nodiscard]] Result<std::unique_ptr<WindowFactory>> 
create_window_factory(const FactoryConfig& config = {});

[[nodiscard]] Result<std::unique_ptr<Window>> 
create_window(const WindowDesc& desc, const FactoryConfig& config = {});

}  // namespace Window
}  // namespace UI
}  // namespace Manifest
