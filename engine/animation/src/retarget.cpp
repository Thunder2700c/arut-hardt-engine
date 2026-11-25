#include "arut/animation/retarget.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <utility>

namespace arut::animation {
namespace {

bool finite(float v) noexcept { return std::isfinite(v); }

Quaternion normalized(Quaternion q) noexcept {
    const float l2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!finite(l2)||l2<=1.0e-12F) return {};
    const float inv=1.0F/std::sqrt(l2);
    return {q.x*inv,q.y*inv,q.z*inv,q.w*inv};
}
Quaternion conjugate(Quaternion q) noexcept { return {-q.x,-q.y,-q.z,q.w}; }
Quaternion multiply(Quaternion a,Quaternion b) noexcept {
    return normalized({
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z});
}
float length(core::Vec3 v) noexcept {
    return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
}
std::string canonical(std::string name) {
    const auto separator=name.find_last_of(":|/");
    if(separator!=std::string::npos) name=name.substr(separator+1U);
    std::string result;
    result.reserve(name.size());
    for(unsigned char c:name)
        if(std::isalnum(c)) result.push_back(static_cast<char>(std::tolower(c)));
    // Normalize common Mixamo, Unreal Manny/Quinn, and Blender Rigify names.
    if(result=="hips" || result=="pelvis" || result=="bip001pelvis") return "pelvis";
    if(result=="leftarm" || result=="upperarml" || result=="lupperarm") return "leftupperarm";
    if(result=="rightarm" || result=="upperarmr" || result=="rupperarm") return "rightupperarm";
    if(result=="leftforearm" || result=="forearml" || result=="lowerarml" || result=="lforearm") return "leftlowerarm";
    if(result=="rightforearm" || result=="forearmr" || result=="lowerarmr" || result=="rforearm") return "rightlowerarm";
    if(result=="leftupleg" || result=="thighl" || result=="lthigh") return "leftupperleg";
    if(result=="rightupleg" || result=="thighr" || result=="rthigh") return "rightupperleg";
    if(result=="leftleg" || result=="shinl" || result=="calfl" || result=="lcalf") return "leftlowerleg";
    if(result=="rightleg" || result=="shinr" || result=="calfr" || result=="rcalf") return "rightlowerleg";
    if(result=="leftshoulder" || result=="claviclel" || result=="lclavicle") return "leftclavicle";
    if(result=="rightshoulder" || result=="clavicler" || result=="rclavicle") return "rightclavicle";
    if(result=="handl" || result=="lefthand") return "lefthand";
    if(result=="handr" || result=="righthand") return "righthand";
    if(result=="footl" || result=="lfoot") return "leftfoot";
    if(result=="footr" || result=="rfoot") return "rightfoot";
    if(result=="spine01" || result=="spine1") return "spine";
    if(result=="spine02" || result=="spine2" || result=="spine03" || result=="spine3") return "chest";
    if(result=="neck01") return "neck";
    if(result=="root" || result=="armature") return "root";
    return result;
}
bool valid_pose_for(const Pose& pose,std::size_t count) noexcept {
    if(pose.joint_count!=count||count==0U||count>kMaxPoseJoints) return false;
    for(std::size_t i=0;i<count;++i) {
        const auto& t=pose.joints[i];
        const auto& q=t.rotation;
        const float q2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        if(!finite(t.translation.x)||!finite(t.translation.y)||!finite(t.translation.z)||
           !finite(t.scale.x)||!finite(t.scale.y)||!finite(t.scale.z)||
           !finite(q.x)||!finite(q.y)||!finite(q.z)||!finite(q.w)||
           !finite(q2)||q2<=1.0e-12F) return false;
    }
    return true;
}

} // namespace

RetargetRig make_retarget_rig(const SkeletalAsset& asset) {
    RetargetRig rig{};
    if(!asset.valid()) return rig;
    rig.joint_count=asset.joint_count;
    for(std::size_t i=0;i<rig.joint_count;++i)
        rig.joints[i]={asset.joints[i].name,asset.joints[i].parent,asset.joints[i].rest_local};
    return rig;
}

RetargetRig make_retarget_rig(const core::HumanoidSkeleton& skeleton) {
    RetargetRig rig{};
    if(!skeleton.valid()) return rig;
    static constexpr const char* names[]={
        "Root","Pelvis","Spine","Chest","Neck","Head",
        "LeftClavicle","LeftUpperArm","LeftLowerArm","LeftHand",
        "RightClavicle","RightUpperArm","RightLowerArm","RightHand",
        "LeftUpperLeg","LeftLowerLeg","LeftFoot",
        "RightUpperLeg","RightLowerLeg","RightFoot"};
    rig.joint_count=skeleton.bones.size();
    for(std::size_t i=0;i<rig.joint_count;++i) {
        const auto& bone=skeleton.bones[i];
        rig.joints[i].name=names[i];
        rig.joints[i].parent=bone.parent;
        rig.joints[i].rest_local.translation=bone.rest_translation;
    }
    return rig;
}

