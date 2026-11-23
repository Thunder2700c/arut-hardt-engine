#include "arut/animation/motion_matching.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace arut::animation {
namespace {

[[nodiscard]] bool finite(float v) noexcept { return std::isfinite(v); }

[[nodiscard]] bool valid_pose_for_schema(const Pose& pose, const PoseSearchSchema& schema) noexcept {
    if (pose.joint_count == 0U || pose.joint_count > kMaxPoseJoints) return false;
    for (const auto index : schema.joints) {
        if (index >= pose.joint_count) return false;
        const auto& t=pose.joints[index];
        const auto& q=t.rotation;
        if (!finite(t.translation.x)||!finite(t.translation.y)||!finite(t.translation.z)||
            !finite(q.x)||!finite(q.y)||!finite(q.z)||!finite(q.w)) return false;
    }
    return true;
}

} // namespace

bool PoseSearchSchema::valid(std::size_t skeleton_joint_count) const noexcept {
    if (joints.empty() || joints.size() > kMaxPoseJoints ||
        joint_weights.size() != joints.size() ||
        !finite(trajectory_weight) || trajectory_weight < 0.0F ||
        !finite(continuity_penalty) || continuity_penalty < 0.0F ||
        !finite(minimum_reselect_seconds) || minimum_reselect_seconds < 0.0F)
        return false;
    for (std::size_t i=0;i<joints.size();++i) {
        if (joints[i] >= skeleton_joint_count || !finite(joint_weights[i]) ||
            joint_weights[i] < 0.0F) return false;
    }
    for (std::size_t i=0;i<trajectory_times.size();++i) {
        if (!finite(trajectory_times[i]) || trajectory_times[i] <= 0.0F ||
            (i>0U && trajectory_times[i] <= trajectory_times[i-1U])) return false;
    }
    return true;
}

bool MotionMatchingDatabase::make_features(
    const Pose& pose,
    const std::array<MotionTrajectoryPoint,kMotionTrajectorySamples>& trajectory,
    const PoseSearchSchema& schema,
    std::vector<float>& output) const {
    if (!valid_pose_for_schema(pose,schema)) return false;
    output.clear();
    output.reserve(schema.joints.size()*7U+kMotionTrajectorySamples*3U);
    for(const auto index:schema.joints) {
        const auto& t=pose.joints[index];
        const auto& q=t.rotation;
        output.insert(output.end(),{t.translation.x,t.translation.y,t.translation.z,
                                    q.x,q.y,q.z,q.w});
    }
    for(std::size_t i=0;i<trajectory.size();++i) {
        const auto& p=trajectory[i];
        if (!finite(p.time_seconds)||!finite(p.position_offset.x)||
            !finite(p.position_offset.y)||!finite(p.position_offset.z)||
            std::abs(p.time_seconds-schema.trajectory_times[i])>1.0e-3F) return false;
        output.insert(output.end(),{p.position_offset.x,p.position_offset.y,p.position_offset.z});
    }
    return true;
}

bool MotionMatchingDatabase::build(
    const std::vector<MotionClipSource>& clips,
    const PoseSearchSchema& schema,
    float sample_rate_hz) {
    samples_.clear();
    feature_count_=0U;
    if (!finite(sample_rate_hz)||sample_rate_hz<1.0F||sample_rate_hz>120.0F||
        clips.empty()) return false;
    std::size_t joint_count=0U;
    for(const auto& source:clips) {
        if(!source.clip||source.clip->keys.empty()) return false;
        joint_count=source.clip->keys.front().pose.joint_count;
        break;
    }
    if(!schema.valid(joint_count)) return false;
    schema_=schema;
    feature_count_=schema.joints.size()*7U+kMotionTrajectorySamples*3U;
    const float step=1.0F/sample_rate_hz;
    std::vector<float> features;
    for(std::size_t clip_index=0;clip_index<clips.size();++clip_index) {
        const auto& source=clips[clip_index];
        if(!source.clip||!source.clip->valid(joint_count)) { samples_.clear(); feature_count_=0U; return false; }
        const float duration=source.clip->duration_seconds;
        for(float time=0.0F;time<duration;time+=step) {
            Pose current{};
            if(!source.clip->sample(time,current)) { samples_.clear(); feature_count_=0U; return false; }
            std::array<MotionTrajectoryPoint,kMotionTrajectorySamples> trajectory{};
            for(std::size_t i=0;i<trajectory.size();++i) {
                const float future=std::min(duration,time+schema.trajectory_times[i]);
                Pose future_pose{};
                if(!source.clip->sample(future,future_pose)) { samples_.clear(); feature_count_=0U; return false; }
                const auto& root_now=current.joints[0].translation;
                const auto& root_future=future_pose.joints[0].translation;
                trajectory[i]={schema.trajectory_times[i],
                    {root_future.x-root_now.x,root_future.y-root_now.y,root_future.z-root_now.z}};
            }
            if(!make_features(current,trajectory,schema,features)) { samples_.clear(); feature_count_=0U; return false; }
            Sample sample{};
            sample.clip_index=static_cast<std::uint32_t>(clip_index);
            sample.tags=source.tags;
            sample.time_seconds=time;
            sample.features=features;
            samples_.push_back(std::move(sample));
        }
    }
    return !samples_.empty();
}

MotionMatchResult MotionMatchingDatabase::search(const MotionMatchQuery& query) const {
    MotionMatchResult best{};
    if(samples_.empty()||!schema_.valid(query.current_pose.joint_count)) return best;
    std::vector<float> query_features;
    if(!make_features(query.current_pose,query.trajectory,schema_,query_features)||
       query_features.size()!=feature_count_) return best;
    for(std::size_t sample_index=0;sample_index<samples_.size();++sample_index) {
        const auto& candidate=samples_[sample_index];
        if((candidate.tags&query.required_tags)!=query.required_tags||
           (candidate.tags&query.forbidden_tags)!=0U) continue;
        float cost=0.0F;
        std::size_t feature=0U;
        for(std::size_t joint=0;joint<schema_.joints.size();++joint) {
            const float weight=schema_.joint_weights[joint];
            for(std::size_t component=0;component<7U;++component,++feature) {
                const float difference=query_features[feature]-candidate.features[feature];
                cost+=weight*difference*difference;
            }
        }
        for(;feature<feature_count_;++feature) {
            const float difference=query_features[feature]-candidate.features[feature];
            cost+=schema_.trajectory_weight*difference*difference;
        }
        if(candidate.clip_index==query.previous_clip_index&&query.previous_time_seconds>=0.0F) {
            const float delta=std::abs(candidate.time_seconds-query.previous_time_seconds);
            if(delta<schema_.minimum_reselect_seconds) cost-=schema_.continuity_penalty;
            else cost+=schema_.continuity_penalty*std::min(delta,1.0F);
        } else if(query.previous_clip_index!=std::numeric_limits<std::uint32_t>::max()) {
            cost+=schema_.continuity_penalty;
        }
        cost=std::max(0.0F,cost);
        if(cost<best.cost) {
            best.found=true;
            best.clip_index=candidate.clip_index;
            best.time_seconds=candidate.time_seconds;
            best.cost=cost;
            best.database_sample_index=sample_index;
        }
    }
    return best;
}

} // namespace arut::animation
