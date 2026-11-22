cmake_minimum_required(VERSION 3.25)

project(ARUT_HARDT_ENGINE
    VERSION 0.1.0
    DESCRIPTION "Independent Android-first open-world game engine"
    LANGUAGES CXX
)

option(ARUT_BUILD_TESTS "Build ARUT-HARDT tests" ON)
option(ARUT_BUILD_RUNTIME "Build the headless bootstrap executable" ON)

add_library(arut_core STATIC engine/core/src/version.cpp)
target_include_directories(arut_core PUBLIC engine/core/include)
target_compile_features(arut_core PUBLIC cxx_std_20)
set_target_properties(arut_core PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(arut_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(arut_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ARUT_BUILD_RUNTIME)
    add_executable(arut_runtime engine/runtime/src/main.cpp)
    target_link_libraries(arut_runtime PRIVATE arut_core)
    target_compile_features(arut_runtime PRIVATE cxx_std_20)
    set_target_properties(arut_runtime PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(ARUT_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_executable(arut_core_tests tests/core_tests.cpp)
    target_link_libraries(arut_core_tests PRIVATE arut_core)
    target_compile_features(arut_core_tests PRIVATE cxx_std_20)
    add_test(NAME arut_core_tests COMMAND arut_core_tests)
endif()
