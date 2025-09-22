#pragma once

#include <concepts>
#include <memory>
#include <numbers>
#include <random>
#include <vector>

#include "../../core/types/Types.hpp"
#include "../../core/math/Vector.hpp"

namespace Manifest::World::Generation {

using Core::Types::Quantity;
using Core::Math::Vec2f;
using Core::Math::Vec3f;

// Strong typing for noise parameters
template <typename Tag>
using NoiseParam = Quantity<Tag, float>;

struct FrequencyTag {};
struct AmplitudeTag {}; 
struct PersistenceTag {};
struct LacunarityTag {};
struct ScaleTag {};

using Frequency = NoiseParam<FrequencyTag>;
using Amplitude = NoiseParam<AmplitudeTag>;
using Persistence = NoiseParam<PersistenceTag>;
using Lacunarity = NoiseParam<LacunarityTag>;
using Scale = NoiseParam<ScaleTag>;

// Noise sampling interface
class Noise {
   public:
    virtual ~Noise() = default;
    virtual float sample(float x_coord, float y_coord) const = 0;
    virtual float sample(float x_coord, float y_coord, float z_coord) const = 0;
    virtual float sample(const Vec2f& position) const { return sample(position.x(), position.y()); }
    virtual float sample(const Vec3f& position) const { return sample(position.x(), position.y(), position.z()); }
};

// Base noise generator with permutation table
class Perlin final : public Noise {
    std::vector<int> permutation_;

    static constexpr float fade(float interpolation) noexcept {
        return interpolation * interpolation * interpolation * (interpolation * (interpolation * 6.0F - 15.0F) + 10.0F);
    }

    static constexpr float lerp(float interpolation, float start_val, float end_val) noexcept {
        return start_val + (interpolation * (end_val - start_val));
    }

    static constexpr float grad(int hash, float x_coord, float y_coord) noexcept {
        const int hash_val = hash & 15;
        const float u_val = hash_val < 8 ? x_coord : y_coord;
        const float v_val = hash_val < 4 ? y_coord : (hash_val == 12 || hash_val == 14 ? x_coord : 0.0F);
        return ((hash_val & 1) == 0 ? u_val : -u_val) + ((hash_val & 2) == 0 ? v_val : -v_val);
    }

    static constexpr float grad(int hash, float x_coord, float y_coord, float z_coord) noexcept {
        const int hash_val = hash & 15;
        const float u_val = hash_val < 8 ? x_coord : y_coord;
        const float v_val = hash_val < 4 ? y_coord : (hash_val == 12 || hash_val == 14 ? x_coord : z_coord);
        return ((hash_val & 1) == 0 ? u_val : -u_val) + ((hash_val & 2) == 0 ? v_val : -v_val);
    }

   public:
    explicit Perlin(std::uint32_t seed = 0) {
        permutation_.resize(512);
        std::iota(permutation_.begin(), permutation_.begin() + 256, 0);

        std::mt19937 rng(seed);
        std::shuffle(permutation_.begin(), permutation_.begin() + 256, rng);
        std::copy_n(permutation_.begin(), 256, permutation_.begin() + 256);
    }

    float sample(float x_coord, float y_coord) const override {
        const int x_int = static_cast<int>(std::floor(x_coord)) & 255;
        const int y_int = static_cast<int>(std::floor(y_coord)) & 255;

        x_coord -= std::floor(x_coord);
        y_coord -= std::floor(y_coord);

        const float u_val = fade(x_coord);
        const float v_val = fade(y_coord);

        const int hash_a = permutation_[static_cast<std::size_t>(x_int)] + y_int;
        const int hash_b = permutation_[static_cast<std::size_t>(x_int + 1)] + y_int;

        return lerp(v_val,
            lerp(u_val, grad(permutation_[static_cast<std::size_t>(hash_a)], x_coord, y_coord),
                   grad(permutation_[static_cast<std::size_t>(hash_b)], x_coord - 1.0F, y_coord)),
            lerp(u_val, grad(permutation_[static_cast<std::size_t>(hash_a + 1)], x_coord, y_coord - 1.0F),
                   grad(permutation_[static_cast<std::size_t>(hash_b + 1)], x_coord - 1.0F, y_coord - 1.0F)));
    }

