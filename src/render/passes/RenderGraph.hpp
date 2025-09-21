#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>

#include "RenderPass.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

/**
 * Modern render graph implementation for 2025.
 * Manages render pass dependencies and execution order automatically.
 * Uses frame-based scheduling for optimal GPU utilization.
 */
class RenderGraph {
   public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    // Non-copyable but movable
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = default;
    RenderGraph& operator=(RenderGraph&&) = default;

    /**
     * Initialize the render graph with a renderer
     */
    [[nodiscard]] Result<void> initialize(Renderer* renderer);

    /**
     * Shutdown and cleanup all passes
     */
    void shutdown();

    /**
     * Add a render pass to the graph
     */
    template <typename PassType, typename... Args>
    PassType* add_pass(Args&&... args) {
        auto pass = std::make_unique<PassType>(std::forward<Args>(args)...);
        PassType* pass_ptr = pass.get();

        RenderPassEntry entry{
            .pass = std::move(pass), .priority = pass_ptr->get_priority(), .enabled = true};

        passes_.emplace_back(std::move(entry));

        // Re-sort passes by priority
        std::sort(passes_.begin(), passes_.end(),
                  [](const RenderPassEntry& a, const RenderPassEntry& b) {
                      return a.priority < b.priority;
                  });

        // Initialize the pass if graph is already initialized
        if (renderer_ && !pass_ptr->is_initialized()) {
            if (auto result = pass_ptr->initialize(renderer_); !result) {
                // Remove the pass if initialization failed
                passes_.erase(std::remove_if(passes_.begin(), passes_.end(),
                                             [pass_ptr](const RenderPassEntry& entry) {
                                                 return entry.pass.get() == pass_ptr;
                                             }),
                              passes_.end());
                return nullptr;
            }
        }

        return pass_ptr;
    }

    /**
     * Find a render pass by type
     */
    template <typename PassType>
    PassType* find_pass() {
        for (auto& entry : passes_) {
            if (auto* pass = dynamic_cast<PassType*>(entry.pass.get())) {
                return pass;
            }
        }
        return nullptr;
    }

    /**
     * Enable or disable a render pass
     */
    template <typename PassType>
    void set_pass_enabled(bool enabled) {
        for (auto& entry : passes_) {
            if (dynamic_cast<PassType*>(entry.pass.get())) {
                entry.enabled = enabled;
                break;
            }
        }
    }

    /**
     * Execute all enabled passes in priority order
     */
    [[nodiscard]] Result<void> execute(const PassContext& context);

    /**
     * Get frame statistics
     */
    [[nodiscard]] const FrameStats& get_frame_stats() const noexcept { return frame_stats_; }

    /**
     * Reset statistics
     */
    void reset_stats() noexcept { frame_stats_ = {}; }

    /**
     * Check if graph is initialized
     */
    [[nodiscard]] bool is_initialized() const noexcept { return renderer_ != nullptr; }

   private:
    struct RenderPassEntry {
        std::unique_ptr<RenderPass> pass;
        std::uint32_t priority;
        bool enabled;
    };

    struct FrameStats {
        std::uint32_t total_passes_executed{0};
        std::uint32_t total_draw_calls{0};
        std::uint32_t total_vertices_rendered{0};
        float total_gpu_time_ms{0.0f};

        void reset() noexcept { *this = {}; }

        void add_pass_stats(const PassResult& result) noexcept {
            total_passes_executed++;
            total_draw_calls += result.draw_calls;
            total_vertices_rendered += result.vertices_rendered;
            total_gpu_time_ms += result.gpu_time_ms;
        }
    };

    Renderer* renderer_{nullptr};
    std::vector<RenderPassEntry> passes_;
    FrameStats frame_stats_{};

    [[nodiscard]] Result<void> initialize_all_passes();
};

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest
