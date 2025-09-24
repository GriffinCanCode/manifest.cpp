#include <gtest/gtest.h>
#include "../../src/render/common/ProceduralHexRenderer.hpp"
#include "../../src/world/tiles/Tile.hpp"
#include "../../src/world/tiles/Map.hpp"
#include <memory>

using namespace Manifest::Render;
using namespace Manifest::World::Tiles;
using namespace Manifest::Core::Math;

class MockRenderer : public Renderer {
public:
    MockRenderer() = default;
    
    // Track method calls for testing
    mutable std::vector<std::string> method_calls;
    
    Result<void> initialize() override {
        method_calls.push_back("initialize");
        return {};
    }
    
    void shutdown() override {
        method_calls.push_back("shutdown");
    }
    
    Result<BufferHandle> create_buffer(const BufferDesc&, Manifest::Core::Modern::span<const Manifest::Core::Modern::byte>) override {
        method_calls.push_back("create_buffer");
        return BufferHandle{1}; // Mock handle
    }
    
    Result<ShaderHandle> create_shader(const ShaderDesc&) override {
        method_calls.push_back("create_shader");
        return ShaderHandle{1}; // Mock handle
    }
    
    Result<PipelineHandle> create_pipeline(const PipelineDesc&) override {
        method_calls.push_back("create_pipeline");
        return PipelineHandle{1}; // Mock handle
    }
    
    void begin_frame() override {
        method_calls.push_back("begin_frame");
    }
    
    void end_frame() override {
        method_calls.push_back("end_frame");
    }
    
    void begin_render_pass(RenderTargetHandle, const Vec4f&) override {
        method_calls.push_back("begin_render_pass");
    }
    
    void end_render_pass() override {
        method_calls.push_back("end_render_pass");
    }
    
    void set_viewport(const Viewport&) override {
        method_calls.push_back("set_viewport");
    }
    
    void bind_pipeline(PipelineHandle) override {
        method_calls.push_back("bind_pipeline");
    }
    
    void bind_vertex_buffer(BufferHandle, std::uint32_t, std::size_t) override {
        method_calls.push_back("bind_vertex_buffer");
    }
    
    void push_constants(Manifest::Core::Modern::span<const Manifest::Core::Modern::byte>, std::uint32_t) override {
        method_calls.push_back("push_constants");
    }
    
    void draw(const DrawCommand&) override {
        method_calls.push_back("draw");
    }
    
    // Stub implementations for other required methods
    Result<TextureHandle> create_texture(const TextureDesc&, Manifest::Core::Modern::span<const Manifest::Core::Modern::byte>) override { return TextureHandle{1}; }
    Result<RenderTargetHandle> create_render_target(Manifest::Core::Modern::span<const TextureHandle>, TextureHandle) override { return RenderTargetHandle{1}; }
    void destroy_buffer(BufferHandle) override {}
    void destroy_texture(TextureHandle) override {}
    void destroy_shader(ShaderHandle) override {}
    void destroy_pipeline(PipelineHandle) override {}
    void destroy_render_target(RenderTargetHandle) override {}
    Result<void> update_buffer(BufferHandle, std::size_t, Manifest::Core::Modern::span<const Manifest::Core::Modern::byte>) override { return {}; }
    Result<void> update_texture(TextureHandle, std::uint32_t, Manifest::Core::Modern::span<const Manifest::Core::Modern::byte>) override { return {}; }
    
    // Missing pure virtual methods
    API get_api() const noexcept override { return API::OpenGL; }
    const RenderStats& get_stats() const noexcept override { 
        static RenderStats stats;
        return stats; 
    }
    void reset_stats() noexcept override {}
    void set_scissor(const Scissor&) override {}
    void bind_index_buffer(BufferHandle, std::size_t) override {}
    void bind_texture(TextureHandle, std::uint32_t) override {}
    void bind_uniform_buffer(BufferHandle, std::uint32_t, std::size_t, std::size_t) override {}
    void draw_indexed(const DrawIndexedCommand&) override {}
    void dispatch(const ComputeDispatch&) override {}
    std::uint64_t get_used_memory() const noexcept override { return 0; }
    std::uint64_t get_available_memory() const noexcept override { return 0; }
    void set_debug_name(BufferHandle, std::string_view) override {}
    void set_debug_name(TextureHandle, std::string_view) override {}
    void set_debug_name(ShaderHandle, std::string_view) override {}
    void set_debug_name(PipelineHandle, std::string_view) override {}
    void push_debug_group(std::string_view) override {}
    void pop_debug_group() override {}
    void insert_debug_marker(std::string_view) override {}
};

class HexRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto mock_renderer = std::make_unique<MockRenderer>();
        mock_renderer_ptr = mock_renderer.get();
        
        hex_renderer = std::make_unique<ProceduralHexRenderer>();
        auto result = hex_renderer->initialize(std::move(mock_renderer));
        ASSERT_TRUE(result.has_value()) << "Failed to initialize ProceduralHexRenderer";
    }

    std::unique_ptr<ProceduralHexRenderer> hex_renderer;
    MockRenderer* mock_renderer_ptr = nullptr;
};

TEST_F(HexRendererTest, InitializeSuccessfully) {
    // Verify that initialization called the expected renderer methods
    auto& calls = mock_renderer_ptr->method_calls;
    
    EXPECT_NE(std::find(calls.begin(), calls.end(), "create_shader"), calls.end())
        << "Should have created shaders during initialization";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "create_buffer"), calls.end())
        << "Should have created buffers during initialization";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "create_pipeline"), calls.end())
        << "Should have created pipeline during initialization";
}

TEST_F(HexRendererTest, AddAndClearInstances) {
    // Test adding instances
    Vec3f position{10.0f, 0.0f, 10.0f};
    Vec4f color{1.0f, 0.0f, 0.0f, 1.0f};
    float elevation = 0.5f;
    float terrain = 1.0f;
    
    hex_renderer->add_instance(position, color, elevation, terrain);
    EXPECT_EQ(hex_renderer->instance_count(), 1);
    
    // Test clearing instances
    hex_renderer->clear_instances();
    EXPECT_EQ(hex_renderer->instance_count(), 0);
}

TEST_F(HexRendererTest, RenderFrameCallsExpectedMethods) {
    // Add a test instance
    hex_renderer->add_instance({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, 0.0f, 0.0f);
    
    // Set up basic uniforms
    Mat4f identity = Mat4f::identity();
    hex_renderer->update_globals(identity, {0.0f, 10.0f, 0.0f}, 0.0f);
    
    // Clear previous method calls
    mock_renderer_ptr->method_calls.clear();
    
    // Render a frame
    Viewport viewport{{0.0f, 0.0f}, {800.0f, 600.0f}, {0.0f, 1.0f}};
    Vec4f clear_color{0.1f, 0.1f, 0.2f, 1.0f};
    
    auto result = hex_renderer->render_frame(clear_color, viewport);
    EXPECT_TRUE(result.has_value()) << "render_frame should succeed";
    
    // Verify expected method call sequence
    auto& calls = mock_renderer_ptr->method_calls;
    
    EXPECT_NE(std::find(calls.begin(), calls.end(), "begin_frame"), calls.end())
        << "Should call begin_frame";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "begin_render_pass"), calls.end())
        << "Should call begin_render_pass";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "set_viewport"), calls.end())
        << "Should call set_viewport";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "bind_pipeline"), calls.end())
        << "Should call bind_pipeline";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "push_constants"), calls.end())
        << "Should call push_constants";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "bind_vertex_buffer"), calls.end())
        << "Should call bind_vertex_buffer";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "draw"), calls.end())
        << "Should call draw";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "end_render_pass"), calls.end())
        << "Should call end_render_pass";
    EXPECT_NE(std::find(calls.begin(), calls.end(), "end_frame"), calls.end())
        << "Should call end_frame";
}

TEST_F(HexRendererTest, HandleManyInstances) {
    // Test with many instances to verify no buffer overflows
    const size_t test_count = 1000;
    
    for (size_t i = 0; i < test_count; ++i) {
        float x = static_cast<float>(i % 10) * 10.0f;
        float z = static_cast<float>(i / 10) * 10.0f;
        hex_renderer->add_instance({x, 0.0f, z}, {1.0f, 0.0f, 0.0f, 1.0f}, 0.1f, 1.0f);
    }
    
    EXPECT_EQ(hex_renderer->instance_count(), test_count);
    
    // Test rendering with many instances
    Mat4f identity = Mat4f::identity();
    hex_renderer->update_globals(identity, {50.0f, 10.0f, 50.0f}, 0.0f);
    
    Viewport viewport{{0.0f, 0.0f}, {800.0f, 600.0f}, {0.0f, 1.0f}};
    Vec4f clear_color{0.1f, 0.1f, 0.2f, 1.0f};
    
    auto result = hex_renderer->render_frame(clear_color, viewport);
    EXPECT_TRUE(result.has_value()) << "Should handle many instances without errors";
}
