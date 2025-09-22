#include <gtest/gtest.h>
#include <cmath>
#include <memory>

#include "../../src/world/generation/Noise.hpp"

using namespace Manifest::World::Generation;

class NoiseTest : public ::testing::Test {
   protected:
    static constexpr float EPSILON = 1e-6f;
    static constexpr std::uint32_t TEST_SEED = 12345;
};

// Perlin noise tests
TEST_F(NoiseTest, PerlinBasicProperties) {
    auto perlin = std::make_unique<Perlin>(TEST_SEED);
    
    // Noise should be bounded roughly between -1 and 1
    for (int i = 0; i < 100; ++i) {
        const float x = static_cast<float>(i) * 0.1f;
        const float y = static_cast<float>(i) * 0.1f;
        const float value = perlin->sample(x, y);
        
        EXPECT_GE(value, -1.5f) << "Noise value too low at (" << x << ", " << y << ")";
        EXPECT_LE(value, 1.5f) << "Noise value too high at (" << x << ", " << y << ")";
    }
}

TEST_F(NoiseTest, PerlinDeterministic) {
    auto perlin1 = std::make_unique<Perlin>(TEST_SEED);
    auto perlin2 = std::make_unique<Perlin>(TEST_SEED);
    
    // Same seed should produce same results
    for (int i = 0; i < 50; ++i) {
        const float x = static_cast<float>(i) * 0.3f;
        const float y = static_cast<float>(i) * 0.2f;
        
        EXPECT_FLOAT_EQ(perlin1->sample(x, y), perlin2->sample(x, y))
            << "Perlin noise not deterministic at (" << x << ", " << y << ")";
    }
}

TEST_F(NoiseTest, PerlinContinuous) {
    auto perlin = std::make_unique<Perlin>(TEST_SEED);
    
    // Test continuity by sampling very close points
    const float x = 10.5f;
    const float y = 20.3f;
    const float delta = 1e-4f;
    
    const float value1 = perlin->sample(x, y);
    const float value2 = perlin->sample(x + delta, y);
    const float value3 = perlin->sample(x, y + delta);
    
    EXPECT_LT(std::abs(value1 - value2), 0.1f) << "Discontinuity in X direction";
    EXPECT_LT(std::abs(value1 - value3), 0.1f) << "Discontinuity in Y direction";
}

// Fractal noise tests
TEST_F(NoiseTest, FractalOctaveAddition) {
    auto base = std::make_unique<Perlin>(TEST_SEED);
    auto fractal = std::make_unique<Fractal>(
        std::make_unique<Perlin>(TEST_SEED), 
        4, Persistence{0.5f}, Lacunarity{2.0f}
    );
    
    // Fractal should generally have more variation than base
    float base_variance = 0.0f;
    float fractal_variance = 0.0f;
    const int samples = 100;
    
    float base_mean = 0.0f;
    float fractal_mean = 0.0f;
    
    for (int i = 0; i < samples; ++i) {
        const float x = static_cast<float>(i) * 0.1f;
        const float y = static_cast<float>(i % 7) * 0.1f;
        
        const float base_value = base->sample(x, y);
        const float fractal_value = fractal->sample(x, y);
        
        base_mean += base_value;
        fractal_mean += fractal_value;
    }
    
    base_mean /= samples;
    fractal_mean /= samples;
    
    for (int i = 0; i < samples; ++i) {
        const float x = static_cast<float>(i) * 0.1f;
        const float y = static_cast<float>(i % 7) * 0.1f;
        
        const float base_value = base->sample(x, y);
        const float fractal_value = fractal->sample(x, y);
        
        base_variance += (base_value - base_mean) * (base_value - base_mean);
        fractal_variance += (fractal_value - fractal_mean) * (fractal_value - fractal_mean);
    }
    
    EXPECT_GT(fractal_variance, base_variance * 0.5f) 
        << "Fractal should have significant detail";
}

