#include "../utils/TestUtils.hpp"
#include "../../src/render/common/Camera.hpp"
#include "../../src/render/common/Renderer.hpp"

using namespace Manifest::Render;
using namespace Manifest::Core::Math;
using namespace Manifest::Test;

class RenderingIntegrationTest : public DeterministicTest {};

TEST_F(RenderingIntegrationTest, CameraMatrixGeneration) {
    Camera camera;
    camera.look_at(Vec3f{0.0f, 10.0f, 10.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    camera.set_perspective(std::numbers::pi_v<float> / 4.0f, 16.0f/9.0f, 0.1f, 100.0f);
    
    // Test that matrices are generated without error
    const Mat4f& view = camera.view_matrix();
    const Mat4f& proj = camera.projection_matrix();
    const Mat4f& vp = camera.view_projection_matrix();
    
    // Basic sanity checks - matrices should not be zero
    bool view_non_zero = false;
    bool proj_non_zero = false;
    
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (view[r][c] != 0.0f) view_non_zero = true;
            if (proj[r][c] != 0.0f) proj_non_zero = true;
        }
    }
    
    EXPECT_TRUE(view_non_zero);
    EXPECT_TRUE(proj_non_zero);
}

// Placeholder for more comprehensive rendering integration tests
