#include "arut/animation/pose_history.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace arut::animation;
    Pose p{}; p.joint_count=arut::core::kHumanoidBoneCount;
    PoseHistory history;
    for(std::size_t i=0;i<40U;++i) {
        p.joints[0].translation.x=static_cast<float>(i);
        assert(history.push(static_cast<double>(i)*0.016,p,{1.0F,0.0F,0.0F}));
    }
    assert(history.size()==kPoseHistoryCapacity);
    assert(history.latest()->pose.joints[0].translation.x==39.0F);
    assert(history.previous(1U)->pose.joints[0].translation.x==38.0F);
    assert(history.previous(32U)==nullptr);
    assert(!history.push(0.016,p,{0.0F,0.0F,0.0F}));

    TrajectoryPredictionInput input{};
    input.world_velocity={2.0F,0.0F,0.0F};
    input.maximum_speed=5.0F;
    const std::array<float,kMotionTrajectorySamples> times{0.2F,0.4F,0.8F};
    std::array<MotionTrajectoryPoint,kMotionTrajectorySamples> trajectory{};
    assert(TrajectoryPredictor::predict(input,times,trajectory));
    assert(trajectory[2].position_offset.x>trajectory[0].position_offset.x);
    input.world_velocity={2.0F,0.0F,0.0F};
    input.facing_yaw_radians=1.57079632679F;
    assert(TrajectoryPredictor::predict(input,times,trajectory));
    assert(trajectory[2].position_offset.x>-0.001F&&
           trajectory[2].position_offset.x<0.001F);
    assert(trajectory[2].position_offset.z<0.0F);
    input.facing_yaw_radians=0.0F;
    input.world_velocity={20.0F,0.0F,0.0F};
    input.maximum_speed=5.0F;
    assert(TrajectoryPredictor::predict(input,times,trajectory));
    assert(trajectory[2].position_offset.x<=4.001F);
    input.maximum_speed=0.0F;
    assert(!TrajectoryPredictor::predict(input,times,trajectory));
    std::cout<<"POSE_HISTORY_TRAJECTORY_TEST pass ring_capacity=32 trajectory_samples=3\n";
}
