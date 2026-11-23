#pragma once

#include "arut/core/humanoid_skeleton.hpp"
#include "arut/rendering/mesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace arut::rendering {

// Built-in, engine-owned humanoid. This is the default character, not a debug
// skeleton: it uses a relaxed A-pose, smooth anatomical volumes and a rigged
// bind-pose mesh. Imported GLB characters remain an optional replacement.
[[nodiscard]] inline MeshData make_robot_mannequin_mesh(
    const core::HumanoidSkeleton& skeleton = core::make_robot_mannequin_skeleton(),
    std::uint32_t radial_segments = 24U) {
    MeshData mesh{};
    if (!skeleton.valid() || radial_segments < 12U || radial_segments > 64U) return mesh;

    using V3 = std::array<float, 3>;
    constexpr float pi = 3.14159265358979323846F;
    const auto add=[](V3 a,V3 b)->V3{return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};};
    const auto sub=[](V3 a,V3 b)->V3{return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};};
    const auto mul=[](V3 a,float s)->V3{return {a[0]*s,a[1]*s,a[2]*s};};
    const auto dot=[](V3 a,V3 b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    const auto cross=[](V3 a,V3 b)->V3{return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
    const auto norm=[&](V3 v)->V3{const float d=dot(v,v);return d>1.0e-12F?mul(v,1.0F/std::sqrt(d)):V3{0,1,0};};
    std::array<V3,core::kHumanoidBoneCount> p{};
    for(std::size_t i=0;i<skeleton.bones.size();++i){
        const auto& b=skeleton.bones[i]; const V3 local{b.rest_translation.x,b.rest_translation.y,b.rest_translation.z};
        p[i]=b.parent<0?local:add(p[static_cast<std::size_t>(b.parent)],local);
    }

    const auto append_ellipsoid=[&](V3 center,V3 radii,std::uint32_t joint){
        constexpr std::uint32_t rings=16U;
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        for(std::uint32_t r=0;r<=rings;++r){
            const float phi=pi*static_cast<float>(r)/static_cast<float>(rings);
            for(std::uint32_t s=0;s<=radial_segments;++s){
                const float theta=2.0F*pi*static_cast<float>(s)/static_cast<float>(radial_segments);
                const V3 unit{std::sin(phi)*std::cos(theta),std::cos(phi),std::sin(phi)*std::sin(theta)};
                const V3 pos=add(center,{unit[0]*radii[0],unit[1]*radii[1],unit[2]*radii[2]});
                const V3 normal=norm({unit[0]/radii[0],unit[1]/radii[1],unit[2]/radii[2]});
                MeshVertex v{pos,normal,{1,0,0,1},{static_cast<float>(s)/radial_segments,static_cast<float>(r)/rings}};
                v.joints={joint,0,0,0}; v.weights={1,0,0,0}; mesh.vertices.push_back(v);
            }
        }
        for(std::uint32_t r=0;r<rings;++r) for(std::uint32_t s=0;s<radial_segments;++s){
            const std::uint32_t a=base+r*(radial_segments+1U)+s,b=a+radial_segments+1U;
            mesh.indices.insert(mesh.indices.end(),{a,a+1U,b,a+1U,b+1U,b});
        }
    };

    // Smooth, rounded limb volume aligned to the actual rig segment. Joint
    // weights blend continuously from parent to child so animation bends it.
    const auto append_limb=[&](V3 start,V3 end,float radius,float depth,
                               std::uint32_t parent,std::uint32_t child){
        const V3 axis=norm(sub(end,start));
        V3 tangent = std::abs(axis[2]) < 0.85F
            ? norm(cross(axis, {0,0,1}))
            : norm(cross(axis, {1,0,0}));
        const V3 bitangent=norm(cross(axis,tangent));
        constexpr std::uint32_t rings=16U;
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        const V3 midpoint=mul(add(start,end),0.5F);
        const float half_length=std::sqrt(dot(sub(end,start),sub(end,start)))*0.5F+radius*0.72F;
        for(std::uint32_t r=0;r<=rings;++r){
            const float t=-1.0F+2.0F*static_cast<float>(r)/static_cast<float>(rings);
            const float profile=std::sqrt(std::max(0.0F,1.0F-t*t));
            const V3 center=add(midpoint,mul(axis,t*half_length));
            for(std::uint32_t s=0;s<=radial_segments;++s){
                const float a=2.0F*pi*static_cast<float>(s)/static_cast<float>(radial_segments);
                const V3 radial=add(mul(tangent,std::cos(a)),mul(bitangent,std::sin(a)));
                const V3 pos=add(center,add(mul(tangent,std::cos(a)*radius*profile),
                                                mul(bitangent,std::sin(a)*depth*profile)));
                const V3 normal=norm(add(mul(tangent,std::cos(a)/radius),
                                           mul(bitangent,std::sin(a)/depth)));
                MeshVertex v{pos,normal,{1,0,0,1},{static_cast<float>(s)/radial_segments,static_cast<float>(r)/rings}};
                const float child_weight=std::clamp((t+1.0F)*0.5F,0.0F,1.0F);
                v.joints={parent,child,0,0}; v.weights={1.0F-child_weight,child_weight,0,0};
                mesh.vertices.push_back(v);
            }
        }
        for(std::uint32_t r=0;r<rings;++r)for(std::uint32_t s=0;s<radial_segments;++s){
            const std::uint32_t a=base+r*(radial_segments+1U)+s,b=a+radial_segments+1U;
            mesh.indices.insert(mesh.indices.end(),{a,a+1U,b,a+1U,b+1U,b});
        }
    };

    using B=core::HumanoidBone;
    const auto idx=[](B b){return static_cast<std::uint32_t>(b);};
    // Torso is layered as pelvis, abdomen, ribcage and shoulder mass rather
    // than one cylindrical trunk. All volumes are weighted to nearby bones.
    append_ellipsoid(add(p[idx(B::Pelvis)],{0,0.01F,0}),{0.205F,0.145F,0.135F},idx(B::Pelvis));
    append_ellipsoid(add(p[idx(B::Spine)],{0,0.105F,0}),{0.155F,0.19F,0.115F},idx(B::Spine));
    append_ellipsoid(add(p[idx(B::Chest)],{0,0.075F,0}),{0.255F,0.205F,0.145F},idx(B::Chest));
    append_ellipsoid(add(p[idx(B::Chest)],{0,0.19F,0}),{0.275F,0.105F,0.135F},idx(B::Chest));
    append_limb(p[idx(B::Chest)],p[idx(B::Neck)],0.075F,0.075F,idx(B::Chest),idx(B::Neck));
    append_ellipsoid(add(p[idx(B::Head)],{0,0.025F,0}),{0.125F,0.16F,0.115F},idx(B::Head));
    // Jaw and cheek volumes make the head read as a designed humanoid rather
    // than a featureless sphere, while remaining a neutral mannequin.
    append_ellipsoid(add(p[idx(B::Head)],{0,-0.075F,-0.012F}),{0.092F,0.075F,0.095F},idx(B::Head));

    for(const auto side:{B::LeftClavicle,B::RightClavicle}){
        const bool left=side==B::LeftClavicle;
        const B upper=left?B::LeftUpperArm:B::RightUpperArm;
        const B lower=left?B::LeftLowerArm:B::RightLowerArm;
        const B hand=left?B::LeftHand:B::RightHand;
        append_limb(p[idx(side)],p[idx(upper)],0.095F,0.105F,idx(side),idx(upper));
        append_ellipsoid(p[idx(upper)],{0.105F,0.105F,0.105F},idx(upper));
        append_limb(p[idx(upper)],p[idx(lower)],0.078F,0.082F,idx(upper),idx(lower));
        append_ellipsoid(p[idx(lower)],{0.073F,0.073F,0.073F},idx(lower));
        append_limb(p[idx(lower)],p[idx(hand)],0.061F,0.065F,idx(lower),idx(hand));
        append_ellipsoid(add(p[idx(hand)],{0,-0.018F,-0.035F}),{0.075F,0.055F,0.09F},idx(hand));
        // Small thumb volume adds a readable hand silhouette.
        append_ellipsoid(add(p[idx(hand)],{left?-0.055F:0.055F,-0.018F,-0.005F}),
                         {0.027F,0.045F,0.035F},idx(hand));
    }

    for(const bool left:{true,false}){
        const B upper=left?B::LeftUpperLeg:B::RightUpperLeg;
        const B knee=left?B::LeftLowerLeg:B::RightLowerLeg;
        const B foot=left?B::LeftFoot:B::RightFoot;
        append_limb(p[idx(upper)],p[idx(knee)],0.125F,0.14F,idx(B::Pelvis),idx(upper));
        append_ellipsoid(p[idx(knee)],{0.09F,0.09F,0.095F},idx(knee));
        append_limb(p[idx(knee)],p[idx(foot)],0.087F,0.095F,idx(knee),idx(foot));
        append_ellipsoid(add(p[idx(foot)],{0,-0.035F,-0.105F}),{0.095F,0.06F,0.17F},idx(foot));
    }

    // Keep a conservative triangle budget for mobile Vulkan while increasing
    // silhouette quality substantially over the old low-resolution tube rig.
    return mesh;
}

} // namespace arut::rendering
