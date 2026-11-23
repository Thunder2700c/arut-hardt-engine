#include "arut/core/test_lab_profile.hpp"

#include <cassert>
#include <cstdint>

int main() {
    using arut::core::TestLabProfile;
    using namespace arut::core;

    assert(valid_test_lab_profile(0));
    assert(valid_test_lab_profile(1));
    assert(valid_test_lab_profile(2));
    assert(!valid_test_lab_profile(-1));
    assert(!valid_test_lab_profile(3));

    assert(test_lab_includes_stress(TestLabProfile::EcsStress));
    assert(!test_lab_includes_humanoid(TestLabProfile::EcsStress));
    assert(!test_lab_includes_stress(TestLabProfile::Humanoid));
    assert(test_lab_includes_humanoid(TestLabProfile::Humanoid));
    assert(test_lab_includes_stress(TestLabProfile::Combined));
    assert(test_lab_includes_humanoid(TestLabProfile::Combined));

    assert(test_lab_profile_name(TestLabProfile::EcsStress) == "ecs_stress");
    assert(test_lab_profile_name(TestLabProfile::Humanoid) == "humanoid");
    assert(test_lab_profile_name(TestLabProfile::Combined) == "combined");
    return 0;
}
