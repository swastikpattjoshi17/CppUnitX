cmake_minimum_required(VERSION 3.16)
project(CppUnitX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ────────────────────────────────────────────────────────────
add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow
    -Wno-unused-parameter
    $<$<CONFIG:Debug>:-g -O0 -fsanitize=address,undefined>
    $<$<CONFIG:Release>:-O2>
)

add_link_options(
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# ── CppUnit-X: header-only interface library ──────────────────────────────────
add_library(cppunitx INTERFACE)
target_include_directories(cppunitx INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ── Self-test suite ───────────────────────────────────────────────────────────
file(GLOB_RECURSE SELF_TEST_SOURCES tests/self_tests/*.cpp)
add_executable(self_tests
    tests/self_tests/main.cpp
    ${SELF_TEST_SOURCES}
)
target_link_libraries(self_tests PRIVATE cppunitx)

# ── Demo project: lock-free queue ─────────────────────────────────────────────
file(GLOB_RECURSE DEMO_SOURCES tests/demo/*.cpp)
add_executable(demo_tests
    tests/demo/main.cpp
    ${DEMO_SOURCES}
)
target_link_libraries(demo_tests PRIVATE cppunitx)
# Lock-free queue uses atomics; link pthread on Linux
find_package(Threads REQUIRED)
target_link_libraries(demo_tests PRIVATE Threads::Threads)

# ── CTest integration ─────────────────────────────────────────────────────────
enable_testing()
add_test(NAME SelfTests  COMMAND self_tests  --xml=self_test_results.xml)
add_test(NAME DemoTests  COMMAND demo_tests  --xml=demo_results.xml)

# ── Install rules ─────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS cppunitx EXPORT CppUnitXTargets)
install(DIRECTORY include/cppunitx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT CppUnitXTargets
    FILE CppUnitXConfig.cmake
    NAMESPACE CppUnitX::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppUnitX
)
