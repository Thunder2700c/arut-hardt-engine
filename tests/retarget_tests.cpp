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
    std::cout<<"RETARGET_TEST pass mapped=3 large_rig=96 scale_ratio=1.5\n";
}
