#pragma once

// Cross-platform OpenGL headers
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

// Define GL_BUFFER if not available (for debug labeling)
#ifndef GL_BUFFER
#define GL_BUFFER 0x82E0
#endif

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/Renderer.hpp"

namespace Manifest {
namespace Render {
namespace OpenGL {

class ShaderHotReloader {
    std::unordered_map<ShaderHandle, std::filesystem::path> shader_paths_;
    std::unordered_map<ShaderHandle, std::filesystem::file_time_type> last_modified_;

   public:
    void register_shader(ShaderHandle handle, const std::filesystem::path& path) {
        shader_paths_[handle] = path;
        if (std::filesystem::exists(path)) {
            last_modified_[handle] = std::filesystem::last_write_time(path);
        }
    }

    void unregister_shader(ShaderHandle handle) {
        shader_paths_.erase(handle);
        last_modified_.erase(handle);
    }

    std::vector<ShaderHandle> check_for_changes() {
        std::vector<ShaderHandle> changed_shaders;

        for (const auto& pair : shader_paths_) {
            const auto& handle = pair.first;
            const auto& path = pair.second;
            if (!std::filesystem::exists(path)) continue;

            auto current_time = std::filesystem::last_write_time(path);
            auto it = last_modified_.find(handle);

            if (it == last_modified_.end() || current_time > it->second) {
                last_modified_[handle] = current_time;
                changed_shaders.push_back(handle);
            }
        }

        return changed_shaders;
    }
};

class OpenGLRenderer : public Renderer {
    // Resource storage
    struct BufferInfo {
        GLuint id{0};
        BufferDesc desc{};
        std::string debug_name{};
    };

    struct TextureInfo {
        GLuint id{0};
        TextureDesc desc{};
        std::string debug_name{};
    };

    struct ShaderInfo {
        GLuint id{0};
        ShaderDesc desc{};
        std::string debug_name{};
        std::string source_code{};
    };

    struct PipelineInfo {
        GLuint program{0};
        PipelineDesc desc{};
        std::string debug_name{};
        std::vector<ShaderHandle> shaders{};
    };

    struct RenderTargetInfo {
        GLuint framebuffer{0};
        std::vector<TextureHandle> color_attachments{};
        TextureHandle depth_attachment{};
        std::string debug_name{};
    };

    // Resource maps
    std::unordered_map<BufferHandle, BufferInfo> buffers_;
    std::unordered_map<TextureHandle, TextureInfo> textures_;
    std::unordered_map<ShaderHandle, ShaderInfo> shaders_;
    std::unordered_map<PipelineHandle, PipelineInfo> pipelines_;
    std::unordered_map<RenderTargetHandle, RenderTargetInfo> render_targets_;

    // Handle generation
    BufferHandle next_buffer_handle_{BufferHandle{1}};
    TextureHandle next_texture_handle_{TextureHandle{1}};
    ShaderHandle next_shader_handle_{ShaderHandle{1}};
    PipelineHandle next_pipeline_handle_{PipelineHandle{1}};
    RenderTargetHandle next_render_target_handle_{RenderTargetHandle{1}};

    // Hot reloading
    std::unique_ptr<ShaderHotReloader> hot_reloader_;

    // Current state
    RenderTargetHandle current_render_target_{};
    PipelineHandle current_pipeline_{};

   public:
    OpenGLRenderer() : hot_reloader_{std::make_unique<ShaderHotReloader>()} {}

    ~OpenGLRenderer() override { shutdown(); }

    Result<void> initialize() override {
// Initialize OpenGL context (assuming it's already created)

// Enable debug output if available
#ifdef GL_DEBUG_OUTPUT
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(debug_callback, this);
        }
#endif

        // Set default state
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        return {};
    }

