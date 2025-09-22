#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <memory>
#include <unordered_map>

#include "../common/Renderer.hpp"
#include "Loader.hpp"
#include "Resource.hpp"

namespace Manifest {
namespace Render {
namespace OpenGL {

class Core : public Renderer {
    // Resource storage
    std::unordered_map<BufferHandle, BufferResource> buffers_;
    std::unordered_map<TextureHandle, TextureResource> textures_;
    std::unordered_map<ShaderHandle, ShaderResource> shaders_;
    std::unordered_map<PipelineHandle, PipelineResource> pipelines_;
    std::unordered_map<RenderTargetHandle, RenderTargetResource> render_targets_;

    // Handle generation
    BufferHandle next_buffer_handle_{BufferHandle{1}};
    TextureHandle next_texture_handle_{TextureHandle{1}};
    ShaderHandle next_shader_handle_{ShaderHandle{1}};
    PipelineHandle next_pipeline_handle_{PipelineHandle{1}};
    RenderTargetHandle next_render_target_handle_{RenderTargetHandle{1}};

    // Hot reloading
    std::unique_ptr<Loader> loader_;

    // Current state
    RenderTargetHandle current_render_target_{};
    PipelineHandle current_pipeline_{};

   public:
    Core() : loader_{std::make_unique<Loader>()} {}

    ~Core() override { shutdown(); }

    Result<void> initialize() override;
    void shutdown() override;

    // Resource creation
    Result<BufferHandle> create_buffer(const BufferDesc& desc, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> initial_data = {}) override;
    Result<TextureHandle> create_texture(const TextureDesc& desc, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> initial_data = {}) override;
    Result<ShaderHandle> create_shader(const ShaderDesc& desc) override;
    Result<PipelineHandle> create_pipeline(const PipelineDesc& desc) override;
    Result<RenderTargetHandle> create_render_target(::Manifest::Core::Modern::span<const TextureHandle> color_attachments, TextureHandle depth_attachment = {}) override;

    // Resource destruction
    void destroy_buffer(BufferHandle handle) override;
    void destroy_texture(TextureHandle handle) override;
    void destroy_shader(ShaderHandle handle) override;
    void destroy_pipeline(PipelineHandle handle) override;
    void destroy_render_target(RenderTargetHandle handle) override;

    // Resource updates
    Result<void> update_buffer(BufferHandle handle, std::size_t offset, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> data) override;
    Result<void> update_texture(TextureHandle handle, std::uint32_t mip_level, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> data) override;

    // Command recording
    void begin_frame() override;
    void end_frame() override;
    void begin_render_pass(RenderTargetHandle target, const Vec4f& clear_color) override;
    void end_render_pass() override;

    // Render state
    void set_viewport(const Viewport& viewport) override;
    void set_scissor(const Scissor& scissor) override;
    void bind_pipeline(PipelineHandle pipeline) override;
    void bind_vertex_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset) override;
    void bind_index_buffer(BufferHandle buffer, std::size_t offset) override;
    void bind_texture(TextureHandle texture, std::uint32_t binding) override;
    void bind_uniform_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset, std::size_t size) override;

    // Drawing
    void draw(const DrawCommand& command) override;
    void draw_indexed(const DrawIndexedCommand& command) override;
    void dispatch(const ComputeDispatch& dispatch) override;

    // State queries
    API get_api() const noexcept override { return API::OpenGL; }
    const RenderStats& get_stats() const noexcept override { return stats_; }
    void reset_stats() noexcept override { stats_.reset(); }

    // Memory management
    std::uint64_t get_used_memory() const noexcept override { return stats_.gpu_memory_used; }
    std::uint64_t get_available_memory() const noexcept override { return 0; }

    // Debug functionality
    void set_debug_name(BufferHandle handle, std::string_view name) override;
    void set_debug_name(TextureHandle handle, std::string_view name) override;
    void set_debug_name(ShaderHandle handle, std::string_view name) override;
    void set_debug_name(PipelineHandle handle, std::string_view name) override;

    void push_debug_group(std::string_view name) override;
    void pop_debug_group() override;
    void insert_debug_marker(std::string_view name) override;

   private:
    void update_hot_reload();
    void reload_shader(ShaderHandle handle);
    void relink_pipeline(PipelineHandle handle);
    void apply_render_state(const RenderState& state);
    void setup_vertex_attributes(const ::Manifest::Core::Modern::span<const VertexBinding>& bindings);
};

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest
