#include "../utils/TestUtils.hpp"
#include "../../src/render/camera/Camera.hpp"

using namespace Manifest::Render::Camera;
using namespace Manifest::Core::Math;
using namespace Manifest::Test;

class CameraControlsTest : public DeterministicTest {
protected:
    System camera_system_;
    
    void SetUp() override {
        DeterministicTest::SetUp();
        // Use mock cursor provider for testing
        camera_system_ = System{std::make_unique<Cursor::MockProvider>()};
    }
};

TEST_F(CameraControlsTest, SystemInitialization) {
    EXPECT_TRUE(camera_system_.is_active());
    EXPECT_EQ(camera_system_.controls().mode(), ControlMode::Orbital);
    EXPECT_FALSE(camera_system_.selection().has_selection());
    EXPECT_TRUE(camera_system_.cursor().is_visible());
}

TEST_F(CameraControlsTest, ControlModeToggling) {
    // Test orbital mode setup
    camera_system_.setup_orbital_controls();
    EXPECT_EQ(camera_system_.controls().mode(), ControlMode::Orbital);
    
    // Test free camera mode setup
    camera_system_.setup_free_controls();
    EXPECT_EQ(camera_system_.controls().mode(), ControlMode::Free);
    
    // Test cinematic mode setup
    camera_system_.setup_cinematic_controls();
    EXPECT_EQ(camera_system_.controls().mode(), ControlMode::Cinematic);
}

TEST_F(CameraControlsTest, StateManagement) {
    // Save current state
    Vec3f initial_pos = camera_system_.camera().position();
    camera_system_.save_current_state("test_state");
    
    // Modify camera position
    camera_system_.camera().set_position(Vec3f{20.0f, 20.0f, 20.0f});
    EXPECT_FALSE(nearly_equal(camera_system_.camera().position(), initial_pos));
    
    // Restore state
    camera_system_.restore_state("test_state");
    EXPECT_TRUE(nearly_equal(camera_system_.camera().position(), initial_pos));
}

TEST_F(CameraControlsTest, BehaviorSystem) {
    auto& behavior = camera_system_.behavior();
    
    // Test movement behavior
    Vec3f start_pos = camera_system_.camera().position();
    Vec3f target_pos{50.0f, 50.0f, 50.0f};
    
    behavior.set_behavior(Behavior::Presets::fly_to(target_pos));
    EXPECT_TRUE(behavior.has_behavior());
    EXPECT_FALSE(behavior.is_complete());
    
    // Simulate multiple updates
    for (int i = 0; i < 100; ++i) {
        behavior.update(camera_system_.camera(), 0.1f);
        if (behavior.is_complete()) break;
    }
    
    EXPECT_TRUE(behavior.is_complete());
    EXPECT_TRUE(nearly_equal(camera_system_.camera().position(), target_pos, 0.1f));
}

TEST_F(CameraControlsTest, CursorSystem) {
    auto& cursor = camera_system_.cursor();
    
    // Test cursor visibility
    EXPECT_TRUE(cursor.is_visible());
    
    auto result = cursor.show(false);
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(cursor.is_visible());
    
    result = cursor.show(true);
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(cursor.is_visible());
}

TEST_F(CameraControlsTest, EventHandling) {
    // Test key event handling
    UI::Window::KeyEvent key_event{
        UI::Window::KeyCode::W,
        0,
        UI::Window::Action::Press,
        UI::Window::Modifier::None
    };
    
    auto result = camera_system_.handle_event(key_event);
    EXPECT_TRUE(result.is_success());
    
    // Test mouse button event
    UI::Window::MouseButtonEvent mouse_event{
        UI::Window::MouseButton::Left,
        UI::Window::Action::Press,
        UI::Window::Modifier::None,
        Vec2d{100.0, 100.0}
    };
    
    result = camera_system_.handle_event(mouse_event);
    EXPECT_TRUE(result.is_success());
}

TEST_F(CameraControlsTest, UpdateSystem) {
    Vec3f initial_pos = camera_system_.camera().position();
    
    // Run several update cycles
    for (int i = 0; i < 10; ++i) {
        camera_system_.update(0.016f);  // 60 FPS
    }
    
    // System should remain functional after updates
    EXPECT_TRUE(camera_system_.is_active());
}

// Factory function tests
TEST_F(CameraControlsTest, FactoryFunctions) {
    // Test RTS camera creation
    auto rts_camera = Factory::create_rts_camera();
    EXPECT_EQ(rts_camera.controls().mode(), ControlMode::Orbital);
    
    // Test FPS camera creation
    auto fps_camera = Factory::create_fps_camera();
    EXPECT_EQ(fps_camera.controls().mode(), ControlMode::Free);
    
    // Test city camera creation
    auto city_camera = Factory::create_city_camera();
    EXPECT_EQ(city_camera.controls().mode(), ControlMode::Orbital);
    
    // Test world camera creation
    auto world_camera = Factory::create_world_camera();
    EXPECT_EQ(world_camera.controls().mode(), ControlMode::Orbital);
}

// Integration test for complete workflow
TEST_F(CameraControlsTest, CompleteWorkflow) {
    // 1. Set up orbital camera
    camera_system_.setup_orbital_controls();
    EXPECT_EQ(camera_system_.controls().mode(), ControlMode::Orbital);
    
    // 2. Save initial state
    camera_system_.save_current_state("workflow_start");
    
    // 3. Apply a behavior to move camera
    Vec3f target{30.0f, 30.0f, 30.0f};
    camera_system_.behavior().set_behavior(Behavior::Presets::fly_to(target));
    
    // 4. Update until behavior completes
    while (!camera_system_.behavior().is_complete()) {
        camera_system_.update(0.016f);
    }
    
    // 5. Verify camera moved to target
    EXPECT_TRUE(nearly_equal(camera_system_.camera().position(), target, 0.5f));
    
    // 6. Save final state
    camera_system_.save_current_state("workflow_end");
    
    // 7. Restore original state
    camera_system_.restore_state("workflow_start");
    
    // 8. Verify restoration worked
    EXPECT_FALSE(nearly_equal(camera_system_.camera().position(), target, 0.5f));
}
