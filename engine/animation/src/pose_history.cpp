#include "arut/animation/pose_history.hpp"

#include <algorithm>
#include <cmath>

namespace arut::animation {
namespace {
[[nodiscard]] bool finite(float v) noexcept { return std::isfinite(v); }
[[nodiscard]] bool valid_pose(const Pose& pose) noexcept {
    if(pose.joint_count==0U||pose.joint_count>kMaxPoseJoints) return false;
    for(std::size_t i=0;i<pose.joint_count;++i) {
        const auto& t=pose.joints[i];
        if(!finite(t.translation.x)||!finite(t.translation.y)||!finite(t.translation.z)||
           !finite(t.rotation.x)||!finite(t.rotation.y)||!finite(t.rotation.z)||
           !finite(t.rotation.w)) return false;
    }
    return true;
}
}
bool PoseHistory::push(double timestamp,const Pose& pose,const core::Vec3& velocity) noexcept {
    if(!std::isfinite(timestamp)||!valid_pose(pose)||!finite(velocity.x)||
       !finite(velocity.y)||!finite(velocity.z)) return false;
    if(size_>0U) {
        const auto* last=latest();
        if(!last||timestamp<=last->timestamp_seconds) return false;
        if(last->pose.joint_count!=pose.joint_count) return false;
    }
    frames_[next_]={timestamp,pose,velocity};
    next_=(next_+1U)%kPoseHistoryCapacity;
    size_=std::min(size_+1U,kPoseHistoryCapacity);
    return true;
}
const PoseHistoryFrame* PoseHistory::latest() const noexcept {
    if(size_==0U) return nullptr;
    return &frames_[(next_+kPoseHistoryCapacity-1U)%kPoseHistoryCapacity];
}
const PoseHistoryFrame* PoseHistory::previous(std::size_t back) const noexcept {
    if(back>=size_) return nullptr;
    return &frames_[(next_+kPoseHistoryCapacity-1U-back)%kPoseHistoryCapacity];
}
bool TrajectoryPredictor::predict(
    const TrajectoryPredictionInput& in,
    const std::array<float,kMotionTrajectorySamples>& times,
    std::array<MotionTrajectoryPoint,kMotionTrajectorySamples>& out) noexcept {
    if(!finite(in.world_position.x)||!finite(in.world_position.y)||!finite(in.world_position.z)||
       !finite(in.world_velocity.x)||!finite(in.world_velocity.y)||!finite(in.world_velocity.z)||
       !finite(in.world_acceleration.x)||!finite(in.world_acceleration.y)||!finite(in.world_acceleration.z)||
       !finite(in.facing_yaw_radians)||!finite(in.angular_velocity_radians)||
       !finite(in.maximum_speed)||in.maximum_speed<=0.0F) return false;
    float previous_time=0.0F;
    for(std::size_t i=0;i<times.size();++i) {
        const float t=times[i];
        if(!finite(t)||t<=previous_time) return false;
        previous_time=t;
        float vx=in.world_velocity.x+in.world_acceleration.x*t;
        float vz=in.world_velocity.z+in.world_acceleration.z*t;
        const float speed=std::sqrt(vx*vx+vz*vz);
        if(!finite(speed)) return false;
        if(speed>in.maximum_speed) {
            const float scale=in.maximum_speed/speed;
            vx*=scale; vz*=scale;
        }
        const float dx=in.world_velocity.x*t+0.5F*in.world_acceleration.x*t*t;
        const float dz=in.world_velocity.z*t+0.5F*in.world_acceleration.z*t*t;
        const float yaw=in.facing_yaw_radians+in.angular_velocity_radians*t;
        const float c=std::cos(yaw), sn=std::sin(yaw);
        // Convert world displacement into character-local X/Z coordinates.
        out[i]={t,{c*dx-sn*dz, in.world_velocity.y*t+0.5F*in.world_acceleration.y*t*t,
                    sn*dx+c*dz}};
        if(!finite(out[i].position_offset.x)||!finite(out[i].position_offset.y)||
           !finite(out[i].position_offset.z)) return false;
    }
    return true;
}
} // namespace arut::animation
