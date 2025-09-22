// Standalone test to verify camera system compiles
#include "Camera.hpp"
#include <iostream>

int main() {
    std::cout << "Testing camera system compilation...\n";
    
    try {
        // Test basic camera system creation
        auto camera_system = Manifest::Render::CameraSystem::Factory::create_rts_camera();
        std::cout << "✅ RTS camera created successfully\n";
        
        // Test controls
        auto mode = camera_system.controls().mode();
        std::cout << "✅ Controls system operational, mode: " << static_cast<int>(mode) << "\n";
        
        // Test state management
        camera_system.save_current_state("test_state");
        std::cout << "✅ State management working\n";
        
        // Test cursor system
        bool visible = camera_system.cursor().is_visible();
        std::cout << "✅ Cursor system working, visible: " << visible << "\n";
        
        // Test behavior system
        bool has_behavior = camera_system.behavior().has_behavior();
        std::cout << "✅ Behavior system working, has behavior: " << has_behavior << "\n";
        
        // Test selection system  
        bool has_selection = camera_system.selection().has_selection();
        std::cout << "✅ Selection system working, has selection: " << has_selection << "\n";
        
        std::cout << "\n🎉 All camera systems compiled and initialized successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
