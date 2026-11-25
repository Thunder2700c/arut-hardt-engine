#pragma once

#include "arut/animation/gltf_importer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <span>
#include <vector>

namespace arut::animation {

struct RetargetJoint final {
    std::string name;
    std::int16_t parent{-1};
    Transform rest_local{};
};

struct RetargetRig final {
    std::array<RetargetJoint,kMaxImportedJoints> joints{};
    std::size_t joint_count{0U};
};

struct RetargetPair final {
    std::uint16_t source_joint{0U};
    std::uint16_t target_joint{0U};
    float translation_scale{1.0F};
};

struct RetargetMap final {
    std::array<RetargetPair,kMaxImportedJoints> pairs{};
    std::size_t pair_count{0U};
    std::size_t unmatched_source{0U};
    std::size_t unmatched_target{0U};
};

[[nodiscard]] RetargetRig make_retarget_rig(const SkeletalAsset& asset);
[[nodiscard]] RetargetRig make_retarget_rig(const core::HumanoidSkeleton& skeleton);
[[nodiscard]] bool build_retarget_map(
    const RetargetRig& source,const RetargetRig& target,RetargetMap& output);
[[nodiscard]] bool retarget_pose(
    const Pose& source_pose,const RetargetRig& source,const RetargetRig& target,
    const RetargetMap& map,Pose& output) noexcept;
[[nodiscard]] bool retarget_clip(
    const AnimationClip& source_clip,const RetargetRig& source,
    const RetargetRig& target,const RetargetMap& map,
    AnimationClip& output);

// Retarget an imported animation pack transactionally. On failure, output is
// left unchanged so callers never install a partially converted library.
[[nodiscard]] bool retarget_animation_library(
    std::span<const AnimationClip> source_clips,
    const RetargetRig& source,const RetargetRig& target,
    const RetargetMap& map,std::vector<AnimationClip>& output);

} // namespace arut::animation