    void shutdown() override {
        // Clean up all resources
        for (auto& pair : render_targets_) {
            auto& info = pair.second;
            if (info.framebuffer) glDeleteFramebuffers(1, &info.framebuffer);
        }

        for (auto& pair : pipelines_) {
            auto& info = pair.second;
            if (info.program) glDeleteProgram(info.program);
        }

        for (auto& pair : shaders_) {
            auto& info = pair.second;
            if (info.id) glDeleteShader(info.id);
        }

        for (auto& pair : textures_) {
            auto& info = pair.second;
            if (info.id) glDeleteTextures(1, &info.id);
        }

        for (auto& pair : buffers_) {
            auto& info = pair.second;
            if (info.id) glDeleteBuffers(1, &info.id);
        }

        // Clear maps
        render_targets_.clear();
        pipelines_.clear();
        shaders_.clear();
        textures_.clear();
        buffers_.clear();
    }

    Result<BufferHandle> create_buffer(const BufferDesc& desc,
                                       std::span<const std::byte> initial_data) override {
        GLuint buffer_id;
        glGenBuffers(1, &buffer_id);

        if (!buffer_id) {
            return std::unexpected(RendererError::InitializationFailed);
        }

        GLenum target = buffer_usage_to_gl_target(desc.usage);
        GLenum usage = desc.host_visible ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

        glBindBuffer(target, buffer_id);
        glBufferData(target, desc.size, initial_data.empty() ? nullptr : initial_data.data(),
                     usage);

        if (!desc.debug_name.empty()) {
            set_gl_object_label(GL_BUFFER, buffer_id, desc.debug_name);
        }

        BufferHandle handle = next_buffer_handle_++;
        buffers_[handle] =
            BufferInfo{.id = buffer_id, .desc = desc, .debug_name = std::string{desc.debug_name}};

        stats_.gpu_memory_used += desc.size;

        return handle;
    }

    Result<ShaderHandle> create_shader(const ShaderDesc& desc) override {
        GLenum shader_type = shader_stage_to_gl_type(desc.stage);
        GLuint shader_id = glCreateShader(shader_type);

        if (!shader_id) {
            return std::unexpected(RendererError::InitializationFailed);
        }

        // Convert bytecode to source if needed (assuming GLSL source for now)
        std::string source{reinterpret_cast<const char*>(desc.bytecode.data()),
                           desc.bytecode.size()};
        const char* source_ptr = source.c_str();

        glShaderSource(shader_id, 1, &source_ptr, nullptr);
        glCompileShader(shader_id);

        // Check compilation status
        GLint compiled;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint log_length;
            glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

            if (log_length > 0) {
                std::vector<char> log(log_length);
                glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
                // Could log the error here
            }

            glDeleteShader(shader_id);
            return std::unexpected(RendererError::CompilationFailed);
        }

        if (!desc.debug_name.empty()) {
            set_gl_object_label(GL_SHADER, shader_id, desc.debug_name);
        }

        ShaderHandle handle = next_shader_handle_++;
        shaders_[handle] = ShaderInfo{.id = shader_id,
                                      .desc = desc,
                                      .debug_name = std::string{desc.debug_name},
                                      .source_code = std::move(source)};

        return handle;
    }

    Result<PipelineHandle> create_pipeline(const PipelineDesc& desc) override {
        GLuint program = glCreateProgram();
        if (!program) {
            return std::unexpected(RendererError::InitializationFailed);
        }

        // Attach shaders
        std::vector<ShaderHandle> shader_handles;
        for (ShaderHandle shader_handle : desc.shaders) {
            auto it = shaders_.find(shader_handle);
            if (it == shaders_.end()) {
                glDeleteProgram(program);
                return std::unexpected(RendererError::InvalidHandle);
            }

            glAttachShader(program, it->second.id);
            shader_handles.push_back(shader_handle);
        }

        // Bind vertex attributes
        for (const auto& binding : desc.vertex_bindings) {
            for (const auto& attribute : binding.attributes) {
                glBindAttribLocation(program, attribute.location, attribute.name.data());
            }
        }

        // Link program
        glLinkProgram(program);

        // Check link status
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint log_length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

            if (log_length > 0) {
                std::vector<char> log(log_length);
                glGetProgramInfoLog(program, log_length, nullptr, log.data());
                // Could log the error here
            }

            glDeleteProgram(program);
            return std::unexpected(RendererError::CompilationFailed);
        }

