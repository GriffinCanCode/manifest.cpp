#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "RenderPass.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

/**
 * Modern Cascaded Shadow Mapping implementation using 2025 best practices.
 * Provides high-quality shadows with minimal performance overhead.
 */
class ShadowPass final : public RenderPass {
   public:
    static constexpr std::uint32_t CASCADE_COUNT = 4;
    static constexpr std::uint32_t SHADOW_MAP_SIZE = 2048;

    struct CascadeData {
        Mat4f light_view_projection;
        float split_distance;
        Vec2f _padding{};
    };

    struct ShadowUniforms {
        std::array<CascadeData, CASCADE_COUNT> cascades;
        Vec3f light_direction;
        float shadow_bias{0.001f};
        Vec4f cascade_splits;  // x,y,z,w = split distances
    };

    ShadowPass() = default;
    ~ShadowPass() override = default;

    [[nodiscard]] Result<void> initialize(Renderer* renderer) override;
    void shutdown() override;
    [[nodiscard]] Result<void> execute(const PassContext& context) override;

    [[nodiscard]] std::string_view get_name() const noexcept override { return "ShadowPass"; }

    [[nodiscard]] std::uint32_t get_priority() const noexcept override {
        return 100;  // Execute early
    }

    /**
     * Get the shadow map texture for use in other passes
     */
    [[nodiscard]] TextureHandle get_shadow_map() const noexcept { return shadow_texture_; }

    /**
     * Get shadow uniforms for use in main pass
     */
    [[nodiscard]] const ShadowUniforms& get_shadow_uniforms() const noexcept {
        return shadow_uniforms_;
    }

   private:
    // GPU resources
    RenderTargetHandle shadow_render_target_{};
    TextureHandle shadow_texture_{};
    BufferHandle shadow_uniforms_buffer_{};

    ShaderHandle shadow_vertex_shader_{};
    ShaderHandle shadow_fragment_shader_{};
    PipelineHandle shadow_pipeline_{};

    // CPU data
    ShadowUniforms shadow_uniforms_{};
    std::array<float, CASCADE_COUNT> cascade_splits_{10.0f, 25.0f, 60.0f, 150.0f};

    [[nodiscard]] Result<void> create_shadow_resources();
    [[nodiscard]] Result<void> create_shadow_shaders();
    [[nodiscard]] Result<void> create_shadow_pipeline();

    void calculate_cascade_matrices(const PassContext& context);
    [[nodiscard]] Mat4f create_orthographic_projection(const Vec3f& light_dir,
                                                       const std::array<Vec3f, 8>& frustum_corners);
    std::array<Vec3f, 8> calculate_frustum_corners(const Mat4f& view_proj, float near_dist,
                                                   float far_dist);
};

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest
