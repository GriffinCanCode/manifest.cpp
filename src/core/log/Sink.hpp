#pragma once

#include "Record.hpp"
#include "Level.hpp"
#include "../types/Modern.hpp"
#include <memory>
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>

namespace Manifest {
namespace Core {
namespace Log {

// Abstract sink interface for extensibility
class Sink {
public:
    virtual ~Sink() = default;
    virtual void log(const Record& record) = 0;
    virtual void flush() = 0;
    virtual void set_level(Level level) noexcept { min_level_ = level; }
    virtual bool should_log(Level level) const noexcept { return level >= min_level_; }

protected:
    std::atomic<Level> min_level_{Level::Info};
};

// High-performance console sink with color support
class ConsoleSink final : public Sink {
    mutable std::mutex mutex_;
    bool colored_;
    std::ostream* stream_;

public:
    explicit ConsoleSink(bool colored = true, std::ostream* stream = &std::cout) 
        : colored_(colored), stream_(stream) {}

    void log(const Record& record) override {
        if (!should_log(record.level)) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (colored_) {
            *stream_ << to_color(record.level);
        }
        
        *stream_ << '[' << record.timestamp_string() << "] "
                 << '[' << to_string(record.level) << "] "
                 << '[' << record.logger_name << "] "
                 << record.message;
                 
        if (colored_) {
            *stream_ << color_reset();
        }
        
        *stream_ << '\n';
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        stream_->flush();
    }
};

// File sink with automatic rotation capability
class FileSink final : public Sink {
    mutable std::mutex mutex_;
    std::string base_filename_;
    std::ofstream file_stream_;
    std::size_t max_size_;
    std::size_t current_size_;

public:
    explicit FileSink(const std::string& filename, std::size_t max_size = 10'000'000)
        : base_filename_(filename), max_size_(max_size), current_size_(0) {
        open_file();
    }

    void log(const Record& record) override {
        if (!should_log(record.level)) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        const std::string log_line = format_record(record);
        current_size_ += log_line.length();
        
        if (current_size_ > max_size_) {
            rotate_file();
        }
        
        file_stream_ << log_line << '\n';
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        file_stream_.flush();
    }

private:
    void open_file() {
        file_stream_.open(base_filename_, std::ios::app);
        auto pos = file_stream_.tellp();
        current_size_ = pos >= 0 ? static_cast<std::size_t>(pos) : 0;
    }
    
    void rotate_file() {
        file_stream_.close();
        
        // Simple rotation: filename.1, filename.2, etc.
        std::string rotated_name = base_filename_ + ".1";
        std::rename(base_filename_.c_str(), rotated_name.c_str());
        
        open_file();
        current_size_ = 0;
    }
    
    std::string format_record(const Record& record) const {
        std::string result;
        result.reserve(200);
        result += '[';
        result += record.timestamp_string();
        result += "] [";
        result += to_string(record.level);
        result += "] [";
        result += record.logger_name;
        result += "] ";
        result += record.message;
        return result;
    }
};

// Null sink for testing and performance benchmarks  
class NullSink final : public Sink {
public:
    void log(const Record&) override {}
    void flush() override {}
};

// Multi-sink for broadcasting to multiple destinations
class MultiSink final : public Sink {
    std::vector<std::shared_ptr<Sink>> sinks_;
    mutable std::mutex mutex_;

public:
    void add_sink(std::shared_ptr<Sink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.emplace_back(std::move(sink));
    }
    
    void log(const Record& record) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& sink : sinks_) {
            sink->log(record);
        }
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& sink : sinks_) {
            sink->flush();
        }
    }
};

}  // namespace Log
}  // namespace Core
}  // namespace Manifest
