#include "arut/animation/motion_matching.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace arut::animation;

static AnimationClip make_clip(const char* name,float direction) {
    Pose a{}; a.joint_count=arut::core::kHumanoidBoneCount;
    Pose b=a;
    b.joints[0].translation.x=direction;
    b.joints[1].translation.y=0.9F;
    AnimationClip clip{};
    clip.name=name; clip.duration_seconds=1.0F; clip.looping=true;
    clip.keys={{0.0F,a},{1.0F,b}};
    return clip;
}

int main() {
    auto forward=make_clip("forward",1.0F);
    auto backward=make_clip("backward",-1.0F);
    PoseSearchSchema schema{};
    schema.joints={0U,1U,2U,3U,14U,17U};
    schema.joint_weights={1.0F,1.0F,0.5F,0.5F,0.8F,0.8F};
    MotionMatchingDatabase database;
    assert(database.build({{&forward,1U},{&backward,2U}},schema,10.0F));
    assert(database.sample_count()>=20U);
    assert(database.feature_count()==schema.joints.size()*7U+9U);

    MotionMatchQuery query{};
    query.current_pose=forward.keys.front().pose;
    query.required_tags=1U;
    for(std::size_t i=0;i<query.trajectory.size();++i)
        query.trajectory[i]={schema.trajectory_times[i],{}};
    const auto result=database.search(query);
    assert(result.found);
    assert(result.clip_index==0U);

    query.required_tags=2U;
    const auto filtered=database.search(query);
    assert(filtered.found&&filtered.clip_index==1U);

    query.required_tags=4U;
    assert(!database.search(query).found);

    auto invalid=schema;
    invalid.joints={99U};
    assert(!database.build({{&forward,1U}},invalid,10.0F));
    std::cout<<"MOTION_MATCHING_TEST pass database="<<database.sample_count()
             <<" features="<<database.feature_count()<<" tag_filter=1 no_match=1\n";
}
