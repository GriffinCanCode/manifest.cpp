#include "Behavior.hpp"
#include <cmath>
#include <random>
#include <algorithm>

namespace Manifest::Render::CameraSystem {

namespace {
    // Utility function for easing calculations
    float ease(float t, Ease type) noexcept {
        switch (type) {
            case Ease::Linear:
                return t;
            case Ease::EaseIn:
                return t * t;
            case Ease::EaseOut:
                return 1.0f - (1.0f - t) * (1.0f - t);
            case Ease::EaseInOut:
                return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
            case Ease::Bounce:
                if (t < 1.0f / 2.75f) {
                    return 7.5625f * t * t;
                } else if (t < 2.0f / 2.75f) {
                    t -= 1.5f / 2.75f;
                    return 7.5625f * t * t + 0.75f;
                } else if (t < 2.5f / 2.75f) {
                    t -= 2.25f / 2.75f;
                    return 7.5625f * t * t + 0.9375f;
                } else {
                    t -= 2.625f / 2.75f;
                    return 7.5625f * t * t + 0.984375f;
                }
            case Ease::Elastic:
                if (t == 0.0f) return 0.0f;
                if (t == 1.0f) return 1.0f;
                return -std::pow(2.0f, 10.0f * (t - 1.0f)) * 
                       std::sin((t - 1.1f) * (2.0f * std::numbers::pi_v<float>) / 0.4f);
            default:
                return t;
        }
    }

    // Random number generator for shake effect
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
}

// MoveTo implementation
MoveTo::MoveTo(PositionTarget target) noexcept 
    : target_pos_(target.target)
    , duration_(target.duration) 
    , easing_(target.easing)
{
}

bool MoveTo::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_) return true;
    
    if (elapsed_ == 0.0f) {
        start_pos_ = camera.position();
    }
    
    elapsed_ += delta_time;
    float t = std::clamp(elapsed_ / duration_.value(), 0.0f, 1.0f);
    float eased_t = apply_easing(t);
    
    Vec3f current_pos = start_pos_ + (target_pos_ - start_pos_) * eased_t;
    camera.set_position(current_pos);
    
    complete_ = (t >= 1.0f);
    return complete_;
}

void MoveTo::reset() noexcept {
    elapsed_ = 0.0f;
    complete_ = false;
}

std::unique_ptr<IBehavior> MoveTo::clone() const {
    auto clone = std::make_unique<MoveTo>(PositionTarget{target_pos_, duration_, easing_});
    clone->start_pos_ = start_pos_;
    clone->elapsed_ = elapsed_;
    clone->complete_ = complete_;
    return clone;
}

float MoveTo::apply_easing(float t) const noexcept {
    return ease(t, easing_);
}

// LookAt implementation
LookAt::LookAt(LookTarget target) noexcept
    : end_target_(target.target)
    , duration_(target.duration)
    , easing_(target.easing)
{
}

bool LookAt::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_) return true;
    
    if (elapsed_ == 0.0f) {
        start_target_ = camera.target();
    }
    
    elapsed_ += delta_time;
    float t = std::clamp(elapsed_ / duration_.value(), 0.0f, 1.0f);
    float eased_t = apply_easing(t);
    
    Vec3f current_target = start_target_ + (end_target_ - start_target_) * eased_t;
    camera.set_target(current_target);
    
    complete_ = (t >= 1.0f);
    return complete_;
}

void LookAt::reset() noexcept {
    elapsed_ = 0.0f;
    complete_ = false;
}

std::unique_ptr<IBehavior> LookAt::clone() const {
    auto clone = std::make_unique<LookAt>(LookTarget{end_target_, duration_, easing_});
    clone->start_target_ = start_target_;
    clone->elapsed_ = elapsed_;
    clone->complete_ = complete_;
    return clone;
}

float LookAt::apply_easing(float t) const noexcept {
    return ease(t, easing_);
}

// ZoomTo implementation
ZoomTo::ZoomTo(ZoomTarget target) noexcept
    : target_distance_(target.distance)
    , duration_(target.duration)
    , easing_(target.easing)
{
}

bool ZoomTo::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_) return true;
    
    if (elapsed_ == 0.0f) {
        start_distance_ = compute_current_distance(camera);
    }
    
    elapsed_ += delta_time;
    float t = std::clamp(elapsed_ / duration_.value(), 0.0f, 1.0f);
    float eased_t = apply_easing(t);
    
    float current_distance = start_distance_ + (target_distance_ - start_distance_) * eased_t;
    float zoom_factor = current_distance / compute_current_distance(camera);
    camera.zoom(zoom_factor);
    
    complete_ = (t >= 1.0f);
    return complete_;
}

void ZoomTo::reset() noexcept {
    elapsed_ = 0.0f;
    complete_ = false;
}

std::unique_ptr<IBehavior> ZoomTo::clone() const {
    auto clone = std::make_unique<ZoomTo>(ZoomTarget{target_distance_, duration_, easing_});
    clone->start_distance_ = start_distance_;
    clone->elapsed_ = elapsed_;
    clone->complete_ = complete_;
    return clone;
}

float ZoomTo::apply_easing(float t) const noexcept {
    return ease(t, easing_);
}

float ZoomTo::compute_current_distance(const Core::Graphics::Camera& camera) const noexcept {
    return (camera.position() - camera.target()).length();
}

// OrbitTo implementation
OrbitTo::OrbitTo(OrbitTarget target) noexcept
    : target_horizontal_(target.horizontal_angle)
    , target_vertical_(target.vertical_angle)
    , duration_(target.duration)
    , easing_(target.easing)
{
}

bool OrbitTo::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_) return true;
    
    if (elapsed_ == 0.0f) {
        auto [h, v] = compute_current_angles(camera);
        start_horizontal_ = h;
        start_vertical_ = v;
    }
    
    elapsed_ += delta_time;
    float t = std::clamp(elapsed_ / duration_.value(), 0.0f, 1.0f);
    float eased_t = apply_easing(t);
    
    float h_delta = (target_horizontal_ - start_horizontal_) * eased_t;
    float v_delta = (target_vertical_ - start_vertical_) * eased_t;
    
    camera.orbit_horizontal(h_delta);
    camera.orbit_vertical(v_delta);
    
    complete_ = (t >= 1.0f);
    return complete_;
}

void OrbitTo::reset() noexcept {
    elapsed_ = 0.0f;
    complete_ = false;
}

std::unique_ptr<IBehavior> OrbitTo::clone() const {
    auto clone = std::make_unique<OrbitTo>(OrbitTarget{target_horizontal_, target_vertical_, duration_, easing_});
    clone->start_horizontal_ = start_horizontal_;
    clone->start_vertical_ = start_vertical_;
    clone->elapsed_ = elapsed_;
    clone->complete_ = complete_;
    return clone;
}

float OrbitTo::apply_easing(float t) const noexcept {
    return ease(t, easing_);
}

std::pair<float, float> OrbitTo::compute_current_angles(const Core::Graphics::Camera& camera) const noexcept {
    Vec3f direction = camera.position() - camera.target();
    float horizontal = std::atan2(direction.x(), direction.z());
    float vertical = std::asin(direction.y() / direction.length());
    return {horizontal, vertical};
}

// Shake implementation
Shake::Shake(Intensity intensity, Duration duration) noexcept
    : intensity_(intensity)
    , duration_(duration)
{
}

bool Shake::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_) return true;
    
    if (!initialized_) {
        original_position_ = camera.position();
        initialized_ = true;
    }
    
    elapsed_ += delta_time;
    float t = elapsed_ / duration_.value();
    
    if (t >= 1.0f) {
        camera.set_position(original_position_);
        complete_ = true;
        return true;
    }
    
    // Decrease intensity over time
    float current_intensity = intensity_.value() * (1.0f - t);
    Vec3f shake_offset = generate_shake_offset() * current_intensity;
    camera.set_position(original_position_ + shake_offset);
    
    return false;
}

void Shake::reset() noexcept {
    elapsed_ = 0.0f;
    complete_ = false;
    initialized_ = false;
}

std::unique_ptr<IBehavior> Shake::clone() const {
    auto clone = std::make_unique<Shake>(intensity_, duration_);
    clone->original_position_ = original_position_;
    clone->elapsed_ = elapsed_;
    clone->complete_ = complete_;
    clone->initialized_ = initialized_;
    return clone;
}

Vec3f Shake::generate_shake_offset() const noexcept {
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    return Vec3f{dist(gen), dist(gen), dist(gen)};
}

// Follow implementation
Follow::Follow(std::function<Vec3f()> target_provider, Vec3f offset, float speed) noexcept
    : target_provider_(std::move(target_provider))
    , offset_(offset)
    , follow_speed_(speed)
{
}

