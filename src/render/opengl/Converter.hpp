#pragma once

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

// Define missing OpenGL constants if not available
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#include <cstdint>
#include <tuple>

#include "../common/Types.hpp"

namespace Manifest {
namespace Render {
namespace OpenGL {

class Converter {
   public:
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

    static std::tuple<GLenum, GLenum, GLenum> texture_format_to_gl(TextureFormat format) {
        switch (format) {
            case TextureFormat::R8_UNORM:
                return {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
            case TextureFormat::RG8_UNORM:
                return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
            case TextureFormat::RGB8_UNORM:
                return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
            case TextureFormat::RGBA8_UNORM:
                return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
            case TextureFormat::R16_FLOAT:
                return {GL_R16F, GL_RED, GL_HALF_FLOAT};
            case TextureFormat::RG16_FLOAT:
                return {GL_RG16F, GL_RG, GL_HALF_FLOAT};
            case TextureFormat::RGB16_FLOAT:
                return {GL_RGB16F, GL_RGB, GL_HALF_FLOAT};
            case TextureFormat::RGBA16_FLOAT:
                return {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT};
            case TextureFormat::R32_FLOAT:
                return {GL_R32F, GL_RED, GL_FLOAT};
            case TextureFormat::RG32_FLOAT:
                return {GL_RG32F, GL_RG, GL_FLOAT};
            case TextureFormat::RGB32_FLOAT:
                return {GL_RGB32F, GL_RGB, GL_FLOAT};
            case TextureFormat::RGBA32_FLOAT:
                return {GL_RGBA32F, GL_RGBA, GL_FLOAT};
            case TextureFormat::D16_UNORM:
                return {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
            case TextureFormat::D24_UNORM_S8_UINT:
                return {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8};
            case TextureFormat::D32_FLOAT:
                return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT};
            default:
                return {0, 0, 0};
        }
    }

    static std::uint32_t get_texture_format_size(TextureFormat format) {
        switch (format) {
            case TextureFormat::R8_UNORM:
                return 1;
            case TextureFormat::RG8_UNORM:
                return 2;
            case TextureFormat::RGB8_UNORM:
                return 3;
            case TextureFormat::RGBA8_UNORM:
                return 4;
            case TextureFormat::R16_FLOAT:
                return 2;
            case TextureFormat::RG16_FLOAT:
                return 4;
            case TextureFormat::RGB16_FLOAT:
                return 6;
            case TextureFormat::RGBA16_FLOAT:
                return 8;
            case TextureFormat::R32_FLOAT:
                return 4;
            case TextureFormat::RG32_FLOAT:
                return 8;
            case TextureFormat::RGB32_FLOAT:
                return 12;
            case TextureFormat::RGBA32_FLOAT:
                return 16;
            case TextureFormat::D16_UNORM:
                return 2;
            case TextureFormat::D24_UNORM_S8_UINT:
                return 4;
            case TextureFormat::D32_FLOAT:
                return 4;
            default:
                return 4;
        }
    }

    static GLenum attribute_format_to_gl_type(AttributeFormat format) {
        switch (format) {
            case AttributeFormat::Float:
            case AttributeFormat::Float2:
            case AttributeFormat::Float3:
            case AttributeFormat::Float4:
                return GL_FLOAT;
            case AttributeFormat::Int:
            case AttributeFormat::Int2:
            case AttributeFormat::Int3:
            case AttributeFormat::Int4:
                return GL_INT;
            case AttributeFormat::UInt:
            case AttributeFormat::UInt2:
            case AttributeFormat::UInt3:
            case AttributeFormat::UInt4:
                return GL_UNSIGNED_INT;
            default:
                return GL_FLOAT;
        }
    }

    static GLint attribute_format_to_gl_size(AttributeFormat format) {
        switch (format) {
            case AttributeFormat::Float:
            case AttributeFormat::Int:
            case AttributeFormat::UInt:
                return 1;
            case AttributeFormat::Float2:
            case AttributeFormat::Int2:
            case AttributeFormat::UInt2:
                return 2;
            case AttributeFormat::Float3:
            case AttributeFormat::Int3:
            case AttributeFormat::UInt3:
                return 3;
            case AttributeFormat::Float4:
            case AttributeFormat::Int4:
            case AttributeFormat::UInt4:
                return 4;
            default:
                return 1;
        }
    }
};

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest
