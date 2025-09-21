#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "../../core/math/Matrix.hpp"
#include "../../core/math/Vector.hpp"

namespace Manifest {
namespace Render {
namespace Culling {

using namespace Core::Math;

/**
 * Modern frustum culling system that extracts 6 planes from view-projection matrix
 * and performs efficient culling tests against tile bounding spheres.
 * 
 * Uses SIMD optimizations where available and batch processing for performance.
 */
class FrustumCuller {
public:
    // Plane equation: ax + by + cz + d = 0
    struct Plane {
        Vec3f normal{};
        float distance{0.0f};
        
        constexpr Plane() = default;
        constexpr Plane(const Vec3f& n, float d) : normal(n), distance(d) {}
        constexpr Plane(const Vec4f& plane_eq) 
            : normal(plane_eq.x(), plane_eq.y(), plane_eq.z()), distance(plane_eq.w()) {}
        
        // Distance from point to plane (positive = front, negative = back)
        [[nodiscard]] constexpr float distance_to_point(const Vec3f& point) const noexcept {
            return normal.dot(point) + distance;
        }
        
        // Normalize plane equation
        void normalize() noexcept {
            float length = normal.length();
            if (length > 0.0f) {
                normal /= length;
                distance /= length;
            }
        }
    };
    
    enum class CullResult : std::uint8_t {
        Outside = 0,    // Completely outside frustum
        Inside = 1,     // Completely inside frustum  
        Intersect = 2   // Partially inside/outside
    };
    
private:
    // Standard 6 frustum planes: left, right, bottom, top, near, far
    std::array<Plane, 6> frustum_planes_{};
    
    // Cached view-projection matrix for change detection
    Mat4f cached_view_projection_{Mat4f::identity()};
    bool frustum_dirty_{true};
    
    void extract_frustum_planes(const Mat4f& view_projection) noexcept;
    
public:
    FrustumCuller() = default;
    ~FrustumCuller() = default;
    
    // Non-copyable but movable
    FrustumCuller(const FrustumCuller&) = delete;
    FrustumCuller& operator=(const FrustumCuller&) = delete;
    FrustumCuller(FrustumCuller&&) = default;
    FrustumCuller& operator=(FrustumCuller&&) = default;
    
    /**
     * Update frustum planes from camera's view-projection matrix
     * Only recalculates if the matrix has changed.
     */
    void update_frustum(const Mat4f& view_projection) noexcept;
    
    /**
     * Test point against frustum
     */
    [[nodiscard]] CullResult test_point(const Vec3f& point) const noexcept;
    
    /**
     * Test sphere against frustum (most common case for hex tiles)
     * @param center Sphere center in world space
     * @param radius Sphere radius
     */
    [[nodiscard]] CullResult test_sphere(const Vec3f& center, float radius) const noexcept;
    
    /**
     * Test axis-aligned bounding box against frustum
     */
    [[nodiscard]] CullResult test_aabb(const Vec3f& min_point, const Vec3f& max_point) const noexcept;
    
    /**
     * Batch test multiple spheres for SIMD efficiency
     * @param positions Array of sphere centers
     * @param radius Uniform radius for all spheres
     * @param results Output array of cull results (must be same size as positions)
     */
    void batch_test_spheres(const std::vector<Vec3f>& positions, float radius,
                           std::vector<CullResult>& results) const noexcept;
    
    /**
     * Batch test multiple spheres with individual radii
     */
    void batch_test_spheres(const std::vector<Vec3f>& positions, const std::vector<float>& radii,
                           std::vector<CullResult>& results) const noexcept;
    
    /**
     * Get read-only access to frustum planes (for debugging)
     */
    [[nodiscard]] const std::array<Plane, 6>& get_planes() const noexcept { return frustum_planes_; }
    
    /**
     * Check if frustum needs updating
     */
    [[nodiscard]] bool is_dirty() const noexcept { return frustum_dirty_; }
};

}  // namespace Culling
}  // namespace Render  
}  // namespace Manifest
