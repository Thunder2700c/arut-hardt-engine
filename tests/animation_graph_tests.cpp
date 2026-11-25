#include "arut/animation/graph.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace arut::animation;

static AnimationClip make_clip(const char* name, float x) {
    AnimationClip clip{};
    clip.name = name;
    clip.duration_seconds = 1.0F;
    clip.looping = true;
    Pose a{}; a.joint_count = 2U;
    Pose b{}; b.joint_count = 2U;
    a.joints[0].translation.x = x;
    b.joints[0].translation.x = x + 1.0F;
    clip.keys.push_back({0.0F, a});
    clip.keys.push_back({1.0F, b});
    clip.events.push_back({0.1F, "notify", name});
    return clip;
}

int main() {
    auto idle = make_clip("idle", 0.0F);
    auto attack = make_clip("attack", 10.0F);
    AnimationStateMachine graph;
    assert(graph.add_state(1U, "Idle", &idle));
    assert(graph.add_state(2U, "Attack", &attack));
    assert(!graph.add_state(1U, "Duplicate", &idle));
    assert(!graph.start(99U));
    assert(graph.start(1U));

    Pose pose{};
    std::vector<AnimationEvent> events;
    assert(graph.update(0.2F, pose, events));
    assert(events.size() == 1U && events[0].payload == "idle");
    assert(pose.joint_count == 2U);
    assert(pose.joints[0].translation.x > 0.19F &&
           pose.joints[0].translation.x < 0.21F);

    assert(graph.transition_to(2U, 0.4F));
    assert(graph.transitioning());
    events.clear();
    assert(graph.update(0.2F, pose, events));
    assert(events.size() == 1U && events[0].payload == "attack");
    assert(pose.joints[0].translation.x > 5.0F &&
           pose.joints[0].translation.x < 6.0F);
    assert(graph.update(0.2F, pose));
    assert(!graph.transitioning());
    assert(graph.current_state() == 2U);
    assert(pose.joints[0].translation.x > 10.0F);

    // Interrupt an in-flight crossfade and retarget before the first one ends.
    // The presented pose must remain continuous instead of snapping to a clip.
    assert(graph.transition_to(1U, 0.4F));
    assert(graph.update(0.2F, pose));
    assert(graph.transitioning());
    const float pose_before_interrupt = pose.joints[0].translation.x;
    assert(graph.transition_to(2U, 0.4F));
    assert(graph.transitioning());
    assert(graph.update(0.001F, pose));
    const float interrupt_delta =
        std::abs(pose.joints[0].translation.x - pose_before_interrupt);
    assert(interrupt_delta < 0.1F);
    for (int frame = 0; frame < 8; ++frame)
        assert(graph.update(0.05F, pose));
    assert(!graph.transitioning());
    assert(graph.current_state() == 2U);
    assert(std::isfinite(pose.joints[0].translation.x));

    assert(!graph.transition_to(1U, std::numeric_limits<float>::quiet_NaN()));
    assert(graph.transition_to(1U, 0.0F));
    assert(graph.current_state() == 1U);
    assert(!graph.transitioning());
    graph.reset();
    assert(!graph.update(0.1F, pose));
    std::cout << "ANIMATION_GRAPH_TEST pass states=2 crossfade=0.4s interrupted_retarget=1 event_routing=1\\n";
}
