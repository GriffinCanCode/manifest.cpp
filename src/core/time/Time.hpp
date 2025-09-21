#pragma once

#include "../types/Types.hpp"
#include <chrono>
#include <concepts>

namespace Manifest::Core::Time {

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::nanoseconds;
using TimePoint = Clock::time_point;

template<typename T>
concept Temporal = requires(T t) {
    { t.update(Duration{}) } -> std::same_as<void>;
};

class GameTime {
    GameTurn current_turn_{0};
    GameYear current_year_{1};
    Duration time_per_turn_{std::chrono::seconds(1)};
    Duration elapsed_turn_time_{};
    bool paused_{false};
    float speed_multiplier_{1.0f};
    
public:
    constexpr GameTime() = default;
    
    void update(Duration delta_time) noexcept {
        if (paused_) return;
        
        Duration scaled_delta = Duration{
            static_cast<Duration::rep>(delta_time.count() * speed_multiplier_)
        };
        
        elapsed_turn_time_ += scaled_delta;
        
        while (elapsed_turn_time_ >= time_per_turn_) {
            elapsed_turn_time_ -= time_per_turn_;
            advance_turn();
        }
    }
    
    void advance_turn() noexcept {
        ++current_turn_;
        // Simple year advancement - could be more sophisticated
        if (current_turn_ % 12 == 0) {
            ++current_year_;
        }
    }
    
    void set_turn(GameTurn turn) noexcept {
        current_turn_ = turn;
        elapsed_turn_time_ = {};
    }
    
    void set_year(GameYear year) noexcept {
        current_year_ = year;
    }
    
    void pause() noexcept { paused_ = true; }
    void resume() noexcept { paused_ = false; }
    void toggle_pause() noexcept { paused_ = !paused_; }
    
    void set_speed(float multiplier) noexcept {
        speed_multiplier_ = std::max(0.0f, multiplier);
    }
    
    void set_turn_duration(Duration duration) noexcept {
        time_per_turn_ = duration;
    }
    
    [[nodiscard]] GameTurn current_turn() const noexcept { return current_turn_; }
    [[nodiscard]] GameYear current_year() const noexcept { return current_year_; }
    [[nodiscard]] Duration turn_progress() const noexcept { return elapsed_turn_time_; }
    [[nodiscard]] float turn_progress_ratio() const noexcept {
        return static_cast<float>(elapsed_turn_time_.count()) / time_per_turn_.count();
    }
    [[nodiscard]] bool is_paused() const noexcept { return paused_; }
    [[nodiscard]] float speed_multiplier() const noexcept { return speed_multiplier_; }
    [[nodiscard]] Duration time_per_turn() const noexcept { return time_per_turn_; }
};

class FrameTimer {
    TimePoint last_frame_{Clock::now()};
    Duration frame_time_{};
    Duration accumulator_{};
    std::size_t frame_count_{};
    float fps_{};
    Duration fps_update_interval_{std::chrono::seconds(1)};
    Duration fps_accumulator_{};
    
public:
    void update() noexcept {
        TimePoint current = Clock::now();
        frame_time_ = current - last_frame_;
        last_frame_ = current;
        
        ++frame_count_;
        fps_accumulator_ += frame_time_;
        
        if (fps_accumulator_ >= fps_update_interval_) {
            fps_ = static_cast<float>(frame_count_) / 
                   std::chrono::duration<float>(fps_accumulator_).count();
            frame_count_ = 0;
            fps_accumulator_ = {};
        }
    }
    
    [[nodiscard]] Duration frame_time() const noexcept { return frame_time_; }
    [[nodiscard]] float fps() const noexcept { return fps_; }
    [[nodiscard]] Duration accumulator() const noexcept { return accumulator_; }
    
    void add_to_accumulator(Duration dt) noexcept {
        accumulator_ += dt;
    }
    
    void consume_accumulator(Duration dt) noexcept {
        accumulator_ -= dt;
    }
    
    void reset_accumulator() noexcept {
        accumulator_ = {};
    }
};

template<typename Func>
class Timer {
    Duration interval_;
    Duration elapsed_{};
    Func callback_;
    bool active_{true};
    bool repeat_{true};
    
public:
    Timer(Duration interval, Func callback, bool repeat = true)
        : interval_{interval}, callback_{std::move(callback)}, repeat_{repeat} {}
    
    void update(Duration delta_time) {
        if (!active_) return;
        
        elapsed_ += delta_time;
        
        if (elapsed_ >= interval_) {
            callback_();
            
            if (repeat_) {
                elapsed_ -= interval_;
            } else {
                active_ = false;
            }
        }
    }
    
    void reset() noexcept {
        elapsed_ = {};
        active_ = true;
    }
    
    void stop() noexcept { active_ = false; }
    void start() noexcept { active_ = true; }
    
    void set_interval(Duration interval) noexcept {
        interval_ = interval;
    }
    
    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] Duration remaining() const noexcept {
        return interval_ > elapsed_ ? interval_ - elapsed_ : Duration{};
    }
    [[nodiscard]] float progress() const noexcept {
        return static_cast<float>(elapsed_.count()) / interval_.count();
    }
};

template<typename Func>
Timer(Duration, Func, bool = true) -> Timer<Func>;

class Profiler {
    struct Sample {
        std::string name;
        Duration duration;
        TimePoint start_time;
    };
    
    std::vector<Sample> samples_;
    std::unordered_map<std::string, Duration> totals_;
    
public:
    class ScopedTimer {
        Profiler* profiler_;
        std::string name_;
        TimePoint start_;
        
    public:
        ScopedTimer(Profiler* profiler, std::string name)
            : profiler_{profiler}, name_{std::move(name)}, start_{Clock::now()} {}
        
        ~ScopedTimer() {
            Duration duration = Clock::now() - start_;
            profiler_->add_sample(name_, duration);
        }
    };
    
    void add_sample(const std::string& name, Duration duration) {
        samples_.emplace_back(name, duration, Clock::now());
        totals_[name] += duration;
    }
    
    [[nodiscard]] ScopedTimer scope(const std::string& name) {
        return ScopedTimer{this, name};
    }
    
    void clear() {
        samples_.clear();
        totals_.clear();
    }
    
    [[nodiscard]] const std::vector<Sample>& samples() const noexcept { return samples_; }
    [[nodiscard]] const std::unordered_map<std::string, Duration>& totals() const noexcept { return totals_; }
};

#define PROFILE_SCOPE(profiler, name) auto _timer = (profiler).scope(name)

// Fixed timestep update system
template<Temporal... Systems>
class FixedTimeStep {
    static constexpr Duration fixed_timestep_{std::chrono::microseconds(16667)}; // ~60 FPS
    Duration accumulator_{};
    std::tuple<Systems*...> systems_;
    
public:
    explicit FixedTimeStep(Systems*... systems) : systems_{systems...} {}
    
    void update(Duration delta_time) {
        accumulator_ += delta_time;
        
        while (accumulator_ >= fixed_timestep_) {
            std::apply([](auto*... systems) {
                (systems->update(fixed_timestep_), ...);
            }, systems_);
            
            accumulator_ -= fixed_timestep_;
        }
    }
    
    [[nodiscard]] static constexpr Duration timestep() noexcept { return fixed_timestep_; }
    [[nodiscard]] Duration accumulator() const noexcept { return accumulator_; }
    [[nodiscard]] float interpolation() const noexcept {
        return static_cast<float>(accumulator_.count()) / fixed_timestep_.count();
    }
};

} // namespace Manifest::Core::Time
