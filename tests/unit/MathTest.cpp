#include "../utils/TestUtils.hpp"
#include "../../src/core/math/Vector.hpp"
#include "../../src/core/math/Matrix.hpp"
#include <numbers>

using namespace Manifest::Core::Math;
using namespace Manifest::Test;

class MathTest : public DeterministicTest {};

// Vector tests
TEST_F(MathTest, VectorConstruction) {
    Vec3f v1{1.0f, 2.0f, 3.0f};
    EXPECT_FLOAT_EQ(v1.x(), 1.0f);
    EXPECT_FLOAT_EQ(v1.y(), 2.0f);
    EXPECT_FLOAT_EQ(v1.z(), 3.0f);
    
    Vec3f v2{5.0f}; // Fill constructor
    EXPECT_FLOAT_EQ(v2.x(), 5.0f);
    EXPECT_FLOAT_EQ(v2.y(), 5.0f);
    EXPECT_FLOAT_EQ(v2.z(), 5.0f);
    
    Vec3f v3; // Default constructor
    EXPECT_FLOAT_EQ(v3.x(), 0.0f);
    EXPECT_FLOAT_EQ(v3.y(), 0.0f);
    EXPECT_FLOAT_EQ(v3.z(), 0.0f);
}

TEST_F(MathTest, VectorArithmetic) {
    Vec3f a{1.0f, 2.0f, 3.0f};
    Vec3f b{4.0f, 5.0f, 6.0f};
    
    Vec3f sum = a + b;
    EXPECT_FLOAT_EQ(sum.x(), 5.0f);
    EXPECT_FLOAT_EQ(sum.y(), 7.0f);
    EXPECT_FLOAT_EQ(sum.z(), 9.0f);
    
    Vec3f diff = b - a;
    EXPECT_FLOAT_EQ(diff.x(), 3.0f);
    EXPECT_FLOAT_EQ(diff.y(), 3.0f);
    EXPECT_FLOAT_EQ(diff.z(), 3.0f);
    
    Vec3f scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x(), 2.0f);
    EXPECT_FLOAT_EQ(scaled.y(), 4.0f);
    EXPECT_FLOAT_EQ(scaled.z(), 6.0f);
    
    Vec3f left_scaled = 3.0f * a;
    EXPECT_FLOAT_EQ(left_scaled.x(), 3.0f);
    EXPECT_FLOAT_EQ(left_scaled.y(), 6.0f);
    EXPECT_FLOAT_EQ(left_scaled.z(), 9.0f);
    
    Vec3f divided = a / 2.0f;
    EXPECT_FLOAT_EQ(divided.x(), 0.5f);
    EXPECT_FLOAT_EQ(divided.y(), 1.0f);
    EXPECT_FLOAT_EQ(divided.z(), 1.5f);
}

TEST_F(MathTest, VectorCompoundAssignment) {
    Vec3f v{1.0f, 2.0f, 3.0f};
    
    v += Vec3f{1.0f, 1.0f, 1.0f};
    EXPECT_FLOAT_EQ(v.x(), 2.0f);
    EXPECT_FLOAT_EQ(v.y(), 3.0f);
    EXPECT_FLOAT_EQ(v.z(), 4.0f);
    
    v -= Vec3f{0.5f, 0.5f, 0.5f};
    EXPECT_FLOAT_EQ(v.x(), 1.5f);
    EXPECT_FLOAT_EQ(v.y(), 2.5f);
    EXPECT_FLOAT_EQ(v.z(), 3.5f);
    
    v *= 2.0f;
    EXPECT_FLOAT_EQ(v.x(), 3.0f);
    EXPECT_FLOAT_EQ(v.y(), 5.0f);
    EXPECT_FLOAT_EQ(v.z(), 7.0f);
    
    v /= 2.0f;
    EXPECT_FLOAT_EQ(v.x(), 1.5f);
    EXPECT_FLOAT_EQ(v.y(), 2.5f);
    EXPECT_FLOAT_EQ(v.z(), 3.5f);
}

