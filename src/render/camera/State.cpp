#include "State.hpp"
#include "ResultHelper.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace Manifest::Render::CameraSystem {

// Snapshot implementation
Snapshot::Snapshot(const Core::Graphics::Camera& camera, Controls::ControlMode mode, std::string desc) noexcept
    : position(camera.position())
    , target(camera.target())
    , up(camera.up())
    , fov(camera.fov())
    , aspect_ratio(camera.aspect_ratio())
    , near_plane(camera.near_plane())
    , far_plane(camera.far_plane())
    , control_mode(mode)
    , timestamp(std::chrono::system_clock::now())
    , description(std::move(desc))
{
}

void Snapshot::apply_to(Core::Graphics::Camera& camera) const noexcept {
    camera.look_at(position, target, up);
    camera.set_perspective(fov, aspect_ratio, near_plane, far_plane);
}

bool Snapshot::is_valid() const noexcept {
    // Check for NaN/Inf values
    auto is_finite_vec3 = [](const Vec3f& v) {
        return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
    };
    
    auto is_finite_scalar = [](float f) {
        return std::isfinite(f);
    };
    
    return is_finite_vec3(position) &&
           is_finite_vec3(target) &&
           is_finite_vec3(up) &&
           is_finite_scalar(fov) &&
           is_finite_scalar(aspect_ratio) &&
           is_finite_scalar(near_plane) &&
           is_finite_scalar(far_plane) &&
           fov > 0.0f && fov < std::numbers::pi_v<float> &&
           aspect_ratio > 0.0f &&
           near_plane > 0.0f &&
           far_plane > near_plane &&
           up.length() > 0.001f; // Up vector shouldn't be zero
}

bool Snapshot::nearly_equal(const Snapshot& other, float epsilon) const noexcept {
    auto vec3_equal = [epsilon](const Vec3f& a, const Vec3f& b) {
        return (a - b).length() < epsilon;
    };
    
    auto scalar_equal = [epsilon](float a, float b) {
        return std::abs(a - b) < epsilon;
    };
    
    return vec3_equal(position, other.position) &&
           vec3_equal(target, other.target) &&
           vec3_equal(up, other.up) &&
           scalar_equal(fov, other.fov) &&
           scalar_equal(aspect_ratio, other.aspect_ratio) &&
           scalar_equal(near_plane, other.near_plane) &&
           scalar_equal(far_plane, other.far_plane) &&
           control_mode == other.control_mode;
}

// BinarySerializer implementation
Result<std::string, SerializationError> BinarySerializer::serialize(const Snapshot& snapshot) const noexcept {
    if (!snapshot.is_valid()) {
        return Result<std::string, SerializationError>{SerializationError{"Invalid snapshot data"}};
    }
    
    try {
        std::ostringstream stream(std::ios::binary);
        
        // Write header
        Header header;
        header.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            snapshot.timestamp.time_since_epoch()).count();
        header.description_length = static_cast<std::uint32_t>(snapshot.description.length());
        
        stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        // Write camera data
        stream.write(reinterpret_cast<const char*>(&snapshot.position), sizeof(Vec3f));
        stream.write(reinterpret_cast<const char*>(&snapshot.target), sizeof(Vec3f));
        stream.write(reinterpret_cast<const char*>(&snapshot.up), sizeof(Vec3f));
        stream.write(reinterpret_cast<const char*>(&snapshot.fov), sizeof(float));
        stream.write(reinterpret_cast<const char*>(&snapshot.aspect_ratio), sizeof(float));
        stream.write(reinterpret_cast<const char*>(&snapshot.near_plane), sizeof(float));
        stream.write(reinterpret_cast<const char*>(&snapshot.far_plane), sizeof(float));
        stream.write(reinterpret_cast<const char*>(&snapshot.control_mode), sizeof(Controls::ControlMode));
        
        // Write description
        if (!snapshot.description.empty()) {
            stream.write(snapshot.description.c_str(), snapshot.description.length());
        }
        
        return Result<std::string, SerializationError>{stream.str()};
    } catch (const std::exception&) {
        return Result<std::string, SerializationError>{SerializationError{"Serialization failed"}};
    }
}

Result<Snapshot, SerializationError> BinarySerializer::deserialize(const std::string& data) const noexcept {
    if (data.size() < sizeof(Header)) {
        return Result<Snapshot, SerializationError>{SerializationError{"Data too short"}};
    }
    
    try {
        std::istringstream stream(data, std::ios::binary);
        
        // Read header
        Header header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (header.magic != 0x43414D53 || header.version != 1) {
            return Result<Snapshot, SerializationError>{SerializationError{"Invalid header"}};
        }
        
        // Read camera data
        Snapshot snapshot{Core::Graphics::Camera{}};
        stream.read(reinterpret_cast<char*>(&snapshot.position), sizeof(Vec3f));
        stream.read(reinterpret_cast<char*>(&snapshot.target), sizeof(Vec3f));
        stream.read(reinterpret_cast<char*>(&snapshot.up), sizeof(Vec3f));
        stream.read(reinterpret_cast<char*>(&snapshot.fov), sizeof(float));
        stream.read(reinterpret_cast<char*>(&snapshot.aspect_ratio), sizeof(float));
        stream.read(reinterpret_cast<char*>(&snapshot.near_plane), sizeof(float));
        stream.read(reinterpret_cast<char*>(&snapshot.far_plane), sizeof(float));
        stream.read(reinterpret_cast<char*>(&snapshot.control_mode), sizeof(Controls::ControlMode));
        
        // Read description
        if (header.description_length > 0) {
            snapshot.description.resize(header.description_length);
            stream.read(snapshot.description.data(), header.description_length);
        }
        
        // Restore timestamp
        snapshot.timestamp = std::chrono::system_clock::from_time_t(header.timestamp);
        
        if (!snapshot.is_valid()) {
            return Result<Snapshot, SerializationError>{SerializationError{"Deserialized data is invalid"}};
        }
        
        return Result<Snapshot, SerializationError>{std::move(snapshot)};
    } catch (const std::exception&) {
        return Result<Snapshot, SerializationError>{SerializationError{"Deserialization failed"}};
    }
}

// JsonSerializer implementation  
Result<std::string, SerializationError> JsonSerializer::serialize(const Snapshot& snapshot) const noexcept {
    if (!snapshot.is_valid()) {
        return Result<std::string, SerializationError>{SerializationError{"Invalid snapshot data"}};
    }
    
    try {
        std::ostringstream json;
        json << std::fixed << std::setprecision(6);
        
        json << "{\n";
        json << "  \"position\": [" << snapshot.position.x() << ", " << snapshot.position.y() << ", " << snapshot.position.z() << "],\n";
        json << "  \"target\": [" << snapshot.target.x() << ", " << snapshot.target.y() << ", " << snapshot.target.z() << "],\n";
        json << "  \"up\": [" << snapshot.up.x() << ", " << snapshot.up.y() << ", " << snapshot.up.z() << "],\n";
        json << "  \"fov\": " << snapshot.fov << ",\n";
        json << "  \"aspect_ratio\": " << snapshot.aspect_ratio << ",\n";
        json << "  \"near_plane\": " << snapshot.near_plane << ",\n";
        json << "  \"far_plane\": " << snapshot.far_plane << ",\n";
        json << "  \"control_mode\": " << static_cast<int>(snapshot.control_mode) << ",\n";
        json << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(snapshot.timestamp.time_since_epoch()).count() << ",\n";
        json << "  \"description\": \"" << snapshot.description << "\"\n";
        json << "}";
        
        return Result<std::string, SerializationError>{json.str()};
    } catch (const std::exception&) {
        return Result<std::string, SerializationError>{SerializationError{"JSON serialization failed"}};
    }
}

Result<Snapshot, SerializationError> JsonSerializer::deserialize(const std::string& data) const noexcept {
    // Simple JSON parser (would use proper library in production)
    try {
        Snapshot snapshot{Core::Graphics::Camera{}};
        
        // This is a simplified parser - in production would use rapidjson or similar
        auto extract_vec3 = [&data](const std::string& key) -> Vec3f {
            auto pos = data.find("\"" + key + "\"");
            if (pos == std::string::npos) return Vec3f{};
            
            auto start = data.find('[', pos);
            auto end = data.find(']', start);
            std::string vec_str = data.substr(start + 1, end - start - 1);
            
            std::istringstream iss(vec_str);
            std::string token;
            Vec3f result;
            
            std::getline(iss, token, ',');
            result.x() = std::stof(token);
            std::getline(iss, token, ',');
            result.y() = std::stof(token);
            std::getline(iss, token);
            result.z() = std::stof(token);
            
            return result;
        };
        
        auto extract_float = [&data](const std::string& key) -> float {
            auto pos = data.find("\"" + key + "\"");
            if (pos == std::string::npos) return 0.0f;
            
            auto colon = data.find(':', pos);
            auto comma = data.find(',', colon);
            if (comma == std::string::npos) comma = data.find('}', colon);
            
            std::string value_str = data.substr(colon + 1, comma - colon - 1);
            value_str.erase(value_str.find_last_not_of(" \t\n\r") + 1);
            value_str.erase(0, value_str.find_first_not_of(" \t\n\r"));
            
            return std::stof(value_str);
        };
        
        snapshot.position = extract_vec3("position");
        snapshot.target = extract_vec3("target");
        snapshot.up = extract_vec3("up");
        snapshot.fov = extract_float("fov");
        snapshot.aspect_ratio = extract_float("aspect_ratio");
        snapshot.near_plane = extract_float("near_plane");
        snapshot.far_plane = extract_float("far_plane");
        snapshot.control_mode = static_cast<Controls::ControlMode>(static_cast<int>(extract_float("control_mode")));
        
        // Extract timestamp and description (simplified)
        snapshot.timestamp = std::chrono::system_clock::from_time_t(static_cast<std::time_t>(extract_float("timestamp")));
        
        if (!snapshot.is_valid()) {
            return Result<Snapshot, SerializationError>{SerializationError{"Deserialized JSON data is invalid"}};
        }
        
        return Result<Snapshot, SerializationError>{std::move(snapshot)};
    } catch (const std::exception&) {
        return Result<Snapshot, SerializationError>{SerializationError{"JSON deserialization failed"}};
    }
}

// StateManager implementation
State::StateManager::StateManager(std::unique_ptr<ISerializer> serializer) noexcept
    : serializer_(serializer ? std::move(serializer) : std::make_unique<BinarySerializer>())
{
}

Result<void, std::string> State::StateManager::save_state(const std::string& name, const Core::Graphics::Camera& camera, 
                                Controls::ControlMode mode, const std::string& description) noexcept {
    if (name.empty()) {
        return Result<void, std::string>{"State name cannot be empty"};
    }
    
    Snapshot snapshot(camera, mode, description);
    if (!snapshot.is_valid()) {
        return Result<void, std::string>{"Invalid camera state"};
    }
    
    saved_states_[name] = std::move(snapshot);
    add_to_history(name);
    current_state_name_ = name;
    
    return Result<void, std::string>{};
}

Result<void, std::string> State::StateManager::restore_state(const std::string& name, Core::Graphics::Camera& camera) noexcept {
    auto it = saved_states_.find(name);
    if (it == saved_states_.end()) {
        return Result<void, std::string>{"State not found: " + name};
    }
    
    it->second.apply_to(camera);
    current_state_name_ = name;
    
    return Result<void, std::string>{};
}

Result<void, std::string> State::StateManager::delete_state(const std::string& name) noexcept {
    auto it = saved_states_.find(name);
    if (it == saved_states_.end()) {
        return Result<void, std::string>{"State not found: " + name};
    }
    
    saved_states_.erase(it);
    
    // Remove from history
    auto hist_it = std::find(state_history_.begin(), state_history_.end(), name);
    if (hist_it != state_history_.end()) {
        state_history_.erase(hist_it);
    }
    
    if (current_state_name_ == name) {
        current_state_name_.clear();
    }
    
    return Result<void, std::string>{};
}

bool State::StateManager::has_state(const std::string& name) const noexcept {
    return saved_states_.find(name) != saved_states_.end();
}

std::vector<std::string> State::StateManager::list_states() const noexcept {
    std::vector<std::string> names;
    names.reserve(saved_states_.size());
    
    for (const auto& [name, snapshot] : saved_states_) {
        names.push_back(name);
    }
    
    return names;
}

Result<Snapshot, std::string> State::StateManager::get_snapshot(const std::string& name) const noexcept {
    auto it = saved_states_.find(name);
    if (it == saved_states_.end()) {
        return Result<Snapshot, std::string>{"State not found: " + name};
    }
    
    return Result<Snapshot, std::string>{it->second};
}

