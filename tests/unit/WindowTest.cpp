#include <gtest/gtest.h>
#include "ui/window/Manager.hpp"
#include "ui/window/backends/Mock.hpp"

using namespace Manifest::UI::Window;
using namespace Manifest::UI::Window::Backends;

class WindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use mock backend for testing
        FactoryConfig config{Backend::Mock};
        auto result = manager_.initialize(config);
        ASSERT_TRUE(result.has_value()) << "Failed to initialize window manager";
    }
    
    void TearDown() override {
        manager_.shutdown();
    }
    
    Manager manager_;
};

TEST_F(WindowTest, CreateAndDestroyWindow) {
    WindowDesc desc{
        .size = {800, 600},
        .title = "Test Window"
    };
    
    auto handle_result = manager_.create_window(desc);
    ASSERT_TRUE(handle_result.has_value());
    
    auto handle = *handle_result;
    EXPECT_TRUE(manager_.has_window(handle));
    EXPECT_EQ(manager_.window_count(), 1);
    
    Window* window = manager_.get_window(handle);
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->properties().size.x(), 800);
    EXPECT_EQ(window->properties().size.y(), 600);
    
    auto destroy_result = manager_.destroy_window(handle);
    EXPECT_TRUE(destroy_result.has_value());
    EXPECT_FALSE(manager_.has_window(handle));
    EXPECT_EQ(manager_.window_count(), 0);
}

TEST_F(WindowTest, WindowProperties) {
    WindowDesc desc{
        .size = {1280, 720},
        .position = {100, 100},
        .title = "Property Test",
        .resizable = false,
        .decorated = false
    };
    
    auto handle_result = manager_.create_window(desc);
    ASSERT_TRUE(handle_result.has_value());
    
    auto handle = *handle_result;
    Window* window = manager_.get_window(handle);
    ASSERT_NE(window, nullptr);
    
    const auto& props = window->properties();
    EXPECT_EQ(props.size.x(), 1280);
    EXPECT_EQ(props.size.y(), 720);
    EXPECT_EQ(props.position.x(), 100);
    EXPECT_EQ(props.position.y(), 100);
    
    // Test property modifications
    auto resize_result = window->set_size({1920, 1080});
    EXPECT_TRUE(resize_result.has_value());
    
    auto move_result = window->set_position({200, 200});  
    EXPECT_TRUE(move_result.has_value());
    
    auto title_result = window->set_title("New Title");
    EXPECT_TRUE(title_result.has_value());
}

TEST_F(WindowTest, MultipleWindows) {
    WindowDesc desc1{.title = "Window 1"};
    WindowDesc desc2{.title = "Window 2"};
    WindowDesc desc3{.title = "Window 3"};
    
    auto handle1_result = manager_.create_window(desc1);
    auto handle2_result = manager_.create_window(desc2);
    auto handle3_result = manager_.create_window(desc3);
    
    ASSERT_TRUE(handle1_result.has_value());
    ASSERT_TRUE(handle2_result.has_value()); 
    ASSERT_TRUE(handle3_result.has_value());
    
    auto handle1 = *handle1_result;
    auto handle2 = *handle2_result;
    auto handle3 = *handle3_result;
    
    EXPECT_EQ(manager_.window_count(), 3);
    EXPECT_EQ(manager_.primary_window(), handle1); // First window is primary
    
    // Test getting all windows
    auto all_windows = manager_.all_windows();
    EXPECT_EQ(all_windows.size(), 3);
    EXPECT_TRUE(std::find(all_windows.begin(), all_windows.end(), handle1) != all_windows.end());
    EXPECT_TRUE(std::find(all_windows.begin(), all_windows.end(), handle2) != all_windows.end());
    EXPECT_TRUE(std::find(all_windows.begin(), all_windows.end(), handle3) != all_windows.end());
    
    // Change primary window
    manager_.set_primary_window(handle2);
    EXPECT_EQ(manager_.primary_window(), handle2);
    
    // Destroy all windows
    manager_.destroy_all_windows();
    EXPECT_EQ(manager_.window_count(), 0);
}

TEST_F(WindowTest, EventSystem) {
    WindowDesc desc{.title = "Event Test"};
    auto handle_result = manager_.create_window(desc);
    ASSERT_TRUE(handle_result.has_value());
    
    auto handle = *handle_result;
    
    std::vector<Event> received_events;
    manager_.set_global_event_callback([&](WindowHandle h, const Event& event) {
        EXPECT_EQ(h, handle);
        received_events.push_back(event);
    });
    
    // Mock events would need to be injected through the mock backend
    // This tests the callback mechanism
    manager_.poll_all_events();
    
    // The mock implementation doesn't generate events automatically,
    // but in real usage, events would be dispatched during polling
}

TEST_F(WindowTest, SurfaceCreation) {
    WindowDesc desc{.title = "Surface Test"};
    auto handle_result = manager_.create_window(desc);
    ASSERT_TRUE(handle_result.has_value());
    
    auto handle = *handle_result;
    Window* window = manager_.get_window(handle);
    ASSERT_NE(window, nullptr);
    
    // Test Vulkan surface creation (mock)
    auto vulkan_surface_result = window->create_vulkan_surface(nullptr);
    EXPECT_TRUE(vulkan_surface_result.has_value());
    
    // Test OpenGL context creation (mock)
    auto opengl_result = window->make_opengl_context_current();
    EXPECT_TRUE(opengl_result.has_value());
}

TEST_F(WindowTest, ErrorHandling) {
    // Test invalid handle
    WindowHandle invalid_handle{999};
    EXPECT_FALSE(manager_.has_window(invalid_handle));
    EXPECT_EQ(manager_.get_window(invalid_handle), nullptr);
    
    auto destroy_result = manager_.destroy_window(invalid_handle);
    EXPECT_FALSE(destroy_result.has_value());
    EXPECT_EQ(destroy_result.error(), WindowError::InvalidHandle);
}

TEST_F(WindowTest, MockWindowBehavior) {
    WindowDesc desc{.title = "Mock Test"};
    auto handle_result = manager_.create_window(desc);
    ASSERT_TRUE(handle_result.has_value());
    
    auto handle = *handle_result;
    Window* window = manager_.get_window(handle);
    ASSERT_NE(window, nullptr);
    
    // Cast to mock window to access testing features
    MockWindow* mock_window = dynamic_cast<MockWindow*>(window);
    ASSERT_NE(mock_window, nullptr);
    
    // Test method call recording
    window->poll_events();
    window->swap_buffers();
    
    const auto& calls = mock_window->method_calls();
    EXPECT_TRUE(std::find(calls.begin(), calls.end(), "poll_events") != calls.end());
    EXPECT_TRUE(std::find(calls.begin(), calls.end(), "swap_buffers") != calls.end());
    
    // Test simulation methods
    mock_window->simulate_close();
    EXPECT_TRUE(window->should_close());
    
    mock_window->simulate_resize({1920, 1080});
    EXPECT_EQ(window->properties().size.x(), 1920);
    EXPECT_EQ(window->properties().size.y(), 1080);
}

// Factory tests
class FactoryTest : public ::testing::Test {};

TEST_F(FactoryTest, FactoryRegistry) {
    auto& registry = FactoryRegistry::instance();
    
    auto backends = registry.available_backends();
    EXPECT_FALSE(backends.empty());
    
    // Mock backend should always be available
    EXPECT_TRUE(std::find(backends.begin(), backends.end(), Backend::Mock) != backends.end());
    
    FactoryConfig config{Backend::Mock};
    auto factory_result = registry.create_factory(config);
    EXPECT_TRUE(factory_result.has_value());
    
    auto factory = std::move(*factory_result);
    EXPECT_EQ(factory->backend(), Backend::Mock);
    EXPECT_TRUE(factory->is_available());
}

TEST_F(FactoryTest, ConvenienceFunctions) {
    FactoryConfig config{Backend::Mock};
    
    auto factory_result = create_window_factory(config);
    EXPECT_TRUE(factory_result.has_value());
    
    WindowDesc desc{.title = "Convenience Test"};
    auto window_result = create_window(desc, config);
    EXPECT_TRUE(window_result.has_value());
    
    auto window = std::move(*window_result);
    EXPECT_NE(window.get(), nullptr);
    EXPECT_FALSE(window->should_close());
}