// Ridge noise tests
TEST_F(NoiseTest, RidgeProperties) {
    auto base = std::make_unique<Perlin>(TEST_SEED);
    auto ridge = std::make_unique<Ridge>(std::make_unique<Perlin>(TEST_SEED));
    
    for (int i = 0; i < 50; ++i) {
        const float x = static_cast<float>(i) * 0.1f;
        const float y = static_cast<float>(i) * 0.1f;
        
        const float base_value = base->sample(x, y);
        const float ridge_value = ridge->sample(x, y);
        
        // Ridge should be 1 - abs(base)
        EXPECT_FLOAT_EQ(ridge_value, 1.0f - std::abs(base_value))
            << "Ridge formula incorrect at (" << x << ", " << y << ")";
            
        // Ridge should always be non-negative
        EXPECT_GE(ridge_value, 0.0f) << "Ridge value negative";
    }
}

// Continent shape tests
TEST_F(NoiseTest, ContinentPangaea) {
    auto primary = std::make_unique<Perlin>(TEST_SEED);
    auto secondary = std::make_unique<Perlin>(TEST_SEED + 1);
    auto continent = std::make_unique<Continent>(
        std::make_unique<Perlin>(TEST_SEED),
        std::make_unique<Perlin>(TEST_SEED + 1),
        ContinentShape::Pangaea,
        0.3f, Vec2{0.0f, 0.0f}, 50.0f
    );
    
    // Test that center has higher elevation than edges
    const float center_value = continent->sample(0.0f, 0.0f);
    const float edge_value = continent->sample(45.0f, 45.0f);
    
    EXPECT_GT(center_value, edge_value) << "Pangaea should be higher at center";
    
    // Far edge should be very low (ocean)
    const float far_edge = continent->sample(100.0f, 100.0f);
    EXPECT_LT(far_edge, 0.1f) << "Far edges should be ocean";
}

TEST_F(NoiseTest, ContinentRing) {
    auto continent = std::make_unique<Continent>(
        std::make_unique<Perlin>(TEST_SEED),
        std::make_unique<Perlin>(TEST_SEED + 1),
        ContinentShape::Ring,
        0.3f, Vec2{0.0f, 0.0f}, 50.0f
    );
    
    // Ring shape should have low elevation at center and far edges
    const float center_value = continent->sample(0.0f, 0.0f);
    const float ring_value = continent->sample(20.0f, 0.0f);  // On the ring
    const float far_edge = continent->sample(50.0f, 0.0f);
    
    EXPECT_LT(center_value, ring_value) << "Ring center should be lower than ring";
    EXPECT_LT(far_edge, ring_value) << "Ring edge should be lower than ring";
}

// Vec2 interface tests
TEST_F(NoiseTest, Vec2Interface) {
    auto perlin = std::make_unique<Perlin>(TEST_SEED);
    
    const Vec2 pos{10.5f, 20.3f};
    const float value1 = perlin->sample(pos);
    const float value2 = perlin->sample(pos.x, pos.y);
    
    EXPECT_FLOAT_EQ(value1, value2) << "Vec2 interface should match float interface";
}

// 3D noise tests
TEST_F(NoiseTest, ThreeDimensional) {
    auto perlin = std::make_unique<Perlin>(TEST_SEED);
    
    // 3D noise should be continuous
    const float x = 5.0f, y = 10.0f, z = 15.0f;
    const float delta = 1e-4f;
    
    const float value1 = perlin->sample(x, y, z);
    const float value2 = perlin->sample(x + delta, y, z);
    const float value3 = perlin->sample(x, y + delta, z);
    const float value4 = perlin->sample(x, y, z + delta);
    
    EXPECT_LT(std::abs(value1 - value2), 0.1f) << "3D noise discontinuous in X";
    EXPECT_LT(std::abs(value1 - value3), 0.1f) << "3D noise discontinuous in Y";
    EXPECT_LT(std::abs(value1 - value4), 0.1f) << "3D noise discontinuous in Z";
}

// Performance baseline test
TEST_F(NoiseTest, PerformanceBaseline) {
    auto perlin = std::make_unique<Perlin>(TEST_SEED);
    
    const auto start = std::chrono::high_resolution_clock::now();
    
    constexpr int SAMPLE_COUNT = 10000;
    float sum = 0.0f;
    
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        const float x = static_cast<float>(i % 100);
        const float y = static_cast<float>(i / 100);
        sum += perlin->sample(x, y);
    }
    
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Ensure the computation actually happened
    EXPECT_NE(sum, 0.0f);
    
    // Performance benchmark (adjust threshold as needed)
    EXPECT_LT(duration.count(), 50000) << "Noise generation too slow: " << duration.count() << "μs";
}
