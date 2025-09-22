#include "../../src/core/log/Log.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <memory>

using namespace Manifest::Core::Log;

class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test sink that captures output
        test_output_ = std::make_shared<std::ostringstream>();
        auto test_sink = std::make_shared<TestSink>(test_output_);
        
        // Create test logger
        test_logger_ = Registry::create("Test", test_sink, Level::Trace);
    }

    void TearDown() override {
        test_logger_.reset();
        test_output_.reset();
    }

    std::shared_ptr<std::ostringstream> test_output_;
    std::unique_ptr<Logger> test_logger_;

private:
    class TestSink : public Sink {
        std::shared_ptr<std::ostringstream> output_;
        mutable std::mutex mutex_;

    public:
        explicit TestSink(std::shared_ptr<std::ostringstream> output)
            : output_(std::move(output)) {}

        void log(const Record& record) override {
            if (!should_log(record.level)) return;
            
            std::lock_guard<std::mutex> lock(mutex_);
            *output_ << '[' << to_string(record.level) << "] "
                    << '[' << record.logger_name << "] "
                    << record.message << '\n';
        }

        void flush() override {
            std::lock_guard<std::mutex> lock(mutex_);
            output_->flush();
        }
    };
};

TEST_F(LogTest, BasicLogging) {
    test_logger_->info("Test message");
    test_logger_->flush();
    
    std::string output = test_output_->str();
    EXPECT_TRUE(output.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(output.find("[Test]") != std::string::npos);
    EXPECT_TRUE(output.find("Test message") != std::string::npos);
}

TEST_F(LogTest, FormattedLogging) {
    test_logger_->info("Value: {}, Count: {}", 42, 100);
    test_logger_->flush();
    
    std::string output = test_output_->str();
    EXPECT_TRUE(output.find("Value: 42, Count: 100") != std::string::npos);
}

TEST_F(LogTest, LevelFiltering) {
    test_logger_->set_level(Level::Warn);
    
    test_logger_->debug("Debug message");
    test_logger_->warn("Warning message");
    test_logger_->flush();
    
    std::string output = test_output_->str();
    EXPECT_TRUE(output.find("Debug message") == std::string::npos);
    EXPECT_TRUE(output.find("Warning message") != std::string::npos);
}

TEST_F(LogTest, AllLevels) {
    const std::vector<std::pair<std::function<void()>, std::string>> tests = {
        {[&]() { test_logger_->trace("Trace test"); }, "TRACE"},
        {[&]() { test_logger_->debug("Debug test"); }, "DEBUG"},
        {[&]() { test_logger_->info("Info test"); }, "INFO"},
        {[&]() { test_logger_->warn("Warn test"); }, "WARN"},
        {[&]() { test_logger_->error("Error test"); }, "ERROR"},
        {[&]() { test_logger_->fatal("Fatal test"); }, "FATAL"}
    };

    for (const auto& [log_func, expected_level] : tests) {
        test_output_->str(""); // Clear
        log_func();
        test_logger_->flush();
        
        std::string output = test_output_->str();
        EXPECT_TRUE(output.find(expected_level) != std::string::npos) 
            << "Failed for level: " << expected_level;
    }
}

TEST_F(LogTest, ThreadSafety) {
    const int num_threads = 4;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                test_logger_->info("Thread {} Message {}", i, j);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    test_logger_->flush();
    
    // Verify we got the expected number of log lines
    std::string output = test_output_->str();
    int line_count = std::count(output.begin(), output.end(), '\n');
    EXPECT_EQ(line_count, num_threads * messages_per_thread);
}

// Test the compile-time level optimization
TEST(LogLevelTest, CompileTimeLevelFiltering) {
    EXPECT_TRUE(should_log<Level::Info>(Level::Info));
    EXPECT_TRUE(should_log<Level::Info>(Level::Warn));
    EXPECT_FALSE(should_log<Level::Warn>(Level::Info));
    EXPECT_FALSE(should_log<Level::Error>(Level::Debug));
}

TEST(LogLevelTest, LevelStringConversion) {
    EXPECT_EQ(to_string(Level::Trace), "TRACE");
    EXPECT_EQ(to_string(Level::Debug), "DEBUG");
    EXPECT_EQ(to_string(Level::Info), "INFO");
    EXPECT_EQ(to_string(Level::Warn), "WARN");
    EXPECT_EQ(to_string(Level::Error), "ERROR");
    EXPECT_EQ(to_string(Level::Fatal), "FATAL");
}

// Test NullSink performance
TEST(SinkTest, NullSinkPerformance) {
    auto null_sink = std::make_shared<NullSink>();
    auto logger = Registry::create("Perf", null_sink, Level::Trace);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000000; ++i) {
        logger->info("Performance test message {}", i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete very quickly with NullSink
    EXPECT_LT(duration.count(), 500000); // Less than 0.5 seconds
}

TEST(SinkTest, MultiSinkDistribution) {
    auto output1 = std::make_shared<std::ostringstream>();
    auto output2 = std::make_shared<std::ostringstream>();
    
    auto multi_sink = std::make_shared<MultiSink>();
    // Would need to implement TestSink as a separate class for this test
    // multi_sink->add_sink(std::make_unique<TestSink>(output1));
    // multi_sink->add_sink(std::make_unique<TestSink>(output2));
    
    // Test that both sinks receive the same messages
}
