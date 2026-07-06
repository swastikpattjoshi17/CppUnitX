#pragma once

/**
 * CppUnit-X: A Lightweight C++ Unit Testing Framework
 * Built from scratch - no external dependencies
 *
 * Usage:
 *   #include <cppunitx/cppunitx.h>
 *
 *   TEST(MyTestSuite, BasicAddition) {
 *       ASSERT_EQ(2 + 2, 4);
 *   }
 *
 *   int main(int argc, char* argv[]) {
 *       return cppunitx::Runner::run(argc, argv);
 *   }
 */

#include "registry.h"
#include "assertions.h"
#include "macros.h"
#include "runner.h"
#include "reporter.h"
