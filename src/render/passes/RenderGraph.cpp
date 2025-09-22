#include "RenderGraph.hpp"

#include "../common/Renderer.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

Result<void> RenderGraph::initialize(Renderer* renderer) {
    if (!renderer) {
        return RendererError::InvalidState;
    }

    renderer_ = renderer;

    // Initialize all existing passes
    return initialize_all_passes();
}

void RenderGraph::shutdown() {
    // Shutdown all passes in reverse order
    for (auto it = passes_.rbegin(); it != passes_.rend(); ++it) {
        if (it->pass && it->pass->is_initialized()) {
            it->pass->shutdown();
        }
    }

    passes_.clear();
    renderer_ = nullptr;
    frame_stats_.reset();
}

Result<void> RenderGraph::execute(const PassContext& context) {
    if (!renderer_) {
        return RendererError::InvalidState;
    }

    frame_stats_.reset();

    // Execute all enabled passes in priority order
    for (auto& entry : passes_) {
        if (!entry.enabled || !entry.pass) {
            continue;
        }

        renderer_->push_debug_group(std::string{entry.pass->get_name()});

        auto start_time = std::chrono::high_resolution_clock::now();

        auto result = entry.pass->execute(context);
        if (!result) {
            renderer_->pop_debug_group();
            return result;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        // Create pass result for statistics
        PassResult pass_result{.draw_calls = 0,  // Would be filled by actual pass implementation
                               .vertices_rendered = 0,
                               .gpu_time_ms = static_cast<float>(duration.count()) / 1000.0f,
                               .debug_info = std::string{entry.pass->get_name()}};

        frame_stats_.draw_calls += pass_result.draw_calls;
        frame_stats_.vertices_rendered += pass_result.vertices_rendered;

        renderer_->pop_debug_group();
    }

    return {};
}

Result<void> RenderGraph::initialize_all_passes() {
    for (auto& entry : passes_) {
        if (entry.pass && !entry.pass->is_initialized()) {
            if (auto result = entry.pass->initialize(renderer_); !result) {
                return result;
            }
        }
    }
    return {};
}

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest
