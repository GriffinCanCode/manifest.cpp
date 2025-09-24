#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

// Test the minimal shaders that should work without any vertex data
TEST(MinimalTriangleTest, TestMinimalShadersExist) {
    // Check that the minimal test shaders exist and contain simple triangle code
    std::vector<std::string> shader_paths = {
        "assets/shaders/minimal_test.vert",
        "build/assets/shaders/minimal_test.vert",
        "../assets/shaders/minimal_test.vert"
    };
    
    std::string vertex_source;
    bool found = false;
    for (const auto& path : shader_paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            vertex_source = buffer.str();
            found = true;
            std::cout << "Found minimal_test.vert at: " << path << std::endl;
            break;
        }
    }
    
    ASSERT_TRUE(found) << "Could not find minimal_test.vert file";
    EXPECT_FALSE(vertex_source.empty()) << "Vertex shader source should not be empty";
    
    // Check that it contains hardcoded triangle positions
    EXPECT_NE(vertex_source.find("vec2 positions[3]"), std::string::npos) 
        << "Minimal vertex shader should contain hardcoded triangle positions";
    EXPECT_NE(vertex_source.find("gl_VertexIndex"), std::string::npos) 
        << "Should use gl_VertexIndex to select triangle vertices";
    
    std::cout << "Minimal vertex shader analysis:" << std::endl;
    std::cout << "- Size: " << vertex_source.size() << " bytes" << std::endl;
    std::cout << "- Contains hardcoded positions: " << (vertex_source.find("vec2 positions[3]") != std::string::npos ? "YES" : "NO") << std::endl;
    std::cout << "- Uses gl_VertexIndex: " << (vertex_source.find("gl_VertexIndex") != std::string::npos ? "YES" : "NO") << std::endl;
}

TEST(MinimalTriangleTest, TestMinimalFragmentShader) {
    std::vector<std::string> shader_paths = {
        "assets/shaders/minimal_test.frag", 
        "build/assets/shaders/minimal_test.frag",
        "../assets/shaders/minimal_test.frag"
    };
    
    std::string fragment_source;
    bool found = false;
    for (const auto& path : shader_paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            fragment_source = buffer.str();
            found = true;
            std::cout << "Found minimal_test.frag at: " << path << std::endl;
            break;
        }
    }
    
    ASSERT_TRUE(found) << "Could not find minimal_test.frag file";
    EXPECT_FALSE(fragment_source.empty()) << "Fragment shader source should not be empty";
    
    // Check that it outputs a solid color
    EXPECT_NE(fragment_source.find("fragment_color"), std::string::npos) 
        << "Should output to fragment_color";
    EXPECT_NE(fragment_source.find("1.0, 0.0, 0.0"), std::string::npos) 
        << "Should output solid red color for visibility";
    
    std::cout << "Minimal fragment shader analysis:" << std::endl;
    std::cout << "- Size: " << fragment_source.size() << " bytes" << std::endl;
    std::cout << "- Outputs solid color: " << (fragment_source.find("1.0, 0.0, 0.0") != std::string::npos ? "YES" : "NO") << std::endl;
}

TEST(MinimalTriangleTest, AnalyzeCurrentShaderState) {
    std::cout << "\n=== CURRENT SHADER STATE ANALYSIS ===" << std::endl;
    
    // Check the current hex_procedural shaders
    std::ifstream vert_file("assets/shaders/hex_procedural.vert");
    if (vert_file.is_open()) {
        std::stringstream buffer;
        buffer << vert_file.rdbuf();
        std::string content = buffer.str();
        
        std::cout << "hex_procedural.vert status:" << std::endl;
        std::cout << "- Size: " << content.size() << " bytes" << std::endl;
        std::cout << "- Has debug colors: " << (content.find("gl_InstanceIndex < 10") != std::string::npos ? "YES" : "NO") << std::endl;
        std::cout << "- Hex size 50.0: " << (content.find("* 50.0") != std::string::npos ? "YES" : "NO") << std::endl;
        std::cout << "- Uses instance data: " << (content.find("instance_position") != std::string::npos ? "YES" : "NO") << std::endl;
        
        if (content.find("gl_InstanceIndex < 10") != std::string::npos) {
            std::cout << "✓ Debug colors should make first 10 hexes BRIGHT RED" << std::endl;
            std::cout << "✓ Next 90 hexes should be BRIGHT GREEN" << std::endl;
            std::cout << "If you see magenta instead of red/green, vertex buffers aren't working!" << std::endl;
        }
    }
    
    SUCCEED() << "Shader analysis complete";
}
