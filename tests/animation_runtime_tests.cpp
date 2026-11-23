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
    clip.events={{0.05F,"loop_marker","wrap"},{0.2F,"footstep","left"},{0.8F,"impact","heavy"}};
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

    std::vector<AnimationEvent> fired;
    AnimationPlayer event_player;
    event_player.set_clip(&clip);
    assert(event_player.update(0.25F, sampled, fired));
    assert(fired.size()==2U && fired[0].name=="loop_marker" && fired[1].name=="footstep");
    fired.clear();
    assert(event_player.update(0.60F, sampled, fired));
    assert(std::abs(event_player.time_seconds()-0.50F)<1.0e-5F);
    assert(fired.empty()); // A long frame is capped at 0.25 seconds.
    assert(event_player.update(0.25F, sampled, fired));
    assert(fired.empty());
    assert(event_player.update(0.10F, sampled, fired));
    assert(fired.size()==1U && fired[0].name=="impact");
    fired.clear();
    assert(event_player.update(0.25F, sampled, fired));
    assert(fired.size()==1U && fired[0].name=="loop_marker");
    event_player.set_playback_rate(-1.0F);
    fired.clear();
    assert(event_player.update(0.10F, sampled, fired));
    assert(fired.size()==1U && fired[0].name=="loop_marker");
    Pose additive=rest;
    additive.joints[1].translation.x += 1.0F;
    additive.joints[1].rotation={0.0F,0.0F,0.70710678F,0.70710678F};
    additive.joints[1].scale={2.0F,2.0F,2.0F};
    Pose layered{};
    assert(apply_additive_layer(rest,additive,rest,0.5F,layered));
    assert(std::abs(layered.joints[1].translation.x-(rest.joints[1].translation.x+0.5F))<1.0e-4F);
    assert(std::abs(layered.joints[1].scale.x-1.5F)<1.0e-4F);
    assert(std::abs(layered.joints[1].rotation.z-0.38268343F)<1.0e-4F);
    Pose bad_reference=rest;
    bad_reference.joints[1].scale.x=0.0F;
    assert(!apply_additive_layer(rest,additive,bad_reference,0.5F,layered));

    auto invalid_event=clip;
    invalid_event.events={{1.0F,"invalid",""}};
    assert(!invalid_event.valid(arut::core::kHumanoidBoneCount));

    auto invalid=clip;
    invalid.keys[1].time_seconds=0.0F;
    assert(!invalid.valid(arut::core::kHumanoidBoneCount));
    test_large_skeleton();
    std::cout << "ANIMATION_RUNTIME_TEST pass clip_sample=1 player_loop=1 pose_blend=1 additive_layers=1 event_tracks=1 invalid_data=1 large_rig=96\n";
    return 0;
}