TEST_F(MathTest, VectorDotProduct) {
    Vec3f a{1.0f, 2.0f, 3.0f};
    Vec3f b{4.0f, 5.0f, 6.0f};
    
    float dot = a.dot(b);
    EXPECT_FLOAT_EQ(dot, 32.0f); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    
    // Test orthogonal vectors
    Vec3f x_axis{1.0f, 0.0f, 0.0f};
    Vec3f y_axis{0.0f, 1.0f, 0.0f};
    EXPECT_FLOAT_EQ(x_axis.dot(y_axis), 0.0f);
}

TEST_F(MathTest, VectorCrossProduct) {
    Vec3f x_axis{1.0f, 0.0f, 0.0f};
    Vec3f y_axis{0.0f, 1.0f, 0.0f};
    Vec3f z_axis{0.0f, 0.0f, 1.0f};
    
    Vec3f cross = x_axis.cross(y_axis);
    EXPECT_FLOAT_EQ(cross.x(), 0.0f);
    EXPECT_FLOAT_EQ(cross.y(), 0.0f);
    EXPECT_FLOAT_EQ(cross.z(), 1.0f);
    
    // Test that x cross y = z, y cross z = x, z cross x = y
    EXPECT_TRUE(nearly_equal(x_axis.cross(y_axis), z_axis));
    EXPECT_TRUE(nearly_equal(y_axis.cross(z_axis), x_axis));
    EXPECT_TRUE(nearly_equal(z_axis.cross(x_axis), y_axis));
}

TEST_F(MathTest, VectorLength) {
    Vec3f v{3.0f, 4.0f, 0.0f};
    
    EXPECT_FLOAT_EQ(v.length_squared(), 25.0f); // 3² + 4² + 0² = 9 + 16 + 0 = 25
    EXPECT_FLOAT_EQ(v.length(), 5.0f);          // √25 = 5
    
    Vec3f unit{1.0f, 0.0f, 0.0f};
    EXPECT_FLOAT_EQ(unit.length(), 1.0f);
    EXPECT_FLOAT_EQ(unit.length_squared(), 1.0f);
}

TEST_F(MathTest, VectorNormalization) {
    Vec3f v{3.0f, 4.0f, 0.0f};
    Vec3f normalized = v.normalized();
    
    EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
    EXPECT_FLOAT_EQ(normalized.x(), 0.6f);  // 3/5
    EXPECT_FLOAT_EQ(normalized.y(), 0.8f);  // 4/5
    EXPECT_FLOAT_EQ(normalized.z(), 0.0f);
    
    // Test in-place normalization
    Vec3f v2{5.0f, 0.0f, 0.0f};
    v2.normalize();
    EXPECT_FLOAT_EQ(v2.length(), 1.0f);
    EXPECT_FLOAT_EQ(v2.x(), 1.0f);
    EXPECT_FLOAT_EQ(v2.y(), 0.0f);
    EXPECT_FLOAT_EQ(v2.z(), 0.0f);
    
    // Test zero vector normalization
    Vec3f zero{0.0f, 0.0f, 0.0f};
    Vec3f zero_norm = zero.normalized();
    EXPECT_FLOAT_EQ(zero_norm.x(), 0.0f);
    EXPECT_FLOAT_EQ(zero_norm.y(), 0.0f);
    EXPECT_FLOAT_EQ(zero_norm.z(), 0.0f);
}

TEST_F(MathTest, VectorSwizzling) {
    Vec3f v{1.0f, 2.0f, 3.0f};
    
    Vec2f xy = v.xy();
    EXPECT_FLOAT_EQ(xy.x(), 1.0f);
    EXPECT_FLOAT_EQ(xy.y(), 2.0f);
    
    Vec4f v4{1.0f, 2.0f, 3.0f, 4.0f};
    Vec3f xyz = v4.xyz();
    EXPECT_FLOAT_EQ(xyz.x(), 1.0f);
    EXPECT_FLOAT_EQ(xyz.y(), 2.0f);
    EXPECT_FLOAT_EQ(xyz.z(), 3.0f);
}

// Matrix tests
TEST_F(MathTest, MatrixConstruction) {
    Mat3f m{
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    };
    
    EXPECT_FLOAT_EQ(m[0][0], 1.0f);
    EXPECT_FLOAT_EQ(m[0][1], 2.0f);
    EXPECT_FLOAT_EQ(m[1][0], 4.0f);
    EXPECT_FLOAT_EQ(m[2][2], 9.0f);
    
    EXPECT_FLOAT_EQ(m.at(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m.at(1, 2), 6.0f);
}

TEST_F(MathTest, MatrixIdentity) {
    Mat3f identity = Mat3f::identity();
    
    EXPECT_FLOAT_EQ(identity[0][0], 1.0f);
    EXPECT_FLOAT_EQ(identity[1][1], 1.0f);
    EXPECT_FLOAT_EQ(identity[2][2], 1.0f);
    
    EXPECT_FLOAT_EQ(identity[0][1], 0.0f);
    EXPECT_FLOAT_EQ(identity[1][0], 0.0f);
    EXPECT_FLOAT_EQ(identity[1][2], 0.0f);
}

TEST_F(MathTest, MatrixArithmetic) {
    Mat2f a{
        1.0f, 2.0f,
        3.0f, 4.0f
    };
    
    Mat2f b{
        5.0f, 6.0f,
        7.0f, 8.0f
    };
    
    Mat2f sum = a + b;
    EXPECT_FLOAT_EQ(sum[0][0], 6.0f);
    EXPECT_FLOAT_EQ(sum[0][1], 8.0f);
    EXPECT_FLOAT_EQ(sum[1][0], 10.0f);
    EXPECT_FLOAT_EQ(sum[1][1], 12.0f);
    
    Mat2f scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled[0][0], 2.0f);
    EXPECT_FLOAT_EQ(scaled[1][1], 8.0f);
}

TEST_F(MathTest, MatrixMultiplication) {
    Mat2f a{
        1.0f, 2.0f,
        3.0f, 4.0f
    };
    
    Mat2f b{
        2.0f, 0.0f,
        1.0f, 2.0f
    };
    
    Mat2f product = a * b;
    EXPECT_FLOAT_EQ(product[0][0], 4.0f);  // 1*2 + 2*1 = 4
    EXPECT_FLOAT_EQ(product[0][1], 4.0f);  // 1*0 + 2*2 = 4
    EXPECT_FLOAT_EQ(product[1][0], 10.0f); // 3*2 + 4*1 = 10
    EXPECT_FLOAT_EQ(product[1][1], 8.0f);  // 3*0 + 4*2 = 8
}

TEST_F(MathTest, MatrixVectorMultiplication) {
    Mat2f m{
        1.0f, 2.0f,
        3.0f, 4.0f
    };
    
    Vec2f v{5.0f, 6.0f};
    Vec2f result = m * v;
    
    EXPECT_FLOAT_EQ(result.x(), 17.0f); // 1*5 + 2*6 = 17
    EXPECT_FLOAT_EQ(result.y(), 39.0f); // 3*5 + 4*6 = 39
}

TEST_F(MathTest, MatrixTranspose) {
    Matrix<float, 2, 3> m{
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f
    };
    
    auto transposed = m.transpose();
    EXPECT_FLOAT_EQ(transposed[0][0], 1.0f);
    EXPECT_FLOAT_EQ(transposed[0][1], 4.0f);
    EXPECT_FLOAT_EQ(transposed[1][0], 2.0f);
    EXPECT_FLOAT_EQ(transposed[1][1], 5.0f);
    EXPECT_FLOAT_EQ(transposed[2][0], 3.0f);
    EXPECT_FLOAT_EQ(transposed[2][1], 6.0f);
}

