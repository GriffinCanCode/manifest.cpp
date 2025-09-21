#pragma once

#include "Types.hpp"
#include <memory>
#include <span>
#include <functional>
#include <expected>
#include <string_view>

namespace Manifest::Render {

enum class RendererError {
    InitializationFailed,
    DeviceLost,
    OutOfMemory,
    InvalidHandle,
    InvalidState,
    CompilationFailed,
    UnsupportedFormat
};

template<typename T>
using Result = std::expected<T, RendererError>;

class Renderer {
public:
    virtual ~Renderer() = default;
    
    // Initialization
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    
    // Resource creation
    virtual Result<BufferHandle> create_buffer(const BufferDesc& desc, std::span<const std::byte> initial_data = {}) = 0;
    virtual Result<TextureHandle> create_texture(const TextureDesc& desc, std::span<const std::byte> initial_data = {}) = 0;
    virtual Result<ShaderHandle> create_shader(const ShaderDesc& desc) = 0;
    virtual Result<PipelineHandle> create_pipeline(const PipelineDesc& desc) = 0;
    virtual Result<RenderTargetHandle> create_render_target(std::span<const TextureHandle> color_attachments, TextureHandle depth_attachment = {}) = 0;
    
    // Resource destruction
    virtual void destroy_buffer(BufferHandle handle) = 0;
    virtual void destroy_texture(TextureHandle handle) = 0;
    virtual void destroy_shader(ShaderHandle handle) = 0;
    virtual void destroy_pipeline(PipelineHandle handle) = 0;
    virtual void destroy_render_target(RenderTargetHandle handle) = 0;
    
    // Resource updates
    virtual Result<void> update_buffer(BufferHandle handle, std::size_t offset, std::span<const std::byte> data) = 0;
    virtual Result<void> update_texture(TextureHandle handle, std::uint32_t mip_level, std::span<const std::byte> data) = 0;
    
    // Command recording
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    
    virtual void begin_render_pass(RenderTargetHandle target, const Vec4f& clear_color = {0.0f, 0.0f, 0.0f, 1.0f}) = 0;
    virtual void end_render_pass() = 0;
    
    virtual void set_viewport(const Viewport& viewport) = 0;
    virtual void set_scissor(const Scissor& scissor) = 0;
    virtual void bind_pipeline(PipelineHandle pipeline) = 0;
    virtual void bind_vertex_buffer(BufferHandle buffer, std::uint32_t binding = 0, std::size_t offset = 0) = 0;
    virtual void bind_index_buffer(BufferHandle buffer, std::size_t offset = 0) = 0;
    virtual void bind_texture(TextureHandle texture, std::uint32_t binding) = 0;
    virtual void bind_uniform_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset = 0, std::size_t size = 0) = 0;
    
    virtual void draw(const DrawCommand& command) = 0;
    virtual void draw_indexed(const DrawIndexedCommand& command) = 0;
    virtual void dispatch(const ComputeDispatch& dispatch) = 0;
    
    // State queries
    virtual API get_api() const noexcept = 0;
    virtual const RenderStats& get_stats() const noexcept = 0;
    virtual void reset_stats() noexcept = 0;
    
    // Memory management
    virtual std::uint64_t get_used_memory() const noexcept = 0;
    virtual std::uint64_t get_available_memory() const noexcept = 0;
    
    // Debug functionality
    virtual void set_debug_name(BufferHandle handle, std::string_view name) = 0;
    virtual void set_debug_name(TextureHandle handle, std::string_view name) = 0;
    virtual void set_debug_name(ShaderHandle handle, std::string_view name) = 0;
    virtual void set_debug_name(PipelineHandle handle, std::string_view name) = 0;
    
    virtual void push_debug_group(std::string_view name) = 0;
    virtual void pop_debug_group() = 0;
    virtual void insert_debug_marker(std::string_view name) = 0;
    
protected:
    RenderStats stats_{};
};

// Factory function
std::unique_ptr<Renderer> create_renderer(API api);

// RAII helpers
class ScopedRenderPass {
    Renderer* renderer_;
    
public:
    ScopedRenderPass(Renderer* renderer, RenderTargetHandle target, const Vec4f& clear_color = {0.0f, 0.0f, 0.0f, 1.0f})
        : renderer_{renderer} {
        renderer_->begin_render_pass(target, clear_color);
    }
    
    ~ScopedRenderPass() {
        renderer_->end_render_pass();
    }
    
    ScopedRenderPass(const ScopedRenderPass&) = delete;
    ScopedRenderPass& operator=(const ScopedRenderPass&) = delete;
    ScopedRenderPass(ScopedRenderPass&&) = default;
    ScopedRenderPass& operator=(ScopedRenderPass&&) = default;
};

class ScopedDebugGroup {
    Renderer* renderer_;
    
public:
    ScopedDebugGroup(Renderer* renderer, std::string_view name)
        : renderer_{renderer} {
        renderer_->push_debug_group(name);
    }
    
    ~ScopedDebugGroup() {
        renderer_->pop_debug_group();
    }
    
    ScopedDebugGroup(const ScopedDebugGroup&) = delete;
    ScopedDebugGroup& operator=(const ScopedDebugGroup&) = delete;
    ScopedDebugGroup(ScopedDebugGroup&&) = default;
    ScopedDebugGroup& operator=(ScopedDebugGroup&&) = default;
};

// Convenience macros
#define SCOPED_RENDER_PASS(renderer, target, ...) \
    auto _render_pass = ScopedRenderPass{renderer, target, ##__VA_ARGS__}

#define SCOPED_DEBUG_GROUP(renderer, name) \
    auto _debug_group = ScopedDebugGroup{renderer, name}

// Error handling helpers
constexpr std::string_view to_string(RendererError error) noexcept {
    switch (error) {
        case RendererError::InitializationFailed: return "Initialization failed";
        case RendererError::DeviceLost: return "Device lost";
        case RendererError::OutOfMemory: return "Out of memory";
        case RendererError::InvalidHandle: return "Invalid handle";
        case RendererError::InvalidState: return "Invalid state";
        case RendererError::CompilationFailed: return "Compilation failed";
        case RendererError::UnsupportedFormat: return "Unsupported format";
        default: return "Unknown error";
    }
}

} // namespace Manifest::Render