Result<void, std::string> State::StateManager::auto_save(const Core::Graphics::Camera& camera, Controls::ControlMode mode) noexcept {
    std::string auto_name = generate_auto_save_name();
    return save_state(auto_name, camera, mode, "Auto-saved state");
}

void State::StateManager::clear_history() noexcept {
    state_history_.clear();
}

Result<std::string, StateError> State::StateManager::export_state(const std::string& name) const noexcept {
    auto it = saved_states_.find(name);
    if (it == saved_states_.end()) {
        return Result<std::string, StateError>{StateError{"State not found: " + name}};
    }
    
    auto result = serializer_->serialize(it->second);
    if (result.has_value()) {
        return Result<std::string, StateError>{*result};
    } else {
        return Result<std::string, StateError>{StateError{result.error().message}};
    }
}

Result<void, StateError> State::StateManager::import_state(const std::string& name, const std::string& data) noexcept {
    auto snapshot_result = serializer_->deserialize(data);
    if (!snapshot_result.has_value()) {
        return Result<void, StateError>{StateError{"Failed to deserialize state: " + snapshot_result.error().message}};
    }
    
    saved_states_[name] = std::move(*snapshot_result);
    add_to_history(name);
    
    return Result<void, StateError>{};
}

Result<std::string, StateError> State::StateManager::export_all_states() const noexcept {
    // Export as JSON array
    try {
        std::ostringstream json;
        json << "{\n  \"states\": [\n";
        
        bool first = true;
        for (const auto& [name, snapshot] : saved_states_) {
            if (!first) json << ",\n";
            first = false;
            
            auto serialized = serializer_->serialize(snapshot);
            if (!serialized.has_value()) {
                return Result<std::string, StateError>{StateError{"Failed to serialize state: " + name}};
            }
            
            json << "    {\n      \"name\": \"" << name << "\",\n";
            json << "      \"data\": \"" << *serialized << "\"\n    }";
        }
        
        json << "\n  ]\n}";
        return Result<std::string, StateError>{json.str()};
    } catch (const std::exception&) {
        return Result<std::string, StateError>{StateError{"Export failed"}};
    }
}

Result<void, StateError> State::StateManager::import_all_states(const std::string& data) noexcept {
    // Simplified JSON parser for import (would use proper library in production)
    try {
        // This would be properly implemented with rapidjson or similar
        return Result<void, StateError>{StateError{"Import not implemented (needs JSON library)"}};
    } catch (const std::exception&) {
        return Result<void, StateError>{StateError{"Import failed"}};
    }
}

void State::StateManager::set_serializer(std::unique_ptr<ISerializer> serializer) noexcept {
    if (serializer) {
        serializer_ = std::move(serializer);
    }
}

std::size_t State::StateManager::memory_usage() const noexcept {
    std::size_t total = 0;
    for (const auto& [name, snapshot] : saved_states_) {
        total += sizeof(snapshot) + name.length() + snapshot.description.length();
    }
    return total;
}

void State::StateManager::add_to_history(const std::string& name) noexcept {
    // Remove existing entry
    auto it = std::find(state_history_.begin(), state_history_.end(), name);
    if (it != state_history_.end()) {
        state_history_.erase(it);
    }
    
    // Add to front
    state_history_.insert(state_history_.begin(), name);
    trim_history();
}

void State::StateManager::trim_history() noexcept {
    if (state_history_.size() > max_history_size_) {
        state_history_.resize(max_history_size_);
    }
}

std::string State::StateManager::generate_auto_save_name() const noexcept {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream name;
    name << "auto_save_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return name.str();
}

// StateGuard implementation
State::StateGuard::StateGuard(State::StateManager& manager, const std::string& name, const Core::Graphics::Camera& camera,
                      bool auto_restore, Controls::ControlMode mode, const std::string& description) noexcept
    : manager_(manager)
    , state_name_(name)
    , should_restore_(auto_restore)
{
    manager_.save_state(name, camera, mode, description);
}

State::StateGuard::~StateGuard() noexcept {
    if (should_restore_) {
        // Can't restore without camera reference - would need different design for automatic restoration
    }
}

void State::StateGuard::force_restore(Core::Graphics::Camera& camera) noexcept {
    manager_.restore_state(state_name_, camera);
    should_restore_ = false;
}

// Transition implementation
State::Transition::Transition(Snapshot start, Snapshot end, Config config) noexcept
    : start_state_(std::move(start))
    , end_state_(std::move(end))
    , config_(config)
{
}

bool State::Transition::update(Core::Graphics::Camera& camera, float delta_time) noexcept {
    if (complete_) return true;
    
    if (!initialized_) {
        start_state_.apply_to(camera);
        initialized_ = true;
    }
    
    elapsed_ += delta_time;
    float t = std::clamp(elapsed_ / config_.duration, 0.0f, 1.0f);
    float eased_t = apply_easing(t);
    
    // Interpolate position, target, up
    Vec3f pos = interpolate_vector(start_state_.position, end_state_.position, eased_t);
    Vec3f target = interpolate_vector(start_state_.target, end_state_.target, eased_t);
    Vec3f up = interpolate_vector(start_state_.up, end_state_.up, eased_t);
    
    camera.look_at(pos, target, up.normalized());
    
    // Optionally interpolate other parameters
    if (config_.interpolate_fov) {
        float fov = interpolate_scalar(start_state_.fov, end_state_.fov, eased_t);
        camera.set_fov(fov);
    }
    
    if (config_.interpolate_clipping) {
        float near_p = interpolate_scalar(start_state_.near_plane, end_state_.near_plane, eased_t);
        float far_p = interpolate_scalar(start_state_.far_plane, end_state_.far_plane, eased_t);
        camera.set_near_plane(near_p);
        camera.set_far_plane(far_p);
    }
    
    complete_ = (t >= 1.0f);
    return complete_;
}

void State::Transition::reset() noexcept {
    elapsed_ = 0.0f;
    complete_ = false;
    initialized_ = false;
}

float State::Transition::progress() const noexcept {
    return std::clamp(elapsed_ / config_.duration, 0.0f, 1.0f);
}

float State::Transition::apply_easing(float t) const noexcept {
    // Simple easing functions
    switch (config_.easing) {
        case 0: return t; // Linear
        case 1: return t * t; // EaseIn
        case 2: return 1.0f - (1.0f - t) * (1.0f - t); // EaseOut
        case 3:
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f; // EaseInOut
        default: return t;
    }
}

Vec3f State::Transition::interpolate_vector(const Vec3f& start, const Vec3f& end, float t) const noexcept {
    return start + (end - start) * t;
}

float State::Transition::interpolate_scalar(float start, float end, float t) const noexcept {
    return start + (end - start) * t;
}

// Presets implementation
namespace Presets {

State::StateManager create_default_manager() noexcept {
    return State::StateManager(std::make_unique<BinarySerializer>());
}

Snapshot world_overview() noexcept {
    Core::Graphics::Camera camera;
    camera.look_at(Vec3f{0.0f, 100.0f, 100.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    return Snapshot(camera, Controls::ControlMode::Orbital, "World overview");
}

Snapshot close_inspection() noexcept {
    Core::Graphics::Camera camera;
    camera.look_at(Vec3f{5.0f, 5.0f, 5.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    return Snapshot(camera, Controls::ControlMode::Free, "Close inspection");
}

Snapshot strategic_view() noexcept {
    Core::Graphics::Camera camera;
    camera.look_at(Vec3f{0.0f, 50.0f, 0.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    return Snapshot(camera, Controls::ControlMode::Orbital, "Strategic overview");
}

Snapshot cinematic_angle() noexcept {
    Core::Graphics::Camera camera;
    camera.look_at(Vec3f{20.0f, 10.0f, 30.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    return Snapshot(camera, Controls::ControlMode::Cinematic, "Cinematic angle");
}

void save_common_states(State::StateManager& manager, const Core::Graphics::Camera& camera) noexcept {
    manager.save_state("world_overview", camera, Controls::ControlMode::Orbital, "Default world view");
    manager.save_state("close_up", camera, Controls::ControlMode::Free, "Detail inspection");
    manager.save_state("strategic", camera, Controls::ControlMode::Orbital, "Strategic planning");
}

} // namespace Presets

} // namespace Manifest::Render::CameraSystem
