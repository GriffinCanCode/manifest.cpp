#pragma once

#include <gtest/gtest.h>
#include "../../src/core/types/Types.hpp"
#include "../../src/core/math/Vector.hpp"
#include "../../src/core/math/Matrix.hpp"
#include "../../src/world/tiles/Tile.hpp"
#include <random>
#include <concepts>

namespace Manifest::Test {

using namespace Core::Types;
using namespace Core::Math;
using namespace World::Tiles;

// Floating point comparison with tolerance
template<std::floating_point T>
bool nearly_equal(T a, T b, T tolerance = T{1e-6}) {
    return std::abs(a - b) <= tolerance;
}

// Vector comparison with tolerance
template<std::floating_point T, std::size_t N>
bool nearly_equal(const Vector<T, N>& a, const Vector<T, N>& b, T tolerance = T{1e-6}) {
    for (std::size_t i = 0; i < N; ++i) {
        if (!nearly_equal(a[i], b[i], tolerance)) {
            return false;
        }
    }
    return true;
}

// Matrix comparison with tolerance
template<std::floating_point T, std::size_t Rows, std::size_t Cols>
bool nearly_equal(const Matrix<T, Rows, Cols>& a, const Matrix<T, Rows, Cols>& b, T tolerance = T{1e-6}) {
    for (std::size_t r = 0; r < Rows; ++r) {
        for (std::size_t c = 0; c < Cols; ++c) {
            if (!nearly_equal(a[r][c], b[r][c], tolerance)) {
                return false;
            }
        }
    }
    return true;
}

// Custom matchers for Google Test
MATCHER_P(NearlyEqualTo, expected, "") {
    return nearly_equal(arg, expected);
}

MATCHER_P2(NearlyEqualTo, expected, tolerance, "") {
    return nearly_equal(arg, expected, tolerance);
}

// Test fixture for deterministic randomness
class DeterministicTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng_.seed(TEST_SEED);
    }
    
    std::mt19937& rng() { return rng_; }
    
    template<typename T>
    T random_in_range(T min_val, T max_val) {
        if constexpr (std::integral<T>) {
            std::uniform_int_distribution<T> dist(min_val, max_val);
            return dist(rng_);
        } else {
            std::uniform_real_distribution<T> dist(min_val, max_val);
            return dist(rng_);
        }
    }
    
    Vec3f random_vector() {
        return Vec3f{
            random_in_range(-10.0f, 10.0f),
            random_in_range(-10.0f, 10.0f),
            random_in_range(-10.0f, 10.0f)
        };
    }
    
    HexCoordinate random_hex_coordinate() {
        std::int32_t q = random_in_range(-50, 50);
        std::int32_t r = random_in_range(-50, 50);
        std::int32_t s = -q - r;
        return HexCoordinate{q, r, s};
    }

private:
    static constexpr std::uint32_t TEST_SEED = 42;
    std::mt19937 rng_{TEST_SEED};
};

// Benchmark utilities
class BenchmarkTimer {
    std::chrono::high_resolution_clock::time_point start_;
    
public:
    BenchmarkTimer() : start_{std::chrono::high_resolution_clock::now()} {}
    
    template<typename Duration = std::chrono::microseconds>
    auto elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<Duration>(end - start_);
    }
    
    void restart() {
        start_ = std::chrono::high_resolution_clock::now();
    }
};

// Performance test macros
#define BENCHMARK_TEST(test_name, iterations) \
    TEST(Performance, test_name) { \
        constexpr int ITERATIONS = iterations; \
        BenchmarkTimer timer; \
        for (int i = 0; i < ITERATIONS; ++i) {

#define END_BENCHMARK_TEST(expected_time_us) \
        } \
        auto elapsed = timer.elapsed<std::chrono::microseconds>(); \
        std::cout << "Elapsed time: " << elapsed.count() << " μs" << std::endl; \
        EXPECT_LT(elapsed.count(), expected_time_us) << "Performance regression detected"; \
    }

// Memory leak detection helpers
class MemoryTracker {
    std::size_t initial_allocations_{};
    std::size_t initial_memory_{};
    
public:
    MemoryTracker() {
        // Would integrate with custom allocator stats
        reset();
    }
    
    void reset() {
        // Reset counters
    }
    
    bool has_leaks() const {
        // Check if allocations/deallocations match
        return false; // Placeholder
    }
    
    std::size_t leaked_bytes() const {
        return 0; // Placeholder
    }
};

// Mock objects for testing
class MockTile {
    TileId id_;
    HexCoordinate coordinate_;
    TerrainType terrain_{TerrainType::Plains};
    bool passable_{true};
    
public:
    MockTile(TileId id, const HexCoordinate& coord) 
        : id_{id}, coordinate_{coord} {}
    
    TileId id() const { return id_; }
    const HexCoordinate& coordinate() const { return coordinate_; }
    TerrainType terrain() const { return terrain_; }
    void set_terrain(TerrainType terrain) { terrain_ = terrain; }
    bool is_passable() const { return passable_; }
    void set_passable(bool passable) { passable_ = passable; }
};

// Test data generators
struct TestDataGenerator {
    static std::vector<HexCoordinate> generate_hex_ring(const HexCoordinate& center, std::int32_t radius) {
        std::vector<HexCoordinate> ring;
        
        if (radius == 0) {
            ring.push_back(center);
            return ring;
        }
        
        HexCoordinate current = center;
        current.q += radius;
        current.s -= radius;
        
        for (int direction = 0; direction < 6; ++direction) {
            for (int step = 0; step < radius; ++step) {
                ring.push_back(current);
                
                switch (direction) {
                    case 0: current.q--; current.r++; break; // Southwest
                    case 1: current.r++; current.s--; break; // South
                    case 2: current.s--; current.q++; break; // Southeast
                    case 3: current.q++; current.r--; break; // Northeast
                    case 4: current.r--; current.s++; break; // North
                    case 5: current.s++; current.q--; break; // Northwest
                }
            }
        }
        
        return ring;
    }
    
    static std::vector<HexCoordinate> generate_hex_spiral(const HexCoordinate& center, std::int32_t max_radius) {
        std::vector<HexCoordinate> spiral;
        
        for (std::int32_t radius = 0; radius <= max_radius; ++radius) {
            auto ring = generate_hex_ring(center, radius);
            spiral.insert(spiral.end(), ring.begin(), ring.end());
        }
        
        return spiral;
    }
};

// Assertion helpers
#define ASSERT_VALID_HEX(coord) \
    ASSERT_TRUE((coord).is_valid()) << "Invalid hex coordinate: " \
        << (coord).q << ", " << (coord).r << ", " << (coord).s

#define EXPECT_VALID_HEX(coord) \
    EXPECT_TRUE((coord).is_valid()) << "Invalid hex coordinate: " \
        << (coord).q << ", " << (coord).r << ", " << (coord).s

#define ASSERT_NEARLY_EQUAL(actual, expected, tolerance) \
    ASSERT_TRUE(nearly_equal(actual, expected, tolerance)) \
        << "Values not nearly equal: " << actual << " vs " << expected

#define EXPECT_NEARLY_EQUAL(actual, expected, tolerance) \
    EXPECT_TRUE(nearly_equal(actual, expected, tolerance)) \
        << "Values not nearly equal: " << actual << " vs " << expected

} // namespace Manifest::Test
