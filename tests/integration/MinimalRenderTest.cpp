#include <gtest/gtest.h>
#include "../../src/render/vulkan/VulkanRenderer.hpp"
#include "../../src/ui/window/Manager.hpp"
#include "../../src/ui/window/backends/GLFW.hpp"

using namespace Manifest::Render;
using namespace Manifest::Render::Vulkan;
using namespace Manifest::UI::Window;

class MinimalRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize window system
        window_manager = std::make_unique<Manager>();
        auto backend = std::make_unique<GLFW>();
        ASSERT_TRUE(window_manager->initialize(std::move(backend)));
        
        WindowDesc window_desc{
            .title = "Minimal Render Test",
            .size = {400, 300},
            .position = {200, 200},
            .initial_state = WindowState::Hidden
        };
        
        auto window_result = window_manager->create_window(window_desc);
        ASSERT_TRUE(window_result.has_value());
        window_handle = *window_result;
        
        test_window = window_manager->get_window(window_handle);
        ASSERT_NE(test_window, nullptr);
        
        renderer = std::make_unique<VulkanRenderer>();
        renderer->set_window(test_window->native_handle());
        ASSERT_TRUE(renderer->initialize());
    }

    void TearDown() override {
        if (renderer) renderer->shutdown();
        if (window_manager && window_handle.has_value()) {
            window_manager->destroy_window(*window_handle);
        }
        if (window_manager) window_manager->shutdown();
    }

    std::unique_ptr<Manager> window_manager;
    std::unique_ptr<VulkanRenderer> renderer;
    std::optional<WindowHandle> window_handle;
    Window* test_window = nullptr;
};

TEST_F(MinimalRenderTest, RenderSingleRedTriangle) {
    // This test creates the absolute minimum to render a red triangle
    // If this fails, we know the basic Vulkan pipeline is broken
    
    // Create minimal vertex shader (no inputs)
    std::string vertex_source = R"(
#version 450 core

void main() {
    vec2 positions[3] = vec2[3](
        vec2(-0.8, -0.8),
        vec2( 0.8, -0.8),
        vec2( 0.0,  0.8)
    );
    
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
)";

    std::string fragment_source = R"(
#version 450 core

layout(location = 0) out vec4 fragment_color;

void main() {
    fragment_color = vec4(1.0, 0.0, 0.0, 1.0);  // Bright red
}
)";

    // Create shaders
    ShaderDesc vertex_desc{
        .bytecode = std::span<const Core::Modern::byte>{
            reinterpret_cast<const Core::Modern::byte*>(vertex_source.data()),
            vertex_source.size()
        },
        .stage = ShaderStage::Vertex,
        .entry_point = "main",
        .debug_name = "minimal_vertex"
    };

    auto vertex_result = renderer->create_shader(vertex_desc);
    ASSERT_TRUE(vertex_result.has_value()) << "Failed to create minimal vertex shader";
    auto vertex_shader = *vertex_result;

    ShaderDesc fragment_desc{
        .bytecode = std::span<const Core::Modern::byte>{
            reinterpret_cast<const Core::Modern::byte*>(fragment_source.data()),
            fragment_source.size()
        },
        .stage = ShaderStage::Fragment,
        .entry_point = "main",
        .debug_name = "minimal_fragment"
    };

    auto fragment_result = renderer->create_shader(fragment_desc);
    ASSERT_TRUE(fragment_result.has_value()) << "Failed to create minimal fragment shader";
    auto fragment_shader = *fragment_result;

    // Create minimal pipeline (no vertex inputs)
    ShaderHandle shaders[] = {vertex_shader, fragment_shader};
    PipelineDesc pipeline_desc{
        .shaders = std::span<const ShaderHandle>{shaders},
        .vertex_bindings = {},  // No vertex buffers needed
        .render_state = RenderState{
            .topology = PrimitiveTopology::Triangles,
            .blend_mode = BlendMode::None,
            .cull_mode = CullMode::None,
            .depth_test = DepthTest::None,
            .depth_write = false,
            .wireframe = false
        },
        .render_target = {},
        .debug_name = "minimal_pipeline"
    };

    auto pipeline_result = renderer->create_pipeline(pipeline_desc);
    ASSERT_TRUE(pipeline_result.has_value()) << "Failed to create minimal pipeline";
    auto pipeline = *pipeline_result;

    // Render one frame
    renderer->begin_frame();
    
    Viewport viewport{{0.0f, 0.0f}, {400.0f, 300.0f}, {0.0f, 1.0f}};
    renderer->set_viewport(viewport);
    
    Vec4f black_clear{0.0f, 0.0f, 0.0f, 1.0f};
    renderer->begin_render_pass({}, black_clear);
    
    renderer->bind_pipeline(pipeline);
    
    DrawCommand draw_cmd{
        .vertex_count = 3,  // Triangle
        .instance_count = 1,
        .first_vertex = 0,
        .first_instance = 0
    };
    renderer->draw(draw_cmd);
    
    renderer->end_render_pass();
    renderer->end_frame();
    
    // Cleanup
    renderer->destroy_pipeline(pipeline);
    renderer->destroy_shader(fragment_shader);
    renderer->destroy_shader(vertex_shader);
    
    SUCCEED() << "Minimal triangle rendering completed successfully";
}

TEST_F(MinimalRenderTest, TestClearColorOnly) {
    // Test that we can at least clear the screen to a specific color
    // This isolates whether the issue is in the rendering pipeline or just geometry
    
    for (int frame = 0; frame < 3; ++frame) {
        renderer->begin_frame();
        
        Viewport viewport{{0.0f, 0.0f}, {400.0f, 300.0f}, {0.0f, 1.0f}};
        renderer->set_viewport(viewport);
        
        // Cycle through different clear colors
        Vec4f clear_colors[] = {
            {1.0f, 0.0f, 0.0f, 1.0f},  // Red
            {0.0f, 1.0f, 0.0f, 1.0f},  // Green  
            {0.0f, 0.0f, 1.0f, 1.0f}   // Blue
        };
        
        renderer->begin_render_pass({}, clear_colors[frame]);
        // Don't render anything - just clear
        renderer->end_render_pass();
        renderer->end_frame();
    }
    
    SUCCEED() << "Clear color test completed - check if window changes colors";
}
