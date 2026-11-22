#include "arut/core/version.hpp"

#include <iostream>

int main() {
    const auto current = arut::core::version();
    std::cout << arut::core::name() << ' '
              << current.major << '.'
              << current.minor << '.'
              << current.patch
              << " (headless bootstrap)\\n";
    return 0;
}
