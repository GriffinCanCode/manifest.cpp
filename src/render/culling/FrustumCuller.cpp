#include "FrustumCuller.hpp"

// SIMD intrinsics for batch operations
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#define HAS_SIMD_SUPPORT 1
#else
#define HAS_SIMD_SUPPORT 0
#endif

namespace Manifest {
namespace Render {
namespace Culling {

void FrustumCuller::update_frustum(const Mat4f& view_projection) noexcept {
    // Only update if matrix has changed - do element-wise comparison since Mat4f may not have == operator
    bool matrices_equal = true;
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            if (cached_view_projection_[i][j] != view_projection[i][j]) {
                matrices_equal = false;
                break;
            }
        }
        if (!matrices_equal) break;
    }
    
    if (matrices_equal && !frustum_dirty_) {
        return;
    }
    
    cached_view_projection_ = view_projection;
    frustum_dirty_ = false;
    
    extract_frustum_planes(view_projection);
}

void FrustumCuller::extract_frustum_planes(const Mat4f& view_projection) noexcept {
    // Extract planes using Gribb/Hartmann method
    // Each plane is extracted from rows and columns of the VP matrix
    
    const auto& m = view_projection;
    
    // Left plane: m[3] + m[0]
    frustum_planes_[0] = Plane{
        Vec3f{m[3][0] + m[0][0], m[3][1] + m[0][1], m[3][2] + m[0][2]},
        m[3][3] + m[0][3]
    };
    
    // Right plane: m[3] - m[0]  
    frustum_planes_[1] = Plane{
        Vec3f{m[3][0] - m[0][0], m[3][1] - m[0][1], m[3][2] - m[0][2]},
        m[3][3] - m[0][3]
    };
    
    // Bottom plane: m[3] + m[1]
    frustum_planes_[2] = Plane{
        Vec3f{m[3][0] + m[1][0], m[3][1] + m[1][1], m[3][2] + m[1][2]},
        m[3][3] + m[1][3]
    };
    
    // Top plane: m[3] - m[1]
    frustum_planes_[3] = Plane{
        Vec3f{m[3][0] - m[1][0], m[3][1] - m[1][1], m[3][2] - m[1][2]},
        m[3][3] - m[1][3]
    };
    
    // Near plane: m[3] + m[2]
    frustum_planes_[4] = Plane{
        Vec3f{m[3][0] + m[2][0], m[3][1] + m[2][1], m[3][2] + m[2][2]},
        m[3][3] + m[2][3]
    };
    
    // Far plane: m[3] - m[2]
    frustum_planes_[5] = Plane{
        Vec3f{m[3][0] - m[2][0], m[3][1] - m[2][1], m[3][2] - m[2][2]},
        m[3][3] - m[2][3]
    };
    
    // Normalize all planes for accurate distance calculations
    for (auto& plane : frustum_planes_) {
        plane.normalize();
    }
}

FrustumCuller::CullResult FrustumCuller::test_point(const Vec3f& point) const noexcept {
    for (const auto& plane : frustum_planes_) {
        if (plane.distance_to_point(point) < 0.0f) {
            return CullResult::Outside;
        }
    }
    return CullResult::Inside;
}

FrustumCuller::CullResult FrustumCuller::test_sphere(const Vec3f& center, float radius) const noexcept {
    bool intersects = false;
    
    for (const auto& plane : frustum_planes_) {
        float distance = plane.distance_to_point(center);
        
        if (distance < -radius) {
            // Completely outside this plane
            return CullResult::Outside;
        } else if (distance < radius) {
            // Sphere intersects this plane
            intersects = true;
        }
    }
    
    return intersects ? CullResult::Intersect : CullResult::Inside;
}

FrustumCuller::CullResult FrustumCuller::test_aabb(const Vec3f& min_point, const Vec3f& max_point) const noexcept {
    bool intersects = false;
    
    for (const auto& plane : frustum_planes_) {
        // Find the positive and negative vertices of the AABB relative to the plane normal
        Vec3f positive_vertex{
            plane.normal.x() >= 0.0f ? max_point.x() : min_point.x(),
            plane.normal.y() >= 0.0f ? max_point.y() : min_point.y(), 
            plane.normal.z() >= 0.0f ? max_point.z() : min_point.z()
        };
        
        Vec3f negative_vertex{
            plane.normal.x() >= 0.0f ? min_point.x() : max_point.x(),
            plane.normal.y() >= 0.0f ? min_point.y() : max_point.y(),
            plane.normal.z() >= 0.0f ? min_point.z() : max_point.z()
        };
        
        // If positive vertex is behind plane, AABB is completely outside
        if (plane.distance_to_point(positive_vertex) < 0.0f) {
            return CullResult::Outside;
        }
        
        // If negative vertex is behind plane, AABB intersects
        if (plane.distance_to_point(negative_vertex) < 0.0f) {
            intersects = true;
        }
    }
    
    return intersects ? CullResult::Intersect : CullResult::Inside;
}

void FrustumCuller::batch_test_spheres(const std::vector<Vec3f>& positions, float radius,
                                     std::vector<CullResult>& results) const noexcept {
    results.resize(positions.size());
    if (positions.empty()) {
        return;
    }
    
#if HAS_SIMD_SUPPORT
    // SIMD optimized batch processing for x86/x64
    const std::size_t simd_count = positions.size() & ~3; // Process in groups of 4
    std::size_t i = 0;
    
    // Process 4 positions at a time using SSE/AVX
    for (; i < simd_count; i += 4) {
        // Load 4 positions into SIMD registers (AoS to SoA conversion)
        __m128 x = _mm_setr_ps(positions[i].x(), positions[i+1].x(), 
                              positions[i+2].x(), positions[i+3].x());
        __m128 y = _mm_setr_ps(positions[i].y(), positions[i+1].y(),
                              positions[i+2].y(), positions[i+3].y());
        __m128 z = _mm_setr_ps(positions[i].z(), positions[i+1].z(),
                              positions[i+2].z(), positions[i+3].z());
        
        __m128 radius_vec = _mm_set1_ps(radius);
        __m128 outside_mask = _mm_setzero_ps();
        __m128 intersect_mask = _mm_setzero_ps();
        
        // Test against all 6 planes
        for (const auto& plane : frustum_planes_) {
            __m128 plane_x = _mm_set1_ps(plane.normal.x());
            __m128 plane_y = _mm_set1_ps(plane.normal.y());
            __m128 plane_z = _mm_set1_ps(plane.normal.z());
            __m128 plane_d = _mm_set1_ps(plane.distance);
            
            // Calculate distances: dot(normal, position) + distance
            __m128 distances = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(plane_x, x), _mm_mul_ps(plane_y, y)),
                _mm_add_ps(_mm_mul_ps(plane_z, z), plane_d)
            );
            
            // Check if completely outside (distance < -radius)
            __m128 neg_radius = _mm_sub_ps(_mm_setzero_ps(), radius_vec);
            __m128 is_outside = _mm_cmplt_ps(distances, neg_radius);
            outside_mask = _mm_or_ps(outside_mask, is_outside);
            
            // Check if intersecting (distance < radius)
            __m128 is_intersecting = _mm_cmplt_ps(distances, radius_vec);
            intersect_mask = _mm_or_ps(intersect_mask, is_intersecting);
        }
        
        // Convert SIMD masks to results
        float outside_results[4], intersect_results[4];
        _mm_storeu_ps(outside_results, outside_mask);
        _mm_storeu_ps(intersect_results, intersect_mask);
        
        for (int j = 0; j < 4; ++j) {
            if (outside_results[j] != 0.0f) {
                results[i + j] = CullResult::Outside;
            } else if (intersect_results[j] != 0.0f) {
                results[i + j] = CullResult::Intersect;
            } else {
                results[i + j] = CullResult::Inside;
            }
        }
    }
    
    // Handle remaining positions with scalar code
    for (; i < positions.size(); ++i) {
        results[i] = test_sphere(positions[i], radius);
    }
#else
    // Fallback scalar implementation
    for (std::size_t i = 0; i < positions.size(); ++i) {
        results[i] = test_sphere(positions[i], radius);
    }
#endif
}

void FrustumCuller::batch_test_spheres(const std::vector<Vec3f>& positions, const std::vector<float>& radii,
                                     std::vector<CullResult>& results) const noexcept {
    if (positions.size() != radii.size()) {
        return; // Invalid input
    }
    results.resize(positions.size());
    
    // For variable radii, use scalar implementation for simplicity
    // Could be optimized with gather/scatter operations in AVX2+
    for (std::size_t i = 0; i < positions.size(); ++i) {
        results[i] = test_sphere(positions[i], radii[i]);
    }
}

}  // namespace Culling
}  // namespace Render
}  // namespace Manifest