bool build_retarget_map(const RetargetRig& source,const RetargetRig& target,
                        RetargetMap& output) {
    output={};
    if(source.joint_count==0U||target.joint_count==0U||
       source.joint_count>kMaxImportedJoints||target.joint_count>kMaxImportedJoints)
        return false;
    std::array<bool,kMaxImportedJoints> source_used{};
    std::array<bool,kMaxImportedJoints> target_used{};
    for(std::size_t ti=0;ti<target.joint_count;++ti) {
        const auto target_name=canonical(target.joints[ti].name);
        if(target_name.empty()) { ++output.unmatched_target; continue; }
        for(std::size_t si=0;si<source.joint_count;++si) {
            if(source_used[si]||canonical(source.joints[si].name)!=target_name) continue;
            const float source_length=length(source.joints[si].rest_local.translation);
            const float target_length=length(target.joints[ti].rest_local.translation);
            const float ratio=source_length>1.0e-5F&&target_length>1.0e-5F
                ?target_length/source_length:1.0F;
            if(!finite(ratio)||ratio<=0.0F) return false;
            output.pairs[output.pair_count++]={static_cast<std::uint16_t>(si),
                static_cast<std::uint16_t>(ti),ratio};
            source_used[si]=true;
            target_used[ti]=true;
            break;
        }
        if(!target_used[ti]) ++output.unmatched_target;
    }
    for(std::size_t si=0;si<source.joint_count;++si)
        if(!source_used[si]) ++output.unmatched_source;
    return output.pair_count>0U;
}

bool retarget_pose(const Pose& source_pose,const RetargetRig& source,
                   const RetargetRig& target,const RetargetMap& map,
                   Pose& output) noexcept {
    if(source.joint_count==0U||target.joint_count==0U||
       source_pose.joint_count!=source.joint_count||
       target.joint_count>kMaxPoseJoints||map.pair_count==0U||
       map.pair_count>kMaxImportedJoints||
       !valid_pose_for(source_pose,source.joint_count)) return false;
    output={};
    output.joint_count=target.joint_count;
    for(std::size_t i=0;i<target.joint_count;++i) output.joints[i]=target.joints[i].rest_local;
    std::array<bool,kMaxImportedJoints> source_seen{};
    std::array<bool,kMaxImportedJoints> target_seen{};
    for(std::size_t p=0;p<map.pair_count;++p) {
        const auto& pair=map.pairs[p];
        if(pair.source_joint>=source.joint_count||pair.target_joint>=target.joint_count||
           source_seen[pair.source_joint]||target_seen[pair.target_joint]||
           !finite(pair.translation_scale)||pair.translation_scale<=0.0F) return false;
        source_seen[pair.source_joint]=true;
        target_seen[pair.target_joint]=true;
        const auto& src_rest=source.joints[pair.source_joint].rest_local;
        const auto& src_pose=source_pose.joints[pair.source_joint];
        const auto& dst_rest=target.joints[pair.target_joint].rest_local;
        auto& dst=output.joints[pair.target_joint];
        dst.translation={
            dst_rest.translation.x+(src_pose.translation.x-src_rest.translation.x)*pair.translation_scale,
            dst_rest.translation.y+(src_pose.translation.y-src_rest.translation.y)*pair.translation_scale,
            dst_rest.translation.z+(src_pose.translation.z-src_rest.translation.z)*pair.translation_scale};
        const Quaternion delta=multiply(conjugate(normalized(src_rest.rotation)),
                                         normalized(src_pose.rotation));
        dst.rotation=multiply(normalized(dst_rest.rotation),delta);
        const auto scale_component=[](float target_value,float animated,float rest) noexcept {
            return std::abs(rest)>1.0e-6F?target_value*(animated/rest):target_value;
        };
        dst.scale={scale_component(dst_rest.scale.x,src_pose.scale.x,src_rest.scale.x),
                   scale_component(dst_rest.scale.y,src_pose.scale.y,src_rest.scale.y),
                   scale_component(dst_rest.scale.z,src_pose.scale.z,src_rest.scale.z)};
    }
    return valid_pose_for(output,target.joint_count);
}


bool retarget_clip(const AnimationClip& source_clip,const RetargetRig& source,
                   const RetargetRig& target,const RetargetMap& map,
                   AnimationClip& output) {
    if(!source_clip.valid(source.joint_count)||target.joint_count==0U||
       target.joint_count>kMaxPoseJoints||map.pair_count==0U) return false;
    AnimationClip candidate{};
    candidate.name=source_clip.name;
    candidate.duration_seconds=source_clip.duration_seconds;
    candidate.looping=source_clip.looping;
    candidate.events=source_clip.events;
    candidate.keys.reserve(source_clip.keys.size());
    for(const auto& source_key:source_clip.keys) {
        PoseKey target_key{};
        target_key.time_seconds=source_key.time_seconds;
        if(!retarget_pose(source_key.pose,source,target,map,target_key.pose)) return false;
        candidate.keys.push_back(std::move(target_key));
    }
    if(!candidate.valid(target.joint_count)) return false;
    output=std::move(candidate);
    return true;
}


bool retarget_animation_library(std::span<const AnimationClip> source_clips,
                                const RetargetRig& source,const RetargetRig& target,
                                const RetargetMap& map,
                                std::vector<AnimationClip>& output) {
    if(source_clips.empty()||source.joint_count==0U||target.joint_count==0U||
       target.joint_count>kMaxPoseJoints||map.pair_count==0U) return false;
    std::vector<AnimationClip> candidate;
    candidate.reserve(source_clips.size());
    for(const auto& source_clip:source_clips) {
        AnimationClip target_clip{};
        if(!retarget_clip(source_clip,source,target,map,target_clip)) return false;
        candidate.push_back(std::move(target_clip));
    }
    output=std::move(candidate);
    return true;
}

} // namespace arut::animation
