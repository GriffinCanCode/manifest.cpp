#include "Factory.hpp"
#include "backends/GLFW.hpp"
#include "backends/Mock.hpp"
#include <algorithm>

namespace Manifest {
namespace UI {
namespace Window {

// Factory Registry Implementation
FactoryRegistry& FactoryRegistry::instance() {
    static FactoryRegistry registry;
    return registry;
}

void FactoryRegistry::register_factory(std::unique_ptr<WindowFactory> factory) {
    if (factory && factory->is_available()) {
        factories_.push_back(std::move(factory));
    }
}

Result<std::unique_ptr<WindowFactory>> 
FactoryRegistry::create_factory(const FactoryConfig& config) const {
    // Try preferred backend first
    for (const auto& factory : factories_) {
        if (factory->backend() == config.preferred_backend && factory->is_available()) {
            // Create new instance of the same type
            switch (factory->backend()) {
                case Backend::GLFW:
                    return std::unique_ptr<WindowFactory>(std::make_unique<Backends::GLFWFactory>());
                case Backend::Mock:
                    return std::unique_ptr<WindowFactory>(std::make_unique<Backends::MockFactory>());
                default:
                    break;
            }
        }
    }
    
    // Fall back to any available backend
    for (const auto& factory : factories_) {
        if (factory->is_available()) {
            switch (factory->backend()) {
                case Backend::GLFW:
                    return std::unique_ptr<WindowFactory>(std::make_unique<Backends::GLFWFactory>());
                case Backend::Mock:
                    return std::unique_ptr<WindowFactory>(std::make_unique<Backends::MockFactory>());
                default:
                    break;
            }
        }
    }
    
    return WindowError::BackendUnavailable;
}

std::vector<Backend> FactoryRegistry::available_backends() const {
    std::vector<Backend> backends;
    backends.reserve(factories_.size());
    
    for (const auto& factory : factories_) {
        if (factory->is_available()) {
            backends.push_back(factory->backend());
        }
    }
    
    return backends;
}

// Initialize factory registry with available backends
namespace {
struct RegistryInitializer {
    RegistryInitializer() {
        auto& registry = FactoryRegistry::instance();
        
        // Register GLFW factory (always available)
        registry.register_factory(std::make_unique<Backends::GLFWFactory>());
        
        // Always register mock factory for testing
        registry.register_factory(std::make_unique<Backends::MockFactory>());
    }
};

static RegistryInitializer g_registry_init;
}  // anonymous namespace

// Convenience functions
Result<std::unique_ptr<WindowFactory>> 
create_window_factory(const FactoryConfig& config) {
    auto& registry = FactoryRegistry::instance();
    auto factory_result = registry.create_factory(config);
    
    if (!factory_result) {
        return factory_result.error();
    }
    
    auto factory = std::move(*factory_result);
    auto init_result = factory->initialize();
    
    if (!init_result) {
        return WindowError::InitializationFailed;
    }
    
    return factory;
}

Result<std::unique_ptr<Window>> 
create_window(const WindowDesc& desc, const FactoryConfig& config) {
    auto factory_result = create_window_factory(config);
    if (!factory_result) {
        return factory_result.error();
    }
    
    auto factory = std::move(*factory_result);
    return factory->create_window(desc);
}

}  // namespace Window
}  // namespace UI
}  // namespace Manifest
