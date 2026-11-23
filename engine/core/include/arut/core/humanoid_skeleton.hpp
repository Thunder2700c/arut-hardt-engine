#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace arut::core {

// Stable humanoid joint order. Parent indices always precede their children.
enum class HumanoidBone : std::uint8_t {
    Root, Pelvis, Spine, Chest, Neck, Head,
    LeftClavicle, LeftUpperArm, LeftLowerArm, LeftHand,
    RightClavicle, RightUpperArm, RightLowerArm, RightHand,
    LeftUpperLeg, LeftLowerLeg, LeftFoot,
    RightUpperLeg, RightLowerLeg, RightFoot,
    Count
};

inline constexpr std::size_t kHumanoidBoneCount =
    static_cast<std::size_t>(HumanoidBone::Count);

struct Vec3 final {
    float x{0.0F}, y{0.0F}, z{0.0F};
};

struct BoneDefinition final {
    HumanoidBone bone{};
    std::int16_t parent{-1};
    Vec3 rest_translation{};
    float visual_radius{0.05F};
};

struct HumanoidSkeleton final {
    std::array<BoneDefinition, kHumanoidBoneCount> bones{};

    [[nodiscard]] bool valid() const noexcept {
        for (std::size_t i = 0; i < bones.size(); ++i) {
            const auto& b = bones[i];
            if (static_cast<std::size_t>(b.bone) != i ||
                !std::isfinite(b.rest_translation.x) ||
                !std::isfinite(b.rest_translation.y) ||
                !std::isfinite(b.rest_translation.z) ||
                !std::isfinite(b.visual_radius) || b.visual_radius <= 0.0F)
                return false;
            if (i == 0U) {
                if (b.parent != -1) return false;
            } else if (b.parent < 0 ||
                       static_cast<std::size_t>(b.parent) >= i) {
                return false;
            }
        }
        return true;
    }
};

// Engine-owned, procedural reference rig. Units are metres; +Y is up and -Z is forward.
// It deliberately contains no third-party or copyrighted character assets.
[[nodiscard]] inline HumanoidSkeleton make_robot_mannequin_skeleton() {
    using B = HumanoidBone;
    HumanoidSkeleton s{};
    const auto set = [&](B bone, std::int16_t parent, Vec3 t, float r) {
        s.bones[static_cast<std::size_t>(bone)] = {bone, parent, t, r};
    };
    set(B::Root, -1, {0, 0, 0}, 0.12F);
    set(B::Pelvis, 0, {0, 0.92F, 0}, 0.18F);
    set(B::Spine, 1, {0, 0.22F, 0}, 0.14F);
    set(B::Chest, 2, {0, 0.24F, 0}, 0.22F);
    set(B::Neck, 3, {0, 0.22F, 0}, 0.07F);
    set(B::Head, 4, {0, 0.16F, 0}, 0.13F);
    set(B::LeftClavicle, 3, {-0.18F, 0.15F, 0}, 0.075F);
    set(B::LeftUpperArm, 6, {-0.18F, -0.02F, 0}, 0.085F);
    set(B::LeftLowerArm, 7, {-0.27F, 0, 0}, 0.065F);
    set(B::LeftHand, 8, {-0.23F, 0, 0}, 0.055F);
    set(B::RightClavicle, 3, {0.18F, 0.15F, 0}, 0.075F);
    set(B::RightUpperArm, 10, {0.18F, -0.02F, 0}, 0.085F);
    set(B::RightLowerArm, 11, {0.27F, 0, 0}, 0.065F);
    set(B::RightHand, 12, {0.23F, 0, 0}, 0.055F);
    set(B::LeftUpperLeg, 1, {-0.11F, -0.08F, 0}, 0.105F);
    set(B::LeftLowerLeg, 14, {0, -0.43F, 0}, 0.075F);
    set(B::LeftFoot, 15, {0, -0.42F, -0.10F}, 0.075F);
    set(B::RightUpperLeg, 1, {0.11F, -0.08F, 0}, 0.105F);
    set(B::RightLowerLeg, 17, {0, -0.43F, 0}, 0.075F);
    set(B::RightFoot, 18, {0, -0.42F, -0.10F}, 0.075F);
    if (!s.valid()) throw std::logic_error("invalid built-in humanoid skeleton");
    return s;
}

} // namespace arut::core
