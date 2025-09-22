#pragma once

#include "../../core/types/Types.hpp"
#include "../../core/types/Modern.hpp"
#include "../../core/math/Vector.hpp"
#include "../../core/graphics/Camera.hpp"
#include <chrono>
#include <memory>
#include <vector>
#include <functional>

namespace Manifest::Render::CameraSystem {

using namespace Core::Types;
using namespace Core::Modern;
using namespace Core::Math;

// Strong typing for behavior parameters
struct DurationTag {};
struct IntensityTag {};
struct CurveTag {};

using Duration = Quantity<DurationTag, float>;
using Intensity = Quantity<IntensityTag, float>;
using Curve = Quantity<CurveTag, float>;

// Easing functions for smooth transitions
enum class Ease : std::uint8_t {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    Bounce,
    Elastic
};

// Animation target types
struct PositionTarget {
    Vec3f target;
    Duration duration;
    Ease easing{Ease::EaseInOut};
};

struct LookTarget {
    Vec3f target;
    Duration duration;
    Ease easing{Ease::EaseInOut};
};

struct ZoomTarget {
    float distance;
    Duration duration;
    Ease easing{Ease::EaseInOut};
};

struct OrbitTarget {
    float horizontal_angle;
    float vertical_angle;
    Duration duration;
    Ease easing{Ease::EaseInOut};
};

// Behavior interface for extensibility
class IBehavior {
public:
    virtual ~IBehavior() = default;
    virtual bool update(Core::Graphics::Camera& camera, float delta_time) noexcept = 0;
    virtual bool is_complete() const noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual std::unique_ptr<IBehavior> clone() const = 0;
};

// Smooth movement to target position
class MoveTo final : public IBehavior {
    Vec3f start_pos_;
    Vec3f target_pos_;
    Duration duration_;
    Ease easing_;
    float elapsed_{0.0f};
    bool complete_{false};
    
public:
    explicit MoveTo(PositionTarget target) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
    
private:
    float apply_easing(float t) const noexcept;
};

// Smooth look-at transition
class LookAt final : public IBehavior {
    Vec3f start_target_;
    Vec3f end_target_;
    Duration duration_;
    Ease easing_;
    float elapsed_{0.0f};
    bool complete_{false};
    
public:
    explicit LookAt(LookTarget target) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
    
private:
    float apply_easing(float t) const noexcept;
};

// Smooth zoom transition
class ZoomTo final : public IBehavior {
    float start_distance_;
    float target_distance_;
    Duration duration_;
    Ease easing_;
    float elapsed_{0.0f};
    bool complete_{false};
    
public:
    explicit ZoomTo(ZoomTarget target) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
    
private:
    float apply_easing(float t) const noexcept;
    float compute_current_distance(const Core::Graphics::Camera& camera) const noexcept;
};

// Orbital camera movement
class OrbitTo final : public IBehavior {
    float start_horizontal_;
    float start_vertical_;
    float target_horizontal_;
    float target_vertical_;
    Duration duration_;
    Ease easing_;
    float elapsed_{0.0f};
    bool complete_{false};
    
public:
    explicit OrbitTo(OrbitTarget target) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
    
private:
    float apply_easing(float t) const noexcept;
    std::pair<float, float> compute_current_angles(const Core::Graphics::Camera& camera) const noexcept;
};

// Screen shake effect
class Shake final : public IBehavior {
    Intensity intensity_;
    Duration duration_;
    Vec3f original_position_;
    float elapsed_{0.0f};
    bool complete_{false};
    bool initialized_{false};
    
public:
    Shake(Intensity intensity, Duration duration) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
    
private:
    Vec3f generate_shake_offset() const noexcept;
};

// Follow target with smooth tracking
class Follow final : public IBehavior {
    std::function<Vec3f()> target_provider_;
    Vec3f offset_;
    float follow_speed_;
    float distance_;
    bool active_{true};
    
public:
    Follow(std::function<Vec3f()> target_provider, Vec3f offset = {0.0f, 5.0f, 10.0f}, float speed = 2.0f) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return false; } // Never completes
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
    
    void set_active(bool active) noexcept { active_ = active; }
    bool is_active() const noexcept { return active_; }
};

// Sequence of behaviors executed in order
class Sequence final : public IBehavior {
    std::vector<std::unique_ptr<IBehavior>> behaviors_;
    std::size_t current_index_{0};
    bool complete_{false};
    
public:
    template<typename... Behaviors>
    explicit Sequence(Behaviors&&... behaviors) {
        (behaviors_.push_back(std::make_unique<std::decay_t<Behaviors>>(std::forward<Behaviors>(behaviors))), ...);
    }
    
    void add(std::unique_ptr<IBehavior> behavior) {
        if (!complete_) {
            behaviors_.push_back(std::move(behavior));
        }
    }
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
};

// Parallel behaviors executed simultaneously
class Parallel final : public IBehavior {
    std::vector<std::unique_ptr<IBehavior>> behaviors_;
    bool complete_{false};
    
public:
    template<typename... Behaviors>
    explicit Parallel(Behaviors&&... behaviors) {
        (behaviors_.push_back(std::make_unique<std::decay_t<Behaviors>>(std::forward<Behaviors>(behaviors))), ...);
    }
    
    void add(std::unique_ptr<IBehavior> behavior) {
        if (!complete_) {
            behaviors_.push_back(std::move(behavior));
        }
    }
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept override;
    bool is_complete() const noexcept override { return complete_; }
    void reset() noexcept override;
    std::unique_ptr<IBehavior> clone() const override;
};

// Main behavior controller
class Controller {
    std::unique_ptr<IBehavior> current_behavior_;
    bool active_{true};
    
public:
    Controller() = default;
    explicit Controller(std::unique_ptr<IBehavior> behavior) noexcept;
    
    // Behavior management
    void set_behavior(std::unique_ptr<IBehavior> behavior) noexcept;
    void clear_behavior() noexcept;
    bool has_behavior() const noexcept { return current_behavior_ != nullptr; }
    
    // Update system
    void update(Core::Graphics::Camera& camera, float delta_time) noexcept;
    
    // State control
    void set_active(bool active) noexcept { active_ = active; }
    bool is_active() const noexcept { return active_; }
    bool is_complete() const noexcept;
    
    // Reset current behavior
    void reset() noexcept;
};

// Utility functions for common camera movements
namespace Presets {
    std::unique_ptr<MoveTo> fly_to(Vec3f target, Duration duration = Duration{2.0f});
    std::unique_ptr<LookAt> focus_on(Vec3f target, Duration duration = Duration{1.0f});
    std::unique_ptr<ZoomTo> zoom_to_distance(float distance, Duration duration = Duration{1.5f});
    std::unique_ptr<Shake> impact_shake(Intensity intensity = Intensity{5.0f});
    std::unique_ptr<Sequence> cinematic_reveal(Vec3f start_pos, Vec3f end_pos, Vec3f look_target);
} // namespace Presets

} // namespace Manifest::Render::CameraSystem
