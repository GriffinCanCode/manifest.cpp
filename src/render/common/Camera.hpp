#pragma once

#include <numbers>

#include "../../core/math/Matrix.hpp"
#include "../../core/math/Vector.hpp"

namespace Manifest::Render {

using namespace Core::Math;

class Camera {
    Vec3f position_{0.0f, 0.0f, 0.0f};
    Vec3f target_{0.0f, 0.0f, -1.0f};
    Vec3f up_{0.0f, 1.0f, 0.0f};

    float fov_{std::numbers::pi_v<float> / 4.0f};  // 45 degrees
    float aspect_ratio_{16.0f / 9.0f};
    float near_plane_{0.1f};
    float far_plane_{1000.0f};

    mutable Mat4f view_matrix_{Mat4f::identity()};
    mutable Mat4f projection_matrix_{Mat4f::identity()};
    mutable Mat4f view_projection_matrix_{Mat4f::identity()};
    mutable bool view_dirty_{true};
    mutable bool projection_dirty_{true};
    mutable bool view_projection_dirty_{true};

    void update_view_matrix() const {
        if (view_dirty_) {
            view_matrix_ = Core::Math::look_at(position_, target_, up_);
            view_dirty_ = false;
            view_projection_dirty_ = true;
        }
    }

    void update_projection_matrix() const {
        if (projection_dirty_) {
            projection_matrix_ = Core::Math::perspective(fov_, aspect_ratio_, near_plane_, far_plane_);
            projection_dirty_ = false;
            view_projection_dirty_ = true;
        }
    }

    void update_view_projection_matrix() const {
        if (view_projection_dirty_) {
            update_view_matrix();
            update_projection_matrix();
            view_projection_matrix_ = projection_matrix_ * view_matrix_;
            view_projection_dirty_ = false;
        }
    }

   public:
    // Position and orientation
    void set_position(const Vec3f& position) {
        position_ = position;
        view_dirty_ = true;
    }

    void set_target(const Vec3f& target) {
        target_ = target;
        view_dirty_ = true;
    }

    void set_up(const Vec3f& up) {
        up_ = up.normalized();
        view_dirty_ = true;
    }

    void look_at(const Vec3f& position, const Vec3f& target, const Vec3f& up = {0.0f, 1.0f, 0.0f}) {
        position_ = position;
        target_ = target;
        up_ = up.normalized();
        view_dirty_ = true;
    }

    // Projection parameters
    void set_perspective(float fov, float aspect_ratio, float near_plane, float far_plane) {
        fov_ = fov;
        aspect_ratio_ = aspect_ratio;
        near_plane_ = near_plane;
        far_plane_ = far_plane;
        projection_dirty_ = true;
    }

    void set_fov(float fov) {
        fov_ = fov;
        projection_dirty_ = true;
    }

    void set_aspect_ratio(float aspect_ratio) {
        aspect_ratio_ = aspect_ratio;
        projection_dirty_ = true;
    }

    void set_near_plane(float near_plane) {
        near_plane_ = near_plane;
        projection_dirty_ = true;
    }

    void set_far_plane(float far_plane) {
        far_plane_ = far_plane;
        projection_dirty_ = true;
    }

    // Movement
    void move_forward(float distance) {
        Vec3f forward = (target_ - position_).normalized();
        position_ += forward * distance;
        target_ += forward * distance;
        view_dirty_ = true;
    }

    void move_right(float distance) {
        Vec3f forward = (target_ - position_).normalized();
        Vec3f right = forward.cross(up_).normalized();
        position_ += right * distance;
        target_ += right * distance;
        view_dirty_ = true;
    }

    void move_up(float distance) {
        position_ += up_ * distance;
        target_ += up_ * distance;
        view_dirty_ = true;
    }

    // Rotation (around position)
    void rotate_yaw(float angle) {
        Vec3f direction = target_ - position_;
        Mat4f rotation = rotation_y(angle);
        Vec4f rotated = rotation * Vec4f{direction.x(), direction.y(), direction.z(), 0.0f};
        target_ = position_ + Vec3f{rotated[0], rotated[1], rotated[2]};
        view_dirty_ = true;
    }

    void rotate_pitch(float angle) {
        Vec3f direction = target_ - position_;
        Vec3f right = direction.cross(up_).normalized();

        // Create rotation matrix around right axis
        float c = std::cos(angle);
        float s = std::sin(angle);
        Mat3f rotation{right.x() * right.x() * (1 - c) + c,
                       right.x() * right.y() * (1 - c) - right.z() * s,
                       right.x() * right.z() * (1 - c) + right.y() * s,
                       right.y() * right.x() * (1 - c) + right.z() * s,
                       right.y() * right.y() * (1 - c) + c,
                       right.y() * right.z() * (1 - c) - right.x() * s,
                       right.z() * right.x() * (1 - c) - right.y() * s,
                       right.z() * right.y() * (1 - c) + right.x() * s,
                       right.z() * right.z() * (1 - c) + c};

        Vec3f rotated_direction = rotation * direction;
        target_ = position_ + rotated_direction;
        view_dirty_ = true;
    }

    // Orbit (around target)
    void orbit_horizontal(float angle) {
        Vec3f direction = position_ - target_;
        Mat4f rotation = rotation_y(angle);
        Vec4f rotated = rotation * Vec4f{direction.x(), direction.y(), direction.z(), 0.0f};
        position_ = target_ + Vec3f{rotated[0], rotated[1], rotated[2]};
        view_dirty_ = true;
    }

    void orbit_vertical(float angle) {
        Vec3f direction = position_ - target_;
        Vec3f right = direction.cross(up_).normalized();

        float c = std::cos(angle);
        float s = std::sin(angle);
        Mat3f rotation{right.x() * right.x() * (1 - c) + c,
                       right.x() * right.y() * (1 - c) - right.z() * s,
                       right.x() * right.z() * (1 - c) + right.y() * s,
                       right.y() * right.x() * (1 - c) + right.z() * s,
                       right.y() * right.y() * (1 - c) + c,
                       right.y() * right.z() * (1 - c) - right.x() * s,
                       right.z() * right.x() * (1 - c) - right.y() * s,
                       right.z() * right.y() * (1 - c) + right.x() * s,
                       right.z() * right.z() * (1 - c) + c};

        Vec3f rotated_direction = rotation * direction;
        position_ = target_ + rotated_direction;
        view_dirty_ = true;
    }

    void zoom(float factor) {
        Vec3f direction = position_ - target_;
        float distance = direction.length() * factor;
        position_ = target_ + direction.normalized() * distance;
        view_dirty_ = true;
    }

    // Getters
    const Vec3f& position() const noexcept { return position_; }
    const Vec3f& target() const noexcept { return target_; }
    const Vec3f& up() const noexcept { return up_; }

    float fov() const noexcept { return fov_; }
    float aspect_ratio() const noexcept { return aspect_ratio_; }
    float near_plane() const noexcept { return near_plane_; }
    float far_plane() const noexcept { return far_plane_; }

    Vec3f forward() const { return (target_ - position_).normalized(); }

    Vec3f right() const { return forward().cross(up_).normalized(); }

    // Matrices
    const Mat4f& view_matrix() const {
        update_view_matrix();
        return view_matrix_;
    }

    const Mat4f& projection_matrix() const {
        update_projection_matrix();
        return projection_matrix_;
    }

    const Mat4f& view_projection_matrix() const {
        update_view_projection_matrix();
        return view_projection_matrix_;
    }

    // Utility functions
    Vec3f screen_to_world(const Vec2f& screen_coords, const Vec2f& viewport_size,
                          float depth = 1.0f) const {
        // Convert screen coordinates to NDC
        Vec3f ndc{(screen_coords.x() / viewport_size.x()) * 2.0f - 1.0f,
                  1.0f - (screen_coords.y() / viewport_size.y()) * 2.0f, depth * 2.0f - 1.0f};

        // Transform from NDC to world space
        Mat4f inverse_vp = view_projection_matrix();  // Would need proper inverse
        Vec4f world = inverse_vp * Vec4f{ndc.x(), ndc.y(), ndc.z(), 1.0f};

        if (world.w() != 0.0f) {
            return Vec3f{world.x(), world.y(), world.z()} / world.w();
        }

        return Vec3f{world.x(), world.y(), world.z()};
    }

    Vec2f world_to_screen(const Vec3f& world_pos, const Vec2f& viewport_size) const {
        Vec4f clip =
            view_projection_matrix() * Vec4f{world_pos.x(), world_pos.y(), world_pos.z(), 1.0f};

        if (clip.w() != 0.0f) {
            Vec3f ndc = Vec3f{clip.x(), clip.y(), clip.z()} / clip.w();
            return Vec2f{(ndc.x() + 1.0f) * 0.5f * viewport_size.x(),
                         (1.0f - ndc.y()) * 0.5f * viewport_size.y()};
        }

        return Vec2f{};
    }
};

}  // namespace Manifest::Render