    float sample(float x, float y, float z) const override {
        const int X = static_cast<int>(std::floor(x)) & 255;
        const int Y = static_cast<int>(std::floor(y)) & 255;
        const int Z = static_cast<int>(std::floor(z)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        const float u = fade(x);
        const float v = fade(y);
        const float w = fade(z);

        const int A = permutation_[static_cast<std::size_t>(X)] + Y;
        const int AA = permutation_[static_cast<std::size_t>(A)] + Z;
        const int AB = permutation_[static_cast<std::size_t>(A + 1)] + Z;
        const int B = permutation_[static_cast<std::size_t>(X + 1)] + Y;
        const int BA = permutation_[static_cast<std::size_t>(B)] + Z;
        const int BB = permutation_[static_cast<std::size_t>(B + 1)] + Z;

        return lerp(w,
            lerp(v,
                lerp(u, grad(permutation_[static_cast<std::size_t>(AA)], x, y, z),
                       grad(permutation_[static_cast<std::size_t>(BA)], x - 1.0f, y, z)),
                lerp(u, grad(permutation_[static_cast<std::size_t>(AB)], x, y - 1.0f, z),
                       grad(permutation_[static_cast<std::size_t>(BB)], x - 1.0f, y - 1.0f, z))),
            lerp(v,
                lerp(u, grad(permutation_[static_cast<std::size_t>(AA + 1)], x, y, z - 1.0f),
                       grad(permutation_[static_cast<std::size_t>(BA + 1)], x - 1.0f, y, z - 1.0f)),
                lerp(u, grad(permutation_[static_cast<std::size_t>(AB + 1)], x, y - 1.0f, z - 1.0f),
                       grad(permutation_[static_cast<std::size_t>(BB + 1)], x - 1.0f, y - 1.0f, z - 1.0f))));
    }
};

// Multi-octave fractal noise implementation
class Fractal final : public Noise {
    std::unique_ptr<Noise> base_;
    int octaves_;
    float persistence_;
    float lacunarity_;

   public:
    Fractal(std::unique_ptr<Noise> base, int octaves = 4,
            Persistence persistence = Persistence{0.5f},
            Lacunarity lacunarity = Lacunarity{2.0f})
        : base_{std::move(base)},
          octaves_{octaves},
          persistence_{persistence.value()},
          lacunarity_{lacunarity.value()} {}

    float sample(float x, float y) const override {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float max_value = 0.0f;

        for (int i = 0; i < octaves_; ++i) {
            value += base_->sample(x * frequency, y * frequency) * amplitude;
            max_value += amplitude;
            amplitude *= persistence_;
            frequency *= lacunarity_;
        }

        return value / max_value;
    }

    float sample(float x, float y, float z) const override {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float max_value = 0.0f;

        for (int i = 0; i < octaves_; ++i) {
            value += base_->sample(x * frequency, y * frequency, z * frequency) * amplitude;
            max_value += amplitude;
            amplitude *= persistence_;
            frequency *= lacunarity_;
        }

        return value / max_value;
    }
};

// Ridge noise for mountain generation
class Ridge final : public Noise {
    std::unique_ptr<Noise> base_;

   public:
    explicit Ridge(std::unique_ptr<Noise> base) : base_{std::move(base)} {}

    float sample(float x, float y) const override {
        return 1.0f - std::abs(base_->sample(x, y));
    }

    float sample(float x, float y, float z) const override {
        return 1.0f - std::abs(base_->sample(x, y, z));
    }
};

// Turbulence noise for chaotic patterns
class Turbulence final : public Noise {
    std::unique_ptr<Noise> base_;
    float intensity_;

   public:
    explicit Turbulence(std::unique_ptr<Noise> base, float intensity = 1.0f)
        : base_{std::move(base)}, intensity_{intensity} {}

    float sample(float x, float y) const override {
        return std::abs(base_->sample(x, y)) * intensity_;
    }

    float sample(float x, float y, float z) const override {
        return std::abs(base_->sample(x, y, z)) * intensity_;
    }
};

// Specialized continent shape generators
enum class ContinentShape : std::uint8_t {
    Pangaea,    // Single large continent
    Scattered,  // Many small islands
    Archipelago, // Island chains
    Dual,       // Two large landmasses
    Ring        // Continent ring around central sea
};

class Continent final : public Noise {
    std::unique_ptr<Noise> primary_;
    std::unique_ptr<Noise> secondary_;
    ContinentShape shape_;
    float sea_level_;
    Vec2f center_;
    float radius_;

   public:
    Continent(std::unique_ptr<Noise> primary, std::unique_ptr<Noise> secondary,
              ContinentShape shape, float sea_level = 0.3f,
              Vec2f center = Vec2f{0.0F, 0.0F}, float radius = 100.0F)
        : primary_{std::move(primary)},
          secondary_{std::move(secondary)},
          shape_{shape},
          sea_level_{sea_level},
          center_{center},
          radius_{radius} {}

    float sample(float x_coord, float y_coord) const override {
        const Vec2f position{x_coord, y_coord};
        const float distance = Core::Math::distance(position, center_) / radius_;
        
        float base_height = primary_->sample(x_coord, y_coord);
        
        // Apply shape-specific modifications
        switch (shape_) {
            case ContinentShape::Pangaea:
                return apply_pangaea(base_height, distance);
            case ContinentShape::Scattered:
                return apply_scattered(base_height, position);
            case ContinentShape::Archipelago:
                return apply_archipelago(base_height, position);
            case ContinentShape::Dual:
                return apply_dual(base_height, position);
            case ContinentShape::Ring:
                return apply_ring(base_height, distance);
        }
        
        return base_height;
    }

    float sample(float x_coord, float y_coord, float /*z_coord*/) const override {
        return sample(x_coord, y_coord);
    }

   private:
    static float apply_pangaea(float height, float distance) {
        const float falloff = std::max(0.0F, 1.0F - (distance * distance));
        return (height + 1.0F) * 0.5F * falloff;
    }

    float apply_scattered(float height, const Vec2f& position) const {
        const float island_noise = secondary_->sample(position.x() * 0.1F, position.y() * 0.1F);
        const float threshold = 0.6F + sea_level_ * 0.2F;
        return island_noise > threshold ? (height + 1.0F) * 0.5F : sea_level_ * 0.5F;
    }

    float apply_archipelago(float height, const Vec2f& position) const {
        const float chain_pattern = std::sin(position.x() * 0.02F) * std::cos(position.y() * 0.015F);
        const float chain_noise = secondary_->sample(position.x() * 0.05F, position.y() * 0.05F);
        const float combined = chain_pattern + (chain_noise * 0.5F);
        return combined > 0.2F ? (height + 1.0F) * 0.5F : sea_level_ * 0.5F;
    }

    float apply_dual(float height, const Vec2f& position) const {
        const float center1_dist = Core::Math::distance(position, Vec2f{-radius_ * 0.5F, 0.0F}) / radius_;
        const float center2_dist = Core::Math::distance(position, Vec2f{radius_ * 0.5F, 0.0F}) / radius_;
        
        const float falloff1 = std::max(0.0F, 1.0F - (center1_dist * center1_dist));
        const float falloff2 = std::max(0.0F, 1.0F - (center2_dist * center2_dist));
        const float combined_falloff = std::max(falloff1, falloff2);
        
        return (height + 1.0F) * 0.5F * combined_falloff;
    }

    static float apply_ring(float height, float distance) {
        const float ring_center = 0.4F;
        const float ring_width = 0.3F;
        const float ring_falloff = 1.0F - (std::abs(distance - ring_center) / ring_width);
        const float clamped_falloff = std::max(0.0F, ring_falloff);
        
        return (height + 1.0F) * 0.5F * clamped_falloff;
    }
};

}  // namespace Manifest::World::Generation
