#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "../../core/math/Matrix.hpp"
#include "../../core/math/Vector.hpp"
#include "../../core/types/Types.hpp"

namespace Manifest::Render {

using namespace Core::Types;
using namespace Core::Math;

enum class API : std::uint8_t { Vulkan, OpenGL };

enum class BufferUsage : std::uint8_t { Vertex, Index, Uniform, Storage, Staging };

enum class TextureFormat : std::uint8_t {
    R8_UNORM,
    RG8_UNORM,
    RGB8_UNORM,
    RGBA8_UNORM,
    R16_FLOAT,
    RG16_FLOAT,
    RGB16_FLOAT,
    RGBA16_FLOAT,
    R32_FLOAT,
    RG32_FLOAT,
    RGB32_FLOAT,
    RGBA32_FLOAT,
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_FLOAT
};

enum class ShaderStage : std::uint8_t {
    Vertex,
    Fragment,
    Geometry,
    TessellationControl,
    TessellationEvaluation,
    Compute
};

enum class PrimitiveTopology : std::uint8_t {
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

enum class BlendMode : std::uint8_t { None, Alpha, Additive, Multiply };

enum class CullMode : std::uint8_t { None, Front, Back, FrontAndBack };

enum class DepthTest : std::uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

struct Viewport {
    Vec2f position{};
    Vec2f size{};
    Vec2f depth_range{0.0f, 1.0f};

    constexpr bool operator==(const Viewport&) const noexcept = default;
};

struct Scissor {
    Vec2i offset{};
    Vec2i extent{};

    constexpr bool operator==(const Scissor&) const noexcept = default;
};

struct RenderState {
    PrimitiveTopology topology{PrimitiveTopology::Triangles};
    BlendMode blend_mode{BlendMode::None};
    CullMode cull_mode{CullMode::Back};
    DepthTest depth_test{DepthTest::Less};
    bool depth_write{true};
    bool wireframe{false};

    constexpr bool operator==(const RenderState&) const noexcept = default;
};

// Resource handles using strong typing
struct BufferTag {};
struct TextureTag {};
struct ShaderTag {};
struct PipelineTag {};
struct RenderTargetTag {};

using BufferHandle = StrongId<BufferTag>;
using TextureHandle = StrongId<TextureTag>;
using ShaderHandle = StrongId<ShaderTag>;
using PipelineHandle = StrongId<PipelineTag>;
using RenderTargetHandle = StrongId<RenderTargetTag>;

struct BufferDesc {
    std::size_t size{};
    BufferUsage usage{};
    bool host_visible{false};
    std::string_view debug_name{};
};

struct TextureDesc {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t depth{1};
    std::uint32_t mip_levels{1};
    std::uint32_t array_layers{1};
    TextureFormat format{};
    bool render_target{false};
    std::string_view debug_name{};
};

struct ShaderDesc {
    ShaderStage stage{};
    std::span<const std::uint8_t> bytecode{};
    std::string_view entry_point{"main"};
    std::string_view debug_name{};
};

// Vertex attributes
enum class AttributeFormat : std::uint8_t {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4
};

struct VertexAttribute {
    std::uint32_t location{};
    AttributeFormat format{};
    std::uint32_t offset{};
    std::string_view name{};
};

struct VertexBinding {
    std::uint32_t binding{};
    std::uint32_t stride{};
    std::span<const VertexAttribute> attributes{};
};

struct PipelineDesc {
    std::span<const ShaderHandle> shaders{};
    std::span<const VertexBinding> vertex_bindings{};
    RenderState render_state{};
    RenderTargetHandle render_target{};
    std::string_view debug_name{};
};

// Rendering commands
struct DrawCommand {
    std::uint32_t vertex_count{};
    std::uint32_t instance_count{1};
    std::uint32_t first_vertex{};
    std::uint32_t first_instance{};
};

struct DrawIndexedCommand {
    std::uint32_t index_count{};
    std::uint32_t instance_count{1};
    std::uint32_t first_index{};
    std::int32_t vertex_offset{};
    std::uint32_t first_instance{};
};

struct ComputeDispatch {
    std::uint32_t group_count_x{1};
    std::uint32_t group_count_y{1};
    std::uint32_t group_count_z{1};
};

// Statistics
struct RenderStats {
    std::uint32_t draw_calls{};
    std::uint32_t vertices_rendered{};
    std::uint32_t triangles_rendered{};
    std::uint64_t gpu_memory_used{};

    void reset() noexcept { *this = {}; }

    RenderStats& operator+=(const RenderStats& other) noexcept {
        draw_calls += other.draw_calls;
        vertices_rendered += other.vertices_rendered;
        triangles_rendered += other.triangles_rendered;
        gpu_memory_used += other.gpu_memory_used;
        return *this;
    }
};

}  // namespace Manifest::Render
