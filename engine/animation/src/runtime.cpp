#include "arut/animation/runtime.hpp"

#include <algorithm>
#include <cmath>

namespace arut::animation {
namespace {

[[nodiscard]] bool finite(float value) noexcept { return std::isfinite(value); }

[[nodiscard]] Quaternion normalized(Quaternion q) noexcept {
    const float length2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (!finite(length2) || length2 <= 1.0e-12F) return {};
    const float inv = 1.0F / std::sqrt(length2);
    return {q.x*inv, q.y*inv, q.z*inv, q.w*inv};
}

[[nodiscard]] Quaternion slerp(Quaternion a, Quaternion b, float alpha) noexcept {
    a = normalized(a);
    b = normalized(b);
    float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    if (dot < 0.0F) {
        dot = -dot;
        b = {-b.x, -b.y, -b.z, -b.w};
    }
    if (dot > 0.9995F) {
        return normalized({a.x + alpha*(b.x-a.x), a.y + alpha*(b.y-a.y),
                           a.z + alpha*(b.z-a.z), a.w + alpha*(b.w-a.w)});
    }
    dot = std::clamp(dot, -1.0F, 1.0F);
    const float angle = std::acos(dot);
    const float sine = std::sin(angle);
    if (std::abs(sine) <= 1.0e-6F) return a;
    const float wa = std::sin((1.0F-alpha)*angle) / sine;
    const float wb = std::sin(alpha*angle) / sine;
    return normalized({wa*a.x+wb*b.x, wa*a.y+wb*b.y,
                        wa*a.z+wb*b.z, wa*a.w+wb*b.w});
}

[[nodiscard]] bool valid_pose(const Pose& pose, std::size_t count) noexcept {
    if (pose.joint_count != count || count == 0U || count > kMaxPoseJoints) return false;
    for (std::size_t i=0; i<count; ++i) {
        const auto& t=pose.joints[i];
        const auto& q=t.rotation;
        const float q2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        if (!finite(t.translation.x) || !finite(t.translation.y) || !finite(t.translation.z) ||
            !finite(t.scale.x) || !finite(t.scale.y) || !finite(t.scale.z) ||
            !finite(q.x) || !finite(q.y) || !finite(q.z) || !finite(q.w) ||
            !finite(q2) || q2 <= 1.0e-12F) return false;
    }
    return true;
}

} // namespace

bool AnimationClip::valid(std::size_t expected_joint_count) const noexcept {
    if (name.empty() || !finite(duration_seconds) || duration_seconds <= 0.0F ||
        expected_joint_count == 0U || expected_joint_count > kMaxPoseJoints || keys.size() < 2U)
        return false;
    float previous = -1.0F;
    for (const auto& key : keys) {
        if (!finite(key.time_seconds) || key.time_seconds < 0.0F ||
            key.time_seconds <= previous || key.time_seconds > duration_seconds ||
            !valid_pose(key.pose, expected_joint_count)) return false;
        previous = key.time_seconds;
    }
    if (keys.front().time_seconds != 0.0F || keys.back().time_seconds > duration_seconds) return false;
    float previous_event = -1.0F;
    for (const auto& event : events) {
        if (!finite(event.time_seconds) || event.time_seconds < 0.0F || event.time_seconds >= duration_seconds || event.time_seconds < previous_event || event.name.empty()) return false;
        previous_event = event.time_seconds;
    }
    return true;
}

bool AnimationClip::sample(float time_seconds, Pose& output) const noexcept {
    if (keys.size() < 2U || !finite(time_seconds) || !finite(duration_seconds) ||
        duration_seconds <= 0.0F) return false;
    const std::size_t count = keys.front().pose.joint_count;
    if (!valid(count)) return false;
    const float t = std::clamp(time_seconds, 0.0F, duration_seconds);
    const auto upper = std::upper_bound(keys.begin(), keys.end(), t,
        [](float value, const PoseKey& key) { return value < key.time_seconds; });
    if (upper == keys.begin()) { output = keys.front().pose; return true; }
    if (upper == keys.end()) { output = keys.back().pose; return true; }
    const auto& b = *upper;
    const auto& a = *(upper-1);
    const float span = b.time_seconds-a.time_seconds;
    const float alpha = span > 1.0e-7F ? std::clamp((t-a.time_seconds)/span,0.0F,1.0F) : 0.0F;
    output.joint_count=count;
    for(std::size_t i=0;i<count;++i) {
        const auto& x=a.pose.joints[i];
        const auto& y=b.pose.joints[i];
        auto& dst=output.joints[i];
        dst.translation={x.translation.x+(y.translation.x-x.translation.x)*alpha,
                         x.translation.y+(y.translation.y-x.translation.y)*alpha,
                         x.translation.z+(y.translation.z-x.translation.z)*alpha};
        dst.rotation=slerp(x.rotation,y.rotation,alpha);
        dst.scale={x.scale.x+(y.scale.x-x.scale.x)*alpha,
                   x.scale.y+(y.scale.y-x.scale.y)*alpha,
                   x.scale.z+(y.scale.z-x.scale.z)*alpha};
    }
    return true;
}

void AnimationPlayer::set_clip(const AnimationClip* clip, bool restart) noexcept {
    clip_ = clip;
    if (restart) time_seconds_ = 0.0F;
    if (!clip_) time_seconds_ = 0.0F;
    looping_ = clip_ ? clip_->looping : true;
}

bool AnimationPlayer::update(float delta_seconds, Pose& output) noexcept {
    if (!clip_ || !clip_->valid(clip_->keys.empty()?0U:clip_->keys.front().pose.joint_count) || !finite(delta_seconds) || delta_seconds < 0.0F || !finite(playback_rate_)) return false;
    time_seconds_ += std::min(delta_seconds, 0.25F) * playback_rate_;
    if (looping_) {
        time_seconds_ = std::fmod(time_seconds_, clip_->duration_seconds);
        if (time_seconds_ < 0.0F) time_seconds_ += clip_->duration_seconds;
    } else {
        time_seconds_ = std::clamp(time_seconds_, 0.0F, clip_->duration_seconds);
    }
    return clip_->sample(time_seconds_, output);
}

bool AnimationPlayer::update(float delta_seconds, Pose& output, std::vector<AnimationEvent>& output_events) {
    if (!clip_ || !clip_->valid(clip_->keys.empty()?0U:clip_->keys.front().pose.joint_count) || !finite(delta_seconds) || delta_seconds < 0.0F || !finite(playback_rate_)) return false;
    const float duration = clip_->duration_seconds;
    const float previous = time_seconds_;
    const float advance = std::min(delta_seconds, 0.25F) * playback_rate_;
    const float unwrapped = previous + advance;
    const float next = looping_ ? (unwrapped < 0.0F ? std::fmod(unwrapped, duration) + duration : std::fmod(unwrapped, duration)) : std::clamp(unwrapped, 0.0F, duration);
    if (!clip_->events.empty() && advance != 0.0F) {
        if (looping_) {
            const double low = std::min<double>(previous, unwrapped);
            const double high = std::max<double>(previous, unwrapped);
            if (high / duration > 4096.0) return false;
            const auto first_cycle = static_cast<long long>(std::floor(low / duration)) - 1LL;
            const auto last_cycle = static_cast<long long>(std::floor(high / duration)) + 1LL;
            if (last_cycle - first_cycle > 4096LL) return false;
            if (advance > 0.0F) {
                for (long long cycle = first_cycle; cycle <= last_cycle; ++cycle) {
                    const double base = static_cast<double>(cycle) * duration;
                    for (const auto& event : clip_->events) {
                        const double occurrence = base + event.time_seconds;
                        if (occurrence > previous && occurrence <= unwrapped) output_events.push_back(event);
                    }
                }
            } else {
                for (long long cycle = last_cycle; cycle >= first_cycle; --cycle) {
                    const double base = static_cast<double>(cycle) * duration;
                    for (auto it = clip_->events.rbegin(); it != clip_->events.rend(); ++it) {
                        const double occurrence = base + it->time_seconds;
                        if (occurrence >= unwrapped && occurrence < previous) output_events.push_back(*it);
                    }
                }
            }
        } else if (advance > 0.0F) {
            for (const auto& event : clip_->events) if (event.time_seconds > previous && event.time_seconds <= next) output_events.push_back(event);
        } else {
            for (auto it = clip_->events.rbegin(); it != clip_->events.rend(); ++it) if (it->time_seconds >= next && it->time_seconds < previous) output_events.push_back(*it);
        }
    }
    time_seconds_ = next;
    return clip_->sample(time_seconds_, output);
}

void AnimationPlayer::set_playback_rate(float rate) noexcept {
    if (finite(rate)) playback_rate_ = std::clamp(rate, -4.0F, 4.0F);
}

bool AnimationPlayer::seek(float time_seconds) noexcept {
    if (!clip_ || !finite(time_seconds) ||
        !clip_->valid(clip_->keys.empty() ? 0U : clip_->keys.front().pose.joint_count))
        return false;
    time_seconds_ = looping_ ? std::fmod(time_seconds, clip_->duration_seconds)
                             : std::clamp(time_seconds, 0.0F, clip_->duration_seconds);
    if (time_seconds_ < 0.0F) time_seconds_ += clip_->duration_seconds;
    return true;
}

void AnimationPlayer::set_looping(bool looping) noexcept { looping_ = looping; }

bool blend_poses(const Pose& from, const Pose& to, float alpha, Pose& output) noexcept {
    if (!finite(alpha) || !valid_pose(from, from.joint_count) ||
        !valid_pose(to, to.joint_count) || from.joint_count != to.joint_count) return false;
    const float a=std::clamp(alpha,0.0F,1.0F);
    output.joint_count=from.joint_count;
    for(std::size_t i=0;i<output.joint_count;++i) {
        const auto& x=from.joints[i]; const auto& y=to.joints[i]; auto& d=output.joints[i];
        d.translation={x.translation.x+(y.translation.x-x.translation.x)*a,
                       x.translation.y+(y.translation.y-x.translation.y)*a,
                       x.translation.z+(y.translation.z-x.translation.z)*a};
        d.rotation=slerp(x.rotation,y.rotation,a);
        d.scale={x.scale.x+(y.scale.x-x.scale.x)*a,
                  x.scale.y+(y.scale.y-x.scale.y)*a,
                  x.scale.z+(y.scale.z-x.scale.z)*a};
    }
    return true;
}

Pose make_reference_pose(const core::HumanoidSkeleton& skeleton) noexcept {
    Pose pose{};
    if (!skeleton.valid()) return pose;
    pose.joint_count=skeleton.bones.size();
    for(std::size_t i=0;i<pose.joint_count;++i) {
        pose.joints[i].translation=skeleton.bones[i].rest_translation;
        pose.joints[i].rotation={};
    }
    return pose;
}

} // namespace arut::animation