        // Detach shaders (they're no longer needed)
        for (ShaderHandle shader_handle : shader_handles) {
            auto it = shaders_.find(shader_handle);
            if (it != shaders_.end()) {
                glDetachShader(program, it->second.id);
            }
        }

        if (!desc.debug_name.empty()) {
            set_gl_object_label(GL_PROGRAM, program, desc.debug_name);
        }

        PipelineHandle handle = next_pipeline_handle_++;
        pipelines_[handle] = PipelineInfo{.program = program,
                                          .desc = desc,
                                          .debug_name = std::string{desc.debug_name},
                                          .shaders = std::move(shader_handles)};

        return handle;
    }

    // Update hot-reloaded shaders
    void update_hot_reload() {
        auto changed_shaders = hot_reloader_->check_for_changes();

        for (ShaderHandle shader_handle : changed_shaders) {
            reload_shader(shader_handle);
        }
    }

    void begin_frame() override {
        stats_.reset();
        update_hot_reload();
    }

    void end_frame() override { glFlush(); }

    void begin_render_pass(RenderTargetHandle target, const Vec4f& clear_color) override {
        current_render_target_ = target;

        if (target.is_valid()) {
            auto it = render_targets_.find(target);
            if (it != render_targets_.end()) {
                glBindFramebuffer(GL_FRAMEBUFFER, it->second.framebuffer);
            }
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Default framebuffer
        }

        glClearColor(clear_color.x(), clear_color.y(), clear_color.z(), clear_color.w());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void end_render_pass() override { current_render_target_ = RenderTargetHandle::invalid(); }

    void bind_pipeline(PipelineHandle pipeline) override {
        current_pipeline_ = pipeline;

        auto it = pipelines_.find(pipeline);
        if (it != pipelines_.end()) {
            glUseProgram(it->second.program);
            apply_render_state(it->second.desc.render_state);
        }
    }

    void draw(const DrawCommand& command) override {
        GLenum primitive =
            primitive_topology_to_gl(pipelines_[current_pipeline_].desc.render_state.topology);

        glDrawArraysInstanced(primitive, command.first_vertex, command.vertex_count,
                              command.instance_count);

        stats_.draw_calls++;
        stats_.vertices_rendered += command.vertex_count * command.instance_count;
    }

    void draw_indexed(const DrawIndexedCommand& command) override {
        GLenum primitive =
            primitive_topology_to_gl(pipelines_[current_pipeline_].desc.render_state.topology);

        glDrawElementsInstancedBaseVertex(
            primitive, command.index_count, GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(command.first_index * sizeof(GLuint)),
            command.instance_count, command.vertex_offset);

        stats_.draw_calls++;
        stats_.vertices_rendered += command.index_count * command.instance_count;
        stats_.triangles_rendered += (command.index_count / 3) * command.instance_count;
    }

    API get_api() const noexcept override { return API::OpenGL; }

    const RenderStats& get_stats() const noexcept override { return stats_; }

    void reset_stats() noexcept override { stats_.reset(); }

   private:
    // Static utility functions
    static GLenum buffer_usage_to_gl_target(BufferUsage usage) {
        switch (usage) {
            case BufferUsage::Vertex:
                return GL_ARRAY_BUFFER;
            case BufferUsage::Index:
                return GL_ELEMENT_ARRAY_BUFFER;
            case BufferUsage::Uniform:
                return GL_UNIFORM_BUFFER;
            case BufferUsage::Storage:
                return GL_SHADER_STORAGE_BUFFER;
            case BufferUsage::Staging:
                return GL_ARRAY_BUFFER;
            default:
                return GL_ARRAY_BUFFER;
        }
    }

    static GLenum shader_stage_to_gl_type(ShaderStage stage) {
        switch (stage) {
            case ShaderStage::Vertex:
                return GL_VERTEX_SHADER;
            case ShaderStage::Fragment:
                return GL_FRAGMENT_SHADER;
            case ShaderStage::Geometry:
                return GL_GEOMETRY_SHADER;
            case ShaderStage::TessellationControl:
                return GL_TESS_CONTROL_SHADER;
            case ShaderStage::TessellationEvaluation:
                return GL_TESS_EVALUATION_SHADER;
            case ShaderStage::Compute:
                return GL_COMPUTE_SHADER;
            default:
                return GL_VERTEX_SHADER;
        }
    }

    static GLenum primitive_topology_to_gl(PrimitiveTopology topology) {
        switch (topology) {
            case PrimitiveTopology::Points:
                return GL_POINTS;
            case PrimitiveTopology::Lines:
                return GL_LINES;
            case PrimitiveTopology::LineStrip:
                return GL_LINE_STRIP;
            case PrimitiveTopology::Triangles:
                return GL_TRIANGLES;
            case PrimitiveTopology::TriangleStrip:
                return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::TriangleFan:
                return GL_TRIANGLE_FAN;
            default:
                return GL_TRIANGLES;
        }
    }

    static GLenum depth_test_to_gl(DepthTest test) {
        switch (test) {
            case DepthTest::Never:
                return GL_NEVER;
            case DepthTest::Less:
                return GL_LESS;
            case DepthTest::Equal:
                return GL_EQUAL;
            case DepthTest::LessEqual:
                return GL_LEQUAL;
            case DepthTest::Greater:
                return GL_GREATER;
            case DepthTest::NotEqual:
                return GL_NOTEQUAL;
            case DepthTest::GreaterEqual:
                return GL_GEQUAL;
            case DepthTest::Always:
                return GL_ALWAYS;
            default:
                return GL_LESS;
        }
    }

    static GLenum cull_mode_to_gl(CullMode mode) {
        switch (mode) {
            case CullMode::Front:
                return GL_FRONT;
            case CullMode::Back:
                return GL_BACK;
            case CullMode::FrontAndBack:
                return GL_FRONT_AND_BACK;
            default:
                return GL_BACK;
        }
    }

    static void APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                        GLsizei length, const GLchar* message,
                                        const void* user_param) {
        // Handle OpenGL debug messages
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            // Log error
        } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
            // Log warning
        }
    }

    void reload_shader(ShaderHandle handle) {
        auto it = shaders_.find(handle);
        if (it == shaders_.end()) return;

        // Recompile shader
        GLuint shader_id = it->second.id;
        const char* source_ptr = it->second.source_code.c_str();

        glShaderSource(shader_id, 1, &source_ptr, nullptr);
        glCompileShader(shader_id);

        // Check compilation
        GLint compiled;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            // Log compilation error but don't fail completely
            return;
        }

        // Relink all pipelines using this shader
        for (auto& pair : pipelines_) {
            const auto& pipeline_handle = pair.first;
            auto& pipeline_info = pair.second;
            bool uses_shader = std::find(pipeline_info.shaders.begin(), pipeline_info.shaders.end(),
                                         handle) != pipeline_info.shaders.end();

            if (uses_shader) {
                relink_pipeline(pipeline_handle);
            }
        }
    }

    void relink_pipeline(PipelineHandle handle) {
        auto it = pipelines_.find(handle);
        if (it == pipelines_.end()) return;

        GLuint program = it->second.program;

        // Detach old shaders
        for (ShaderHandle shader_handle : it->second.shaders) {
            auto shader_it = shaders_.find(shader_handle);
            if (shader_it != shaders_.end()) {
                glDetachShader(program, shader_it->second.id);
            }
        }

        // Attach updated shaders
        for (ShaderHandle shader_handle : it->second.shaders) {
            auto shader_it = shaders_.find(shader_handle);
            if (shader_it != shaders_.end()) {
                glAttachShader(program, shader_it->second.id);
            }
        }

        // Relink
        glLinkProgram(program);

        // Check link status
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            // Log link error
        }

        // Detach shaders again
        for (ShaderHandle shader_handle : it->second.shaders) {
            auto shader_it = shaders_.find(shader_handle);
            if (shader_it != shaders_.end()) {
                glDetachShader(program, shader_it->second.id);
            }
        }
    }

    void apply_render_state(const RenderState& state) {
        // Blend mode
        switch (state.blend_mode) {
            case BlendMode::None:
                glDisable(GL_BLEND);
                break;
            case BlendMode::Alpha:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Additive:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case BlendMode::Multiply:
                glEnable(GL_BLEND);
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
        }

        // Depth test
        if (state.depth_test != DepthTest::Always) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(depth_test_to_gl(state.depth_test));
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        glDepthMask(state.depth_write ? GL_TRUE : GL_FALSE);

        // Cull mode
        if (state.cull_mode != CullMode::None) {
            glEnable(GL_CULL_FACE);
            glCullFace(cull_mode_to_gl(state.cull_mode));
        } else {
            glDisable(GL_CULL_FACE);
        }

        // Wireframe
        glPolygonMode(GL_FRONT_AND_BACK, state.wireframe ? GL_LINE : GL_FILL);
    }

    void set_gl_object_label(GLenum identifier, GLuint name, std::string_view label) {
#ifdef GL_KHR_debug
        if (glObjectLabel) {
            glObjectLabel(identifier, name, static_cast<GLsizei>(label.length()), label.data());
        }
#endif
    }

    // Stub implementations for other required methods
    Result<TextureHandle> create_texture(const TextureDesc& desc,
                                         std::span<const std::byte> initial_data) override {
        return std::unexpected(RendererError::UnsupportedFormat);
    }

    Result<RenderTargetHandle> create_render_target(
        std::span<const TextureHandle> color_attachments, TextureHandle depth_attachment) override {
        return std::unexpected(RendererError::UnsupportedFormat);
    }

    void destroy_buffer(BufferHandle handle) override {}
    void destroy_texture(TextureHandle handle) override {}
    void destroy_shader(ShaderHandle handle) override {}
    void destroy_pipeline(PipelineHandle handle) override {}
    void destroy_render_target(RenderTargetHandle handle) override {}

    Result<void> update_buffer(BufferHandle handle, std::size_t offset,
                               std::span<const std::byte> data) override {
        return std::unexpected(RendererError::InvalidHandle);
    }

    Result<void> update_texture(TextureHandle handle, std::uint32_t mip_level,
                                std::span<const std::byte> data) override {
        return std::unexpected(RendererError::InvalidHandle);
    }

    void set_viewport(const Viewport& viewport) override {}
    void set_scissor(const Scissor& scissor) override {}
    void bind_vertex_buffer(BufferHandle buffer, std::uint32_t binding,
                            std::size_t offset) override {}
    void bind_index_buffer(BufferHandle buffer, std::size_t offset) override {}
    void bind_texture(TextureHandle texture, std::uint32_t binding) override {}
    void bind_uniform_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset,
                             std::size_t size) override {}
    void dispatch(const ComputeDispatch& dispatch) override {}

    std::uint64_t get_used_memory() const noexcept override { return stats_.gpu_memory_used; }
    std::uint64_t get_available_memory() const noexcept override { return 0; }

    void set_debug_name(BufferHandle handle, std::string_view name) override {}
    void set_debug_name(TextureHandle handle, std::string_view name) override {}
    void set_debug_name(ShaderHandle handle, std::string_view name) override {}
    void set_debug_name(PipelineHandle handle, std::string_view name) override {}

    void push_debug_group(std::string_view name) override {}
    void pop_debug_group() override {}
    void insert_debug_marker(std::string_view name) override {}
};

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest
