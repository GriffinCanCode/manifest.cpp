# Manifest Logging System

A high-performance, extensible logging system built for modern C++23 with strong typing and zero-overhead abstractions.

## Features

- **🚀 High Performance**: Compile-time level filtering, minimal allocations, lock-free fast path
- **🎯 Strong Typing**: Type-safe log levels and structured data support
- **🔧 Extensible**: Pluggable sink architecture with built-in console, file, and multi-sink support
- **🧪 Testable**: Interface-based design with dependency injection and mock-friendly APIs
- **⚡ Modern C++23**: Uses `std::format`, `std::source_location`, and other modern features
- **🔒 Thread-Safe**: All components are thread-safe by design

## Quick Start

```cpp
#include "core/log/Log.hpp"

int main() {
    // Initialize logging (optional - uses console sink by default)
    auto logger = Manifest::Core::Log::Registry::create("MyApp");
    Manifest::Core::Log::Global::set(std::move(logger));
    
    // Use global logging macros
    LOG_INFO("Application starting");
    LOG_WARN("This is a warning with data: {}", 42);
    LOG_ERROR("Error occurred: {}", "file not found");
    
    return 0;
}
```

## Architecture

### Core Components

- **`Level.hpp`**: Strongly typed log levels with compile-time utilities
- **`Record.hpp`**: Log record structure with timestamp, thread info, and source location
- **`Sink.hpp`**: Sink interface and built-in implementations (Console, File, Multi, Null)
- **`Logger.hpp`**: Main logger implementation with formatting support
- **`Log.hpp`**: Global logger and convenience macros

### Design Principles

1. **Single Responsibility**: Each file has one clear purpose
2. **Zero-Cost Abstractions**: Compile-time optimization where possible
3. **Exception Safety**: Strong exception safety guarantees
4. **Resource Management**: RAII throughout, no raw pointers
5. **Testability**: Dependency injection and mockable interfaces

## Usage Examples

### Module-Specific Logging

```cpp
class GameEngine {
private:
    MODULE_LOGGER("Engine");  // Creates static logger

public:
    void initialize() {
        logger_->info("GameEngine initializing...");
        // ... initialization code ...
        logger_->info("GameEngine ready");
    }
    
    void handle_error(const std::exception& e) {
        logger_->error("Exception: {}", e.what());
    }
};
```

### Custom Sink Configuration

```cpp
// Multi-sink setup: console + file + custom
auto multi_sink = std::make_shared<MultiSink>();
multi_sink->add_sink(std::make_shared<ConsoleSink>(true)); // colored
multi_sink->add_sink(std::make_shared<FileSink>("app.log", 1024*1024)); // 1MB rotation
multi_sink->add_sink(std::make_shared<MyCustomSink>());

auto logger = Registry::create("MyApp", multi_sink, Level::Debug);
```

### Structured Logging

```cpp
// Performance metrics
auto start = std::chrono::high_resolution_clock::now();
// ... operation ...
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

logger->info("Operation completed: duration={}ms, status=success", duration.count());

// Error context
try {
    risky_operation();
} catch (const std::exception& e) {
    logger->error("Operation failed: error='{}', context=user_action", e.what());
}
```

## Performance

The logging system is designed for high-performance applications:

- **Compile-time filtering**: Debug/trace logs have zero runtime cost in release builds
- **Minimal allocations**: Efficient string formatting and memory reuse
- **Lock-free fast path**: Level checks and filtering avoid locks
- **Structured data**: Type-safe formatting with `std::format`

Benchmarks show >1M messages/second with NullSink on modern hardware.

## Thread Safety

All components are thread-safe:
- Loggers can be shared between threads
- Sinks use appropriate synchronization
- Global state is properly protected
- Lock-free operations where possible

## Testing

Comprehensive unit tests cover:
- All log levels and formatting
- Thread safety under load
- Sink implementations
- Error handling and edge cases
- Performance benchmarks

Run tests with:
```bash
cd build && ctest --output-on-failure
```

## Integration

To add logging to your module:

1. Include the main header: `#include "core/log/Log.hpp"`
2. Use global macros: `LOG_INFO("message")` 
3. Or create module logger: `MODULE_LOGGER("ModuleName")`
4. Configure sinks as needed in your main function

The system integrates seamlessly with the existing Manifest Engine architecture and follows the established patterns for modern C++23 development.
