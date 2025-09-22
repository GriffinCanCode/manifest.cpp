#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "RenderPass.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

/**
 * Post-processing pass implementing modern 2025 techniques:
 * - Temporal Anti-Aliasing (TAA)
 * - Tone mapping
 * - Basic color grading
 * Uses minimal GPU resources and compute-shader-like efficiency.
 */
class PostProcessPass final : public RenderPass {
   public:
    enum class Effect : std::uint8_t {
        TAA = 1 << 0,
        ToneMapping = 1 << 1,
        ColorGrading = 1 << 2,
        Sharpen = 1 << 3
    };

    struct PostProcessUniforms {
        Vec2f screen_size;
        Vec2f texel_size;

        Mat4f prev_view_projection_matrix;
        Mat4f current_view_projection_matrix;

        // TAA parameters
        Vec2f jitter_offset;
        float taa_blend_factor{0.9f};
        float taa_threshold{0.1f};

        // Tone mapping
        float exposure{1.0f};
        float gamma{2.2f};
        Vec2f _padding1{};

        // Color grading
        Vec3f color_balance{1.0f, 1.0f, 1.0f};
        float saturation{1.0f};

        float contrast{1.0f};
        float brightness{0.0f};
        std::uint32_t enabled_effects{0};
        float _padding2{};
    };

    PostProcessPass() = default;
    ~PostProcessPass() override = default;

    [[nodiscard]] Result<void> initialize(Renderer* renderer) override;
    void shutdown() override;
    [[nodiscard]] Result<void> execute(const PassContext& context) override;

    [[nodiscard]] std::string_view get_name() const noexcept override { return "PostProcessPass"; }

    [[nodiscard]] std::uint32_t get_priority() const noexcept override {
        return 900;  // Execute late
    }

    /**
     * Enable/disable specific post-processing effects
     */
    void set_effect_enabled(Effect effect, bool enabled) {
        if (enabled) {
            enabled_effects_ |= static_cast<std::uint32_t>(effect);
        } else {
            enabled_effects_ &= ~static_cast<std::uint32_t>(effect);
        }
    }

    /**
     * Configure TAA parameters
     */
    void set_taa_parameters(float blend_factor, float threshold) {
        uniforms_.taa_blend_factor = blend_factor;
        uniforms_.taa_threshold = threshold;
    }

    /**
     * Configure tone mapping parameters
     */
    void set_tone_mapping(float exposure, float gamma) {
        uniforms_.exposure = exposure;
        uniforms_.gamma = gamma;
    }

   private:
    // GPU resources
    RenderTargetHandle temp_render_target_{};
    TextureHandle temp_texture_{};
    TextureHandle history_texture_{};

    BufferHandle postprocess_uniforms_buffer_{};
    ShaderHandle fullscreen_vertex_shader_{};
    ShaderHandle postprocess_fragment_shader_{};
    PipelineHandle postprocess_pipeline_{};

    // CPU data
    PostProcessUniforms uniforms_{};
    std::uint32_t enabled_effects_{static_cast<std::uint32_t>(Effect::TAA) |
                                   static_cast<std::uint32_t>(Effect::ToneMapping)};

    // TAA jitter pattern
    std::uint32_t frame_index_{0};
    static constexpr std::array<Vec2f, 8> TAA_JITTER_PATTERN{{Vec2f{-0.5f, -0.5f},
                                                              Vec2f{0.5f, -0.5f},
                                                              Vec2f{-0.5f, 0.5f},
                                                              Vec2f{0.5f, 0.5f},
                                                              Vec2f{-0.25f, -0.25f},
                                                              Vec2f{0.25f, -0.25f},
                                                              Vec2f{-0.25f, 0.25f},
                                                              Vec2f{0.25f, 0.25f}}};

    [[nodiscard]] Result<void> create_postprocess_resources();
    [[nodiscard]] Result<void> create_postprocess_shaders();
    [[nodiscard]] Result<void> create_postprocess_pipeline();

    Vec2f get_taa_jitter() const;
    void update_uniforms(const PassContext& context);
    [[nodiscard]] Result<void> upload_uniforms();
};

constexpr PostProcessPass::Effect operator|(PostProcessPass::Effect a, PostProcessPass::Effect b) {
    return static_cast<PostProcessPass::Effect>(static_cast<std::uint32_t>(a) |
                                                static_cast<std::uint32_t>(b));
}

constexpr PostProcessPass::Effect operator&(PostProcessPass::Effect a, PostProcessPass::Effect b) {
    return static_cast<PostProcessPass::Effect>(static_cast<std::uint32_t>(a) &
                                                static_cast<std::uint32_t>(b));
}

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest
