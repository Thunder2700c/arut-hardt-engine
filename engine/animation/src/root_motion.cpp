#include "arut/animation/root_motion.hpp"

#include <cmath>
#include <cctype>
#include <string>

namespace arut::animation {
namespace {
bool finite(float v) noexcept { return std::isfinite(v); }
float yaw_of(const Quaternion& q) noexcept {
    const float siny = 2.0F * (q.w*q.y + q.x*q.z);
    const float cosy = 1.0F - 2.0F * (q.y*q.y + q.z*q.z);
    return std::atan2(siny, cosy);
}
}

bool is_root_motion_joint_name(std::string_view name) noexcept {
    std::string normalized;
    normalized.reserve(name.size());
    for (const unsigned char ch : name) {
        if (std::isalnum(ch))
            normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    return normalized.find("root") != std::string::npos ||
           normalized.find("hips") != std::string::npos ||
           normalized.find("pelvis") != std::string::npos ||
           normalized.find("motion") != std::string::npos;
}

bool extract_root_motion(const Pose& previous, const Pose& current,
                         std::size_t root_joint, RootMotionDelta& output) noexcept {
    if (previous.joint_count == 0U || previous.joint_count != current.joint_count ||
        root_joint >= previous.joint_count || previous.joint_count > kMaxPoseJoints)
        return false;
    const auto& a = previous.joints[root_joint];
    const auto& b = current.joints[root_joint];
    const float values[] = {a.translation.x,a.translation.y,a.translation.z,
        b.translation.x,b.translation.y,b.translation.z,
        a.rotation.x,a.rotation.y,a.rotation.z,a.rotation.w,
        b.rotation.x,b.rotation.y,b.rotation.z,b.rotation.w};
    for (float value : values) if (!finite(value)) return false;
    const float qa = a.rotation.x*a.rotation.x+a.rotation.y*a.rotation.y+
                     a.rotation.z*a.rotation.z+a.rotation.w*a.rotation.w;
    const float qb = b.rotation.x*b.rotation.x+b.rotation.y*b.rotation.y+
                     b.rotation.z*b.rotation.z+b.rotation.w*b.rotation.w;
    if (qa < 1.0e-12F || qb < 1.0e-12F) return false;
    output.local_translation = {b.translation.x-a.translation.x, 0.0F,
                                b.translation.z-a.translation.z};
    output.yaw_radians = std::remainder(yaw_of(b.rotation)-yaw_of(a.rotation),
                                        6.2831853071795864769F);
    return finite(output.local_translation.x) && finite(output.local_translation.z) &&
           finite(output.yaw_radians);
}

bool apply_root_motion(const RootMotionDelta& delta, core::Vec3& world_position,
                       float& world_yaw_radians) noexcept {
    if (!finite(delta.local_translation.x) || !finite(delta.local_translation.z) ||
        !finite(delta.yaw_radians) || !finite(world_position.x) ||
        !finite(world_position.y) || !finite(world_position.z) ||
        !finite(world_yaw_radians)) return false;
    const float c = std::cos(world_yaw_radians);
    const float s = std::sin(world_yaw_radians);
    world_position.x += delta.local_translation.x*c + delta.local_translation.z*s;
    world_position.z += -delta.local_translation.x*s + delta.local_translation.z*c;
    world_yaw_radians = std::remainder(world_yaw_radians + delta.yaw_radians,
                                       6.2831853071795864769F);
    return finite(world_position.x) && finite(world_position.z);
}

bool strip_root_motion(Pose& pose, std::size_t root_joint,
                       const core::Vec3& reference_translation) noexcept {
    if (pose.joint_count == 0U || pose.joint_count > kMaxPoseJoints ||
        root_joint >= pose.joint_count || !finite(reference_translation.x) ||
        !finite(reference_translation.z)) return false;
    auto& root = pose.joints[root_joint].translation;
    if (!finite(root.x) || !finite(root.z)) return false;
    root.x = reference_translation.x;
    root.z = reference_translation.z;
    return true;
}

} // namespace arut::animation