TEST_F(MathTest, MatrixDeterminant) {
    // 2x2 matrix
    Mat2f m2{
        1.0f, 2.0f,
        3.0f, 4.0f
    };
    
    float det2 = m2.determinant();
    EXPECT_FLOAT_EQ(det2, -2.0f); // 1*4 - 2*3 = -2
    
    // 3x3 matrix
    Mat3f m3{
        1.0f, 2.0f, 3.0f,
        0.0f, 1.0f, 4.0f,
        5.0f, 6.0f, 0.0f
    };
    
    float det3 = m3.determinant();
    EXPECT_FLOAT_EQ(det3, 1.0f); // 1*(1*0 - 4*6) - 2*(0*0 - 4*5) + 3*(0*6 - 1*5) = -24 + 40 - 15 = 1
}

// Transform matrices
TEST_F(MathTest, TransformMatrices) {
    Vec3f offset{1.0f, 2.0f, 3.0f};
    Mat4f trans = translation(offset);
    
    Vec4f point{0.0f, 0.0f, 0.0f, 1.0f};
    Vec4f transformed = trans * point;
    
    EXPECT_FLOAT_EQ(transformed[0], 1.0f);
    EXPECT_FLOAT_EQ(transformed[1], 2.0f);
    EXPECT_FLOAT_EQ(transformed[2], 3.0f);
    EXPECT_FLOAT_EQ(transformed[3], 1.0f);
}

TEST_F(MathTest, RotationMatrices) {
    // Test 90-degree rotation around Y axis
    Mat4f rot_y = rotation_y(std::numbers::pi_v<float> / 2.0f);
    
    Vec4f x_axis{1.0f, 0.0f, 0.0f, 0.0f};
    Vec4f rotated = rot_y * x_axis;
    
    // After 90-degree Y rotation, X axis should point toward negative Z
    EXPECT_NEAR(rotated[0], 0.0f, 1e-6f);
    EXPECT_NEAR(rotated[1], 0.0f, 1e-6f);
    EXPECT_NEAR(rotated[2], -1.0f, 1e-6f);
    EXPECT_NEAR(rotated[3], 0.0f, 1e-6f);
}

// SIMD Vec4f tests
TEST_F(MathTest, SIMDVector4Operations) {
    Vector<float, 4> a{1.0f, 2.0f, 3.0f, 4.0f};
    Vector<float, 4> b{5.0f, 6.0f, 7.0f, 8.0f};
    
    Vector<float, 4> sum = a + b;
    EXPECT_FLOAT_EQ(sum[0], 6.0f);
    EXPECT_FLOAT_EQ(sum[1], 8.0f);
    EXPECT_FLOAT_EQ(sum[2], 10.0f);
    EXPECT_FLOAT_EQ(sum[3], 12.0f);
    
    Vector<float, 4> scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled[0], 2.0f);
    EXPECT_FLOAT_EQ(scaled[1], 4.0f);
    EXPECT_FLOAT_EQ(scaled[2], 6.0f);
    EXPECT_FLOAT_EQ(scaled[3], 8.0f);
    
    float dot = a.dot(b);
    EXPECT_FLOAT_EQ(dot, 70.0f); // 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
}

// Utility function tests
TEST_F(MathTest, DistanceFunctions) {
    Vec3f a{0.0f, 0.0f, 0.0f};
    Vec3f b{3.0f, 4.0f, 0.0f};
    
    float dist_sq = distance_squared(a, b);
    EXPECT_FLOAT_EQ(dist_sq, 25.0f);
    
    float dist = distance(a, b);
    EXPECT_FLOAT_EQ(dist, 5.0f);
}

TEST_F(MathTest, LerpFunction) {
    Vec3f a{0.0f, 0.0f, 0.0f};
    Vec3f b{10.0f, 20.0f, 30.0f};
    
    Vec3f halfway = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(halfway.x(), 5.0f);
    EXPECT_FLOAT_EQ(halfway.y(), 10.0f);
    EXPECT_FLOAT_EQ(halfway.z(), 15.0f);
    
    Vec3f quarter = lerp(a, b, 0.25f);
    EXPECT_FLOAT_EQ(quarter.x(), 2.5f);
    EXPECT_FLOAT_EQ(quarter.y(), 5.0f);
    EXPECT_FLOAT_EQ(quarter.z(), 7.5f);
}
