#include "arut/core/version.hpp"

#include <cassert>
#include <string_view>

int main() {
    constexpr auto current = arut::core::version();
    static_assert(current.major == 0);
    static_assert(current.minor == 1);
    static_assert(current.patch == 0);
    assert(arut::core::name() == std::string_view{"ARUT-HARDT Engine"});
    return 0;
}
