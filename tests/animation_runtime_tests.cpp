#include "arut/animation/runtime.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace arut::animation;

static AnimationClip make_clip() {
    const auto skeleton=arut::core::make_robot_mannequin_skeleton();
    Pose a=make_reference_pose(skeleton), b=a;
    b.joints[1].translation.y += 1.0F;
    b.joints[1].rotation={0.0F,0.0F,0.70710678F,0.70710678F};
    AnimationClip clip{};
    clip.name="runtime_test";
    clip.duration_seconds=1.0F;
    clip.looping=true;
    clip.keys={{0.0F,a},{1.0F,b}};
    return clip;
}

static void test_large_skeleton() {
    constexpr std::size_t joints=96U;
    Pose rest{};
    rest.joint_count=joints;
    for(std::size_t i=0;i<joints;++i) {
        rest.joints[i].translation={static_cast<float>(i)*0.01F,0.0F,0.0F};
        rest.joints[i].rotation={};
        rest.joints[i].scale={1.0F,1.0F,1.0F};
    }
    Pose action=rest;
    action.joints[95].translation.y=0.75F;
    action.joints[95].rotation={0.0F,0.0F,0.38268343F,0.92387953F};

    AnimationClip clip{};
    clip.name="large_rig_regression";
    clip.duration_seconds=1.0F;
    clip.keys={{0.0F,rest},{1.0F,action}};
    assert(clip.valid(joints));

    Pose sampled{};
    assert(clip.sample(0.5F,sampled));
    assert(sampled.joint_count==joints);
    assert(std::abs(sampled.joints[95].translation.y-0.375F)<1.0e-5F);

    Pose blended{};
    assert(blend_poses(rest,action,0.5F,blended));
    assert(blended.joint_count==joints);
    assert(std::abs(blended.joints[95].translation.y-0.375F)<1.0e-5F);
    assert(!clip.valid(kMaxPoseJoints+1U));
}

int main() {
    const auto clip=make_clip();
    assert(clip.valid(arut::core::kHumanoidBoneCount));
    Pose midpoint{};
    assert(clip.sample(0.5F,midpoint));
    assert(std::abs(midpoint.joints[1].translation.y-
        (arut::core::make_robot_mannequin_skeleton().bones[1].rest_translation.y+0.5F))<1.0e-4F);

    AnimationPlayer player;
    player.set_clip(&clip);
    Pose sampled{};
    assert(player.update(0.25F,sampled));
    assert(std::abs(player.time_seconds()-0.25F)<1.0e-5F);
    assert(player.update(0.90F,sampled));
    assert(std::abs(player.time_seconds()-0.50F)<1.0e-4F);

    Pose blended{};
    const Pose rest=make_reference_pose(arut::core::make_robot_mannequin_skeleton());
    assert(blend_poses(rest,midpoint,0.5F,blended));
    assert(std::abs(blended.joints[1].translation.y-
        (rest.joints[1].translation.y+0.25F))<1.0e-4F);

    auto invalid=clip;
    invalid.keys[1].time_seconds=0.0F;
    assert(!invalid.valid(arut::core::kHumanoidBoneCount));
    test_large_skeleton();
    std::cout << "ANIMATION_RUNTIME_TEST pass clip_sample=1 player_loop=1 pose_blend=1 invalid_data=1 large_rig=96\n";
    return 0;
}
