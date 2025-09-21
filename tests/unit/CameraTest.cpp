#include "../utils/TestUtils.hpp"
#include "../../src/render/common/Camera.hpp"

using namespace Manifest::Render;
using namespace Manifest::Core::Math;
using namespace Manifest::Test;

class CameraTest : public DeterministicTest {};

TEST_F(CameraTest, BasicCameraOperations) {
    Camera camera;
    
    Vec3f initial_pos = camera.position();
    Vec3f initial_target = camera.target();
    
    EXPECT_FLOAT_EQ(camera.fov(), std::numbers::pi_v<float> / 4.0f);
    EXPECT_FLOAT_EQ(camera.aspect_ratio(), 16.0f / 9.0f);
    
    Vec3f new_pos{10.0f, 5.0f, 10.0f};
    camera.set_position(new_pos);
    EXPECT_TRUE(nearly_equal(camera.position(), new_pos));
}

// Placeholder - would be expanded with comprehensive camera tests
