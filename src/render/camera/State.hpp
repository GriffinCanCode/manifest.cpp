#pragma once

#include "../../core/types/Types.hpp"
#include "../../core/types/Modern.hpp"
#include "../../core/math/Vector.hpp"
#include "../../core/graphics/Camera.hpp"
#include "Controls.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace Manifest::Render::CameraSystem {

using Core::Types::StrongId;
using Core::Modern::Result;
using Core::Math::Vec3f;

// Custom error types to avoid Result template ambiguity
struct SerializationError {
    std::string message;
    SerializationError(const std::string& msg) : message(msg) {}
    SerializationError(std::string&& msg) : message(std::move(msg)) {}
};

struct StateError {
    std::string message;
    StateError(const std::string& msg) : message(msg) {}
    StateError(std::string&& msg) : message(std::move(msg)) {}
};

// Strong typing for state management
struct NameTag {};
using StateName = StrongId<NameTag, std::string>;

// Camera state snapshot for save/restore
struct Snapshot {
    Vec3f position{0.0f, 0.0f, 0.0f};
    Vec3f target{0.0f, 0.0f, -1.0f};
    Vec3f up{0.0f, 1.0f, 0.0f};
    float fov{std::numbers::pi_v<float> / 4.0f};
    float aspect_ratio{16.0f / 9.0f};
    float near_plane{0.1f};
    float far_plane{1000.0f};
    Controls::ControlMode control_mode{Controls::ControlMode::Orbital};
    std::chrono::system_clock::time_point timestamp{};
    std::string description{};
    
    // Default constructor
    Snapshot() = default;
    
    // Construct from camera
    explicit Snapshot(const Core::Graphics::Camera& camera, Controls::ControlMode mode = Controls::ControlMode::Orbital, 
                     std::string desc = {}) noexcept;
    
    // Apply to camera
    void apply_to(Core::Graphics::Camera& camera) const noexcept;
    
    // Validation
    bool is_valid() const noexcept;
    
    // Comparison for testing
    bool nearly_equal(const Snapshot& other, float epsilon = 0.001f) const noexcept;
};

// Serialization interface for different formats
class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual Result<std::string, SerializationError> serialize(const Snapshot& snapshot) const noexcept = 0;
    virtual Result<Snapshot, SerializationError> deserialize(const std::string& data) const noexcept = 0;
    virtual std::string format_name() const noexcept = 0;
};

// Binary serializer for performance
class BinarySerializer final : public ISerializer {
public:
    Result<std::string, SerializationError> serialize(const Snapshot& snapshot) const noexcept override;
    Result<Snapshot, SerializationError> deserialize(const std::string& data) const noexcept override;
    std::string format_name() const noexcept override { return "binary"; }
    
private:
    struct Header {
        std::uint32_t magic{0x43414D53}; // "CAMS"
        std::uint32_t version{1};
        std::uint64_t timestamp{};
        std::uint32_t description_length{};
    };
};

// JSON serializer for debugging/config
class JsonSerializer final : public ISerializer {
public:
    Result<std::string, SerializationError> serialize(const Snapshot& snapshot) const noexcept override;
    Result<Snapshot, SerializationError> deserialize(const std::string& data) const noexcept override;
    std::string format_name() const noexcept override { return "json"; }
};

// State manager for save/restore functionality  
namespace State {
class StateManager {
    std::unordered_map<std::string, Snapshot> saved_states_;
    std::vector<std::string> state_history_;
    std::unique_ptr<ISerializer> serializer_;
    std::size_t max_history_size_{100};
    std::string current_state_name_{};
    
public:
    explicit StateManager(std::unique_ptr<ISerializer> serializer = nullptr) noexcept;
    
    // State management
    Result<void, std::string> save_state(const std::string& name, const Core::Graphics::Camera& camera, 
                           Controls::ControlMode mode = Controls::ControlMode::Orbital, 
                           const std::string& description = {}) noexcept;
    
    Result<void, std::string> restore_state(const std::string& name, Core::Graphics::Camera& camera) noexcept;
    
    Result<void, std::string> delete_state(const std::string& name) noexcept;
    
    bool has_state(const std::string& name) const noexcept;
    
    std::vector<std::string> list_states() const noexcept;
    
    Result<Snapshot, std::string> get_snapshot(const std::string& name) const noexcept;
    
    // Auto-save functionality
    Result<void, std::string> auto_save(const Core::Graphics::Camera& camera, Controls::ControlMode mode = Controls::ControlMode::Orbital) noexcept;
    
    // History management
    std::vector<std::string> get_history() const noexcept { return state_history_; }
    
    void clear_history() noexcept;
    
    void set_max_history_size(std::size_t size) noexcept { max_history_size_ = size; }
    
    std::size_t max_history_size() const noexcept { return max_history_size_; }
    
    // Serialization
    Result<std::string, StateError> export_state(const std::string& name) const noexcept;
    
    Result<void, StateError> import_state(const std::string& name, const std::string& data) noexcept;
    
    Result<std::string, StateError> export_all_states() const noexcept;
    
    Result<void, StateError> import_all_states(const std::string& data) noexcept;
    
    // Configuration
    void set_serializer(std::unique_ptr<ISerializer> serializer) noexcept;
    
    const ISerializer* serializer() const noexcept { return serializer_.get(); }
    
    // Statistics
    std::size_t state_count() const noexcept { return saved_states_.size(); }
    
    std::size_t memory_usage() const noexcept;
    
private:
    void add_to_history(const std::string& name) noexcept;
    
    void trim_history() noexcept;
    
    std::string generate_auto_save_name() const noexcept;
};

// Quick state operations using RAII
class StateGuard {
    StateManager& manager_;
    std::string state_name_;
    bool should_restore_;
    
public:
    StateGuard(StateManager& manager, const std::string& name, const Core::Graphics::Camera& camera, 
               bool auto_restore = true, Controls::ControlMode mode = Controls::ControlMode::Orbital,
               const std::string& description = {}) noexcept;
    
    ~StateGuard() noexcept;
    
    // Disable copy, allow move
    StateGuard(const StateGuard&) = delete;
    StateGuard& operator=(const StateGuard&) = delete;
    StateGuard(StateGuard&&) noexcept = default;
    StateGuard& operator=(StateGuard&&) noexcept = delete;
    
    void cancel_restore() noexcept { should_restore_ = false; }
    
    void force_restore(Core::Graphics::Camera& camera) noexcept;
};

// Transition system for smooth state changes
class Transition {
public:
    struct Config {
        float duration{1.0f};  // Duration in seconds
        int easing{2};  // Easing type: 0=Linear, 1=EaseIn, 2=EaseOut, 3=EaseInOut
        bool interpolate_fov{true};
        bool interpolate_clipping{false};
    };
    
private:
    Snapshot start_state_;
    Snapshot end_state_;
    Config config_;
    float elapsed_{0.0f};
    bool complete_{false};
    bool initialized_{false};
    
public:
    Transition(Snapshot start, Snapshot end, Config config) noexcept;
    
    bool update(Core::Graphics::Camera& camera, float delta_time) noexcept;
    
    bool is_complete() const noexcept { return complete_; }
    
    void reset() noexcept;
    
    float progress() const noexcept;
    
private:
    float apply_easing(float t) const noexcept;
    
    Vec3f interpolate_vector(const Vec3f& start, const Vec3f& end, float t) const noexcept;
    
    float interpolate_scalar(float start, float end, float t) const noexcept;
};

} // namespace State

// Preset state configurations
namespace Presets {
    State::StateManager create_default_manager() noexcept;
    
    // Common camera positions
    Snapshot world_overview() noexcept;
    Snapshot close_inspection() noexcept;
    Snapshot strategic_view() noexcept;
    Snapshot cinematic_angle() noexcept;
    
    // Quick save/restore for common operations
    void save_common_states(State::StateManager& manager, const Core::Graphics::Camera& camera) noexcept;
}

} // namespace Manifest::Render::CameraSystem
