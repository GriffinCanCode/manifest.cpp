#include "HexRenderer.hpp"

#include <cstddef>
#include <vector>

#include "Renderer.hpp"

namespace Manifest {
namespace Render {

using namespace World::Tiles;

bool HexRenderer::initialize(std::unique_ptr<Renderer> renderer, RenderingMode mode) {
    mode_ = mode;

    // Determine the actual rendering approach to use
    determine_rendering_mode();

    if (use_procedural_) {
        // Initialize procedural renderer
        procedural_renderer_ = std::make_unique<ProceduralHexRenderer>();
        // TODO: Handle initialization result properly
        procedural_renderer_->initialize(std::move(renderer));
        return true;
    }

    // Legacy rendering doesn't need special initialization
    return true;
}

void HexRenderer::shutdown() {
    if (procedural_renderer_) {
        procedural_renderer_->shutdown();
        procedural_renderer_.reset();
    }

    use_procedural_ = false;
}

void HexRenderer::update_camera(const Mat4f& view_proj, const Vec3f& camera_pos, float time) {
    if (use_procedural_ && procedural_renderer_) {
        procedural_renderer_->update_globals(view_proj, camera_pos, time);
    }
}

void HexRenderer::update_lighting(const Vec3f& sun_dir, const Vec3f& sun_color,
                                  const Vec3f& ambient_color, float sun_intensity) {
    if (use_procedural_ && procedural_renderer_) {
        procedural_renderer_->update_lighting(sun_dir, sun_color, ambient_color, sun_intensity);
    }
}

bool HexRenderer::render_tiles(const std::vector<const Tile*>& tiles) {
    if (use_procedural_ && procedural_renderer_) {
        // Use modern GPU-based procedural rendering
        procedural_renderer_->prepare_instances(tiles);
        // TODO: Handle render result properly
        procedural_renderer_->render();
        return true;
    } else {
        // Fall back to legacy CPU-based rendering
        return render_legacy(tiles);
    }
}

RenderStats HexRenderer::get_stats() const {
    RenderStats stats{};

    if (use_procedural_ && procedural_renderer_) {
        stats.vertices_rendered = static_cast<std::uint32_t>(
            procedural_renderer_->instance_count() * 7);  // 7 vertices per hex
        stats.triangles_rendered = static_cast<std::uint32_t>(
            procedural_renderer_->instance_count() * 6);  // 6 triangles per hex
        stats.draw_calls =
            procedural_renderer_->instance_count() > 0 ? 1 : 0;  // Single instanced draw call
    }

    return stats;
}

void HexRenderer::determine_rendering_mode() {
    switch (mode_) {
        case RenderingMode::Auto:
            // Prefer procedural rendering for better performance
            // Could add capability detection here in the future
            use_procedural_ = true;
            break;

        case RenderingMode::Legacy:
            use_procedural_ = false;
            break;

        case RenderingMode::Procedural:
            use_procedural_ = true;
            break;
    }
}

bool HexRenderer::render_legacy(const std::vector<const Tile*>& tiles) {
    // Legacy CPU-based rendering using HexMeshGenerator
    // This is a simplified implementation - in practice you'd want to:
    // 1. Generate meshes using HexMeshGenerator
    // 2. Upload to GPU
    // 3. Render with traditional vertex buffers

    std::vector<Tile*> mutable_tiles;
    mutable_tiles.reserve(tiles.size());

    for (const Tile* tile : tiles) {
        mutable_tiles.push_back(const_cast<Tile*>(tile));
    }

    // Generate mesh (this is expensive on CPU)
    // TODO: Re-enable when HexMeshGenerator is available
    // [[maybe_unused]] auto mesh = HexMeshGenerator::generate_tile_mesh(mutable_tiles);

    // TODO: In a full implementation, you would:
    // 1. Upload mesh.vertices and mesh.indices to GPU buffers
    // 2. Bind appropriate shaders
    // 3. Issue draw calls
    // For now, this is just a placeholder

    return true;  // Success placeholder
}

}  // namespace Render
}  // namespace Manifest
