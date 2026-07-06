#include <cppunitx/cppunitx.h>
#include <unistd.h>
#include <csignal>

// ─────────────────────────────────────────────
// Crash Isolation Tests
//
// These tests intentionally crash or hang.
// The runner should catch them and report CRASH
// or TIMEOUT without killing the whole suite.
//
// NOTE: These are tagged "isolation" so you can
// run them separately:
//   ./self_tests --tag=isolation
//
// They only work correctly with fork() enabled
// (no --no-fork flag).
// ─────────────────────────────────────────────

TEST_TAGGED(CrashIsolation, SegFaultIsCaught, "isolation") {
    // Deliberate null pointer dereference
    // The framework should catch SIGSEGV and mark CRASH
    volatile int* bad_ptr = nullptr;
    (void)(*bad_ptr);  // SIGSEGV
}

TEST_TAGGED(CrashIsolation, AbortIsCaught, "isolation") {
    abort();  // SIGABRT
}

TEST_TAGGED(TimeoutIsolation, HangsAreCaught, "isolation") {
    // Infinite loop — should be killed by SIGALRM
    while (true) { ; }
}

TEST_TIMEOUT(TimeoutIsolation, CustomTimeout, 100) {
    // Fast test — should pass within 100ms
    int sum = 0;
    for (int i = 0; i < 1000; ++i) sum += i;
    ASSERT_EQ(sum, 499500);
}

// ─────────────────────────────────────────────
// Normal test runs after crash tests — this
// verifies crash isolation is working: if the
// suite dies on the segfault above, this test
// would never run.
// ─────────────────────────────────────────────

TEST(CrashIsolation, SuiteStillRunsAfterCrash) {
    // This test must pass — if it doesn't appear
    // in results, crash isolation is broken.
    ASSERT_TRUE(true);
}
