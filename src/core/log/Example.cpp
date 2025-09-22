#include "Log.hpp"
#include <memory>
#include <thread>
#include <chrono>

using namespace Manifest::Core::Log;

// Example module-specific logger
class GameEngine {
private:
    MODULE_LOGGER("Engine");

public:
    void initialize() {
        logger_->info("GameEngine initializing...");
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        logger_->info("GameEngine initialized successfully");
    }

    void run_frame() {
        SCOPED_LOGGER_TRACE(*logger_, "Running frame {}", frame_count_++);
        
        // Simulate frame processing
        if (frame_count_ % 1000 == 0) {
            logger_->debug("Processed {} frames", frame_count_);
        }
    }

    void handle_error(const std::exception& e) {
        logger_->error("Exception caught: {}", e.what());
    }

private:
    std::size_t frame_count_ = 0;
};

// Example of different sink configurations
void demonstrate_sink_configurations() {
    std::cout << "\n=== Sink Configuration Examples ===\n\n";

    // Console sink with colors
    {
        std::cout << "1. Colored Console Output:\n";
        auto console_sink = std::make_shared<ConsoleSink>(true);
        auto logger = Registry::create("ColorDemo", console_sink);
        
        logger->trace("This is a trace message");
        logger->debug("This is a debug message");
        logger->info("This is an info message");
        logger->warn("This is a warning message");
        logger->error("This is an error message");
        logger->fatal("This is a fatal message");
    }

    // File logging
    {
        std::cout << "\n2. File Logging:\n";
        auto file_sink = std::make_shared<FileSink>("game_logs.txt", 1000);
        auto logger = Registry::create("FileDemo", file_sink);
        
        logger->info("Log message written to file");
        logger->warn("Warning message in file");
        std::cout << "Messages written to game_logs.txt\n";
    }

    // Multiple sinks
    {
        std::cout << "\n3. Multiple Sinks (Console + File):\n";
        auto multi_sink = std::make_shared<MultiSink>();
        multi_sink->add_sink(std::make_shared<ConsoleSink>(false)); // No colors for clarity
        multi_sink->add_sink(std::make_shared<FileSink>("multi_logs.txt"));
        
        auto logger = Registry::create("MultiDemo", std::dynamic_pointer_cast<Sink>(multi_sink));
        logger->info("This message goes to both console and file");
    }
}

// Example of structured logging patterns
void demonstrate_structured_logging() {
    std::cout << "\n=== Structured Logging Examples ===\n\n";

    auto logger = Registry::create("Structured");

    // Performance logging with metrics
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    logger->info("Operation completed: duration={}ms, result=success", duration.count());

    // Error context logging
    try {
        throw std::runtime_error("Simulated error");
    } catch (const std::exception& e) {
        logger->error("Error in operation: exception='{}', context=demo", e.what());
    }

    // Resource usage logging
    logger->debug("Memory usage: allocated={}MB, peak={}MB", 128, 256);
    
    // State transition logging
    logger->info("State transition: from=loading to=ready");
}

// Performance benchmark
void benchmark_logging() {
    std::cout << "\n=== Performance Benchmark ===\n\n";

    const int iterations = 100000;

    // Benchmark with NullSink (no I/O overhead)
    {
        auto null_sink = std::make_shared<NullSink>();
        auto logger = Registry::create("Benchmark", null_sink);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            logger->info("Benchmark message {} with data {}", i, i * 2);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double ops_per_second = (iterations * 1000000.0) / duration.count();
        
        std::cout << "NullSink Performance:\n";
        std::cout << "  Messages: " << iterations << "\n";
        std::cout << "  Duration: " << duration.count() << " μs\n";
        std::cout << "  Rate: " << static_cast<int>(ops_per_second) << " msgs/sec\n";
    }

    // Benchmark with level filtering (should be nearly as fast)
    {
        auto null_sink = std::make_shared<NullSink>();
        auto logger = Registry::create("FilterBench", null_sink, Level::Error);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            logger->debug("This message should be filtered out {}", i);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double ops_per_second = (iterations * 1000000.0) / duration.count();
        
        std::cout << "\nLevel Filtering Performance:\n";
        std::cout << "  Filtered Messages: " << iterations << "\n";
        std::cout << "  Duration: " << duration.count() << " μs\n";
        std::cout << "  Rate: " << static_cast<int>(ops_per_second) << " msgs/sec\n";
    }
}

int main() {
    try {
        std::cout << "=== Manifest Logging System Demo ===\n";

        // Basic global logging
        LOG_INFO("Application starting up");
        LOG_DEBUG("Debug information");
        LOG_WARN("This is a warning");
        
        // Module-specific logging
        GameEngine engine;
        engine.initialize();
        
        for (int i = 0; i < 5; ++i) {
            engine.run_frame();
        }

        demonstrate_sink_configurations();
        demonstrate_structured_logging();
        benchmark_logging();

        LOG_INFO("Demo completed successfully");
        
        return 0;
    } catch (const std::exception& e) {
        LOG_FATAL("Unhandled exception: {}", e.what());
        return 1;
    }
}