bool Follow::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (!active_ || !target_provider_) return false;
    
    Vec3f target_pos = target_provider_();
    Vec3f desired_pos = target_pos + offset_;
    Vec3f current_pos = camera.position();
    
    Vec3f direction = desired_pos - current_pos;
    float distance = direction.length();
    
    if (distance > 0.001f) {
        Vec3f move_delta = direction.normalized() * follow_speed_ * delta_time * distance;
        camera.set_position(current_pos + move_delta);
        camera.set_target(target_pos);
    }
    
    return false; // Never completes
}

void Follow::reset() noexcept {
    // Follow behavior doesn't need resetting
}

std::unique_ptr<IBehavior> Follow::clone() const {
    auto clone = std::make_unique<Follow>(target_provider_, offset_, follow_speed_);
    clone->active_ = active_;
    return clone;
}

// Sequence implementation
bool Sequence::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_ || behaviors_.empty()) return true;
    
    while (current_index_ < behaviors_.size()) {
        auto& current = behaviors_[current_index_];
        if (!current->update(camera, delta_time)) {
            return false; // Still running
        }
        
        if (current->is_complete()) {
            ++current_index_;
        } else {
            return false; // Still running
        }
    }
    
    complete_ = true;
    return true;
}

void Sequence::reset() noexcept {
    current_index_ = 0;
    complete_ = false;
    for (auto& behavior : behaviors_) {
        behavior->reset();
    }
}

std::unique_ptr<IBehavior> Sequence::clone() const {
    auto clone = std::make_unique<Sequence>();
    for (const auto& behavior : behaviors_) {
        clone->behaviors_.push_back(behavior->clone());
    }
    clone->current_index_ = current_index_;
    clone->complete_ = complete_;
    return clone;
}

// Parallel implementation
bool Parallel::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_ || behaviors_.empty()) return true;
    
    bool any_running = false;
    for (auto& behavior : behaviors_) {
        if (!behavior->is_complete()) {
            behavior->update(camera, delta_time);
            if (!behavior->is_complete()) {
                any_running = true;
            }
        }
    }
    
    complete_ = !any_running;
    return complete_;
}

void Parallel::reset() noexcept {
    complete_ = false;
    for (auto& behavior : behaviors_) {
        behavior->reset();
    }
}

std::unique_ptr<IBehavior> Parallel::clone() const {
    auto clone = std::make_unique<Parallel>();
    for (const auto& behavior : behaviors_) {
        clone->behaviors_.push_back(behavior->clone());
    }
    clone->complete_ = complete_;
    return clone;
}

// Controller implementation
Controller::Controller(std::unique_ptr<IBehavior> behavior) noexcept
    : current_behavior_(std::move(behavior))
{
}

void Controller::set_behavior(std::unique_ptr<IBehavior> behavior) noexcept {
    current_behavior_ = std::move(behavior);
}

void Controller::clear_behavior() noexcept {
    current_behavior_.reset();
}

void Controller::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (!active_ || !current_behavior_) return;
    
    current_behavior_->update(camera, delta_time);
}

bool Controller::is_complete() const noexcept {
    return !current_behavior_ || current_behavior_->is_complete();
}

void Controller::reset() noexcept {
    if (current_behavior_) {
        current_behavior_->reset();
    }
}

// Preset implementations
namespace Presets {

std::unique_ptr<MoveTo> fly_to(Vec3f target, Duration duration) {
    return std::make_unique<MoveTo>(PositionTarget{target, duration, Ease::EaseInOut});
}

std::unique_ptr<LookAt> focus_on(Vec3f target, Duration duration) {
    return std::make_unique<LookAt>(LookTarget{target, duration, Ease::EaseInOut});
}

std::unique_ptr<ZoomTo> zoom_to_distance(float distance, Duration duration) {
    return std::make_unique<ZoomTo>(ZoomTarget{distance, duration, Ease::EaseOut});
}

std::unique_ptr<Shake> impact_shake(Intensity intensity) {
    return std::make_unique<Shake>(intensity, Duration{0.5f});
}

std::unique_ptr<Sequence> cinematic_reveal(Vec3f start_pos, Vec3f end_pos, Vec3f look_target) {
    return std::make_unique<Sequence>(
        MoveTo{PositionTarget{start_pos, Duration{0.1f}, Ease::Linear}},
        Parallel{
            MoveTo{PositionTarget{end_pos, Duration{3.0f}, Ease::EaseInOut}},
            LookAt{LookTarget{look_target, Duration{2.0f}, Ease::EaseOut}}
        }
    );
}

} // namespace Presets

} // namespace Manifest::Render::CameraSystem
