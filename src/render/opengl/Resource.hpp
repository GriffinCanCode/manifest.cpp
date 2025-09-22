#pragma once

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <string>
#include <vector>

#include "../common/Types.hpp"

namespace Manifest {
namespace Render {
namespace OpenGL {

struct BufferResource {
    GLuint id{0};
    BufferDesc desc{};
    std::string debug_name{};
    
    BufferResource() = default;
    BufferResource(GLuint buffer_id, const BufferDesc& buffer_desc, std::string name = {})
        : id{buffer_id}, desc{buffer_desc}, debug_name{std::move(name)} {}
};

struct TextureResource {
    GLuint id{0};
    TextureDesc desc{};
    std::string debug_name{};
    
    TextureResource() = default;
    TextureResource(GLuint texture_id, const TextureDesc& texture_desc, std::string name = {})
        : id{texture_id}, desc{texture_desc}, debug_name{std::move(name)} {}
};

struct ShaderResource {
    GLuint id{0};
    ShaderDesc desc{};
    std::string debug_name{};
    std::string source_code{};
    
    ShaderResource() = default;
    ShaderResource(GLuint shader_id, const ShaderDesc& shader_desc, std::string name = {}, std::string source = {})
        : id{shader_id}, desc{shader_desc}, debug_name{std::move(name)}, source_code{std::move(source)} {}
};

struct PipelineResource {
    GLuint program{0};
    PipelineDesc desc{};
    std::string debug_name{};
    std::vector<ShaderHandle> shaders{};
    
    PipelineResource() = default;
    PipelineResource(GLuint program_id, const PipelineDesc& pipeline_desc, std::string name = {})
        : program{program_id}, desc{pipeline_desc}, debug_name{std::move(name)} {}
};

struct RenderTargetResource {
    GLuint framebuffer{0};
    std::vector<TextureHandle> color_attachments{};
    TextureHandle depth_attachment{};
    std::string debug_name{};
    
    RenderTargetResource() = default;
    RenderTargetResource(GLuint fbo, std::string name = {})
        : framebuffer{fbo}, debug_name{std::move(name)} {}
};

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest
