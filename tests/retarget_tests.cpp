#include "arut/animation/retarget.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

int main() {
    using namespace arut::animation;
    RetargetRig source{};
    source.joint_count=3U;
    source.joints[0].name="mixamorig:Root";
    source.joints[0].parent=-1;
    source.joints[1].name="mixamorig:Hips";
    source.joints[1].parent=0;
    source.joints[1].rest_local.translation={0.0F,0.5F,0.0F};
    source.joints[2].name="mixamorig:LeftArm";
    source.joints[2].parent=1;
    source.joints[2].rest_local.translation={-0.2F,0.0F,0.0F};

    RetargetRig target{};
    target.joint_count=3U;
    target.joints[0].name="Root";
    target.joints[0].parent=-1;
    target.joints[1].name="Pelvis";
    target.joints[1].parent=0;
    target.joints[1].rest_local.translation={0.0F,0.9F,0.0F};
    target.joints[2].name="LeftUpperArm";
    target.joints[2].parent=1;
    target.joints[2].rest_local.translation={-0.3F,0.0F,0.0F};

    RetargetMap map{};
    assert(build_retarget_map(source,target,map));
    assert(map.pair_count==3U);
    assert(map.unmatched_source==0U&&map.unmatched_target==0U);

    Pose source_pose{};
    source_pose.joint_count=3U;
    for(std::size_t i=0;i<3U;++i) source_pose.joints[i]=source.joints[i].rest_local;
    source_pose.joints[1].translation.y=0.6F;
    source_pose.joints[2].rotation={0.0F,0.0F,0.38268343F,0.92387953F};
    source_pose.joints[2].scale={1.2F,1.2F,1.2F};

    Pose result{};
    assert(retarget_pose(source_pose,source,target,map,result));
    assert(result.joint_count==3U);
    assert(std::abs(result.joints[1].translation.y-1.08F)<1.0e-4F);
    assert(std::abs(result.joints[2].rotation.z-0.38268343F)<1.0e-4F);
    assert(std::abs(result.joints[2].scale.x-1.2F)<1.0e-4F);

    constexpr std::size_t large_count=96U;
    RetargetRig large_source{},large_target{};
    large_source.joint_count=large_count;
    large_target.joint_count=large_count;
    Pose large_pose{};
    large_pose.joint_count=large_count;
    for(std::size_t i=0;i<large_count;++i) {
        large_source.joints[i].name="Bone"+std::to_string(i);
        large_target.joints[i].name="Bone"+std::to_string(i);
        large_source.joints[i].parent=i==0U?-1:static_cast<std::int16_t>(i-1U);
        large_target.joints[i].parent=i==0U?-1:static_cast<std::int16_t>(i-1U);
        large_pose.joints[i]=large_source.joints[i].rest_local;
    }
    large_pose.joints[95].translation.x=0.2F;
    RetargetMap large_map{};
    assert(build_retarget_map(large_source,large_target,large_map));
    assert(large_map.pair_count==large_count);
    Pose large_result{};
    assert(retarget_pose(large_pose,large_source,large_target,large_map,large_result));
    assert(large_result.joint_count==large_count);
    assert(std::abs(large_result.joints[95].translation.x-0.2F)<1.0e-5F);
    // Retarget a complete imported clip once for playback on the engine rig.
    AnimationClip source_clip{};
    source_clip.name="Walk";
    source_clip.duration_seconds=1.0F;
    source_clip.looping=true;
    PoseKey source_key_a{};
    source_key_a.time_seconds=0.0F;
    source_key_a.pose=source_pose;
    PoseKey source_key_b{};
    source_key_b.time_seconds=1.0F;
    source_key_b.pose=source_pose;
    source_key_b.pose.joints[2].rotation={0.0F,0.0F,0.70710678F,0.70710678F};
    source_clip.keys={source_key_a,source_key_b};
    AnimationClip target_clip{};
    assert(retarget_clip(source_clip,source,target,map,target_clip));
    assert(target_clip.valid(target.joint_count));
    assert(target_clip.keys.size()==2U);
    assert(std::abs(target_clip.keys[1].pose.joints[2].rotation.z-0.70710678F)<1.0e-4F);
    // Unreal Manny/Quinn names map to the engine's canonical humanoid names.
    RetargetRig unreal{};
    unreal.joint_count=6U;
    unreal.joints[0].name="root";
    unreal.joints[0].parent=-1;
    unreal.joints[1].name="pelvis";
    unreal.joints[1].parent=0;
    unreal.joints[2].name="spine_01";
    unreal.joints[2].parent=1;
    unreal.joints[3].name="upperarm_l";
    unreal.joints[3].parent=2;
    unreal.joints[4].name="lowerarm_r";
    unreal.joints[4].parent=2;
    unreal.joints[5].name="thigh_l";
    unreal.joints[5].parent=1;
    RetargetRig engine_rig{};
    engine_rig.joint_count=6U;
    engine_rig.joints[0].name="Root";
    engine_rig.joints[0].parent=-1;
    engine_rig.joints[1].name="Pelvis";
    engine_rig.joints[1].parent=0;
    engine_rig.joints[2].name="Spine";
    engine_rig.joints[2].parent=1;
    engine_rig.joints[3].name="LeftUpperArm";
    engine_rig.joints[3].parent=2;
    engine_rig.joints[4].name="RightLowerArm";
    engine_rig.joints[4].parent=2;
    engine_rig.joints[5].name="LeftUpperLeg";
    engine_rig.joints[5].parent=1;
    RetargetMap unreal_map{};
    assert(build_retarget_map(unreal,engine_rig,unreal_map));
    assert(unreal_map.pair_count==6U);
    assert(unreal_map.unmatched_source==0U&&unreal_map.unmatched_target==0U);
    std::cout<<"RETARGET_TEST pass mapped=3 unreal_aliases=6 large_rig=96 scale_ratio=1.5\n";
}
