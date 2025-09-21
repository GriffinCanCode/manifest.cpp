#include "../utils/TestUtils.hpp"
#include "../../src/core/math/Vector.hpp"
#include "../../src/core/math/Matrix.hpp"

using namespace Manifest::Core::Math;
using namespace Manifest::Test;

class MathPerformanceTest : public DeterministicTest {};

BENCHMARK_TEST(VectorOperations, 1000000)
    Vec3f a = random_vector();
    Vec3f b = random_vector();
    
    Vec3f result = a + b;
    result = result * 2.0f;
    result = result.normalized();
    float dot = result.dot(a);
    (void)dot; // Prevent optimization
END_BENCHMARK_TEST(50000) // 50ms

BENCHMARK_TEST(SIMDVectorOperations, 1000000)
    Vector<float, 4> a{random_in_range(-10.0f, 10.0f), random_in_range(-10.0f, 10.0f), 
                       random_in_range(-10.0f, 10.0f), random_in_range(-10.0f, 10.0f)};
    Vector<float, 4> b{random_in_range(-10.0f, 10.0f), random_in_range(-10.0f, 10.0f), 
                       random_in_range(-10.0f, 10.0f), random_in_range(-10.0f, 10.0f)};
    
    Vector<float, 4> result = a + b;
    result = result * 2.0f;
    float dot = result.dot(a);
    (void)dot;
END_BENCHMARK_TEST(30000) // Should be faster than non-SIMD

BENCHMARK_TEST(MatrixMultiplication, 100000)
    Mat4f a = Mat4f::identity();
    Mat4f b = Mat4f::identity();
    
    // Simulate some transformations
    a[0][3] = random_in_range(-100.0f, 100.0f);
    b[1][3] = random_in_range(-100.0f, 100.0f);
    
    Mat4f result = a * b;
    (void)result;
END_BENCHMARK_TEST(100000) // 100ms
