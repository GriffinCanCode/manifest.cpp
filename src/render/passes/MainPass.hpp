#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include "../common/ProceduralHexRenderer.hpp"
#include "RenderPass.hpp"
#include "ShadowPass.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

/**
 * Main scene rendering pass with shadow mapping and modern lighting.
 * Integrates with existing ProceduralHexRenderer while adding shadows.
 */
class MainPass final : public RenderPass {
   public:
    struct MainUniforms {
        Mat4f view_projection_matrix;
        Vec3f camera_position;
        float _padding1;

        Vec3f sun_direction;
        float sun_intensity;

        Vec3f sun_color;
        float _padding2;

        Vec3f ambient_color;
        float time;

        // Shadow mapping data
        std::array<Mat4f, ShadowPass::CASCADE_COUNT> shadow_matrices;
        Vec4f cascade_splits;
        float shadow_bias{0.001f};
        Vec3f _padding3{};
    };

    MainPass() = default;
    ~MainPass() override = default;

    [[nodiscard]] Result<void> initialize(Renderer* renderer) override;
    void shutdown() override;
    [[nodiscard]] Result<void> execute(const PassContext& context) override;

    [[nodiscard]] std::string_view get_name() const noexcept override { return "MainPass"; }

    [[nodiscard]] std::uint32_t get_priority() const noexcept override {
        return 500;  // Execute after shadow pass
    }

    /**
     * Set the shadow pass reference for shadow mapping
     */
    void set_shadow_pass(const ShadowPass* shadow_pass) { shadow_pass_ = shadow_pass; }

    /**
     * Get the hex renderer for external use
     */
    [[nodiscard]] ProceduralHexRenderer* get_hex_renderer() noexcept { return hex_renderer_.get(); }

   private:
    // Dependencies
    const ShadowPass* shadow_pass_{nullptr};

    // Renderers
    std::unique_ptr<ProceduralHexRenderer> hex_renderer_;

    // Additional GPU resources for shadow mapping
    BufferHandle main_uniforms_buffer_{};
    ShaderHandle main_vertex_shader_{};
    ShaderHandle main_fragment_shader_{};
    PipelineHandle main_pipeline_{};

    // CPU data
    MainUniforms main_uniforms_{};

    [[nodiscard]] Result<void> create_main_resources();
    [[nodiscard]] Result<void> create_main_shaders();
    [[nodiscard]] Result<void> create_main_pipeline();

    void update_uniforms(const PassContext& context);
    [[nodiscard]] Result<void> upload_uniforms();
};

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest
