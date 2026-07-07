# CppUnit-X

[![CI](https://github.com/swastikpattjoshi17/CppUnitX/actions/workflows/ci.yml/badge.svg)](https://github.com/swastikpattjoshi17/CppUnitX/actions/workflows/ci.yml)

**A Lightweight, Zero-Dependency C++ Unit Testing Framework**

[![CI](https://github.com/swastikpattjoshi17/CppUnitX/actions/workflows/ci.yml/badge.svg)](https://github.com/swastikpattjoshi17/CppUnitX/actions)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-green)

---

## Why I Built This

Most SDET roles expect you to *use* testing tools. This project demonstrates that I understand how they work **internally** — macros, process isolation, signal handling, structured reporting, and CI integration in C++17.

---

## Features

| Feature | Description |
|---|---|
| `TEST()` macro | Automatic test registration at static-init time |
| 12+ `ASSERT_*` macros | Rich failure messages with file/line info |
| **Crash isolation** | Each test runs in a `fork()`-ed subprocess — a SIGSEGV doesn't kill your suite |
| **Timeout enforcement** | Per-test timeouts via `SIGALRM`; default 5 seconds |
| **Parameterized tests** | `PARAMETERIZED_TEST(Suite, Name, Type, val1, val2, ...)` |
| **Test filtering** | `--filter="Math.*"` or `--tag=slow` from CLI |
| **JUnit XML output** | `--xml=results.xml` — consumed by GitHub Actions, Jenkins |
| **JSON reports** | `--json=results.json` |
| Colored terminal output | Auto-detects TTY; disabled in CI pipes |
| `SKIP(msg)` | Skip a test with a reason |
| **Self-tests** | The framework tests itself with 40+ tests |
| **Demo project** | Tests a concurrent lock-free SPSC queue |

---

## Quick Start

```cpp
#include <cppunitx/cppunitx.h>

TEST(MathTests, Addition) {
    ASSERT_EQ(2 + 2, 4);
}

TEST(MathTests, FloatingPoint) {
    ASSERT_NEAR(3.14159, 3.14158, 0.0001);
}

TEST(Exceptions, ThrowsOnBadInput) {
    ASSERT_THROWS(throw std::invalid_argument("bad"), std::invalid_argument);
}

PARAMETERIZED_TEST(PrimeTests, IsPrime, int, 2, 3, 5, 7, 11) {
    ASSERT_TRUE(is_prime(param));
}

int main(int argc, char* argv[]) {
    return cppunitx::Runner::run(argc, argv);
}
```

**Output:**
```
══════════════════════════════════════
  CppUnit-X Test Runner
══════════════════════════════════════
  Running 4 test(s)

[ PASS ]  MathTests.Addition          (0ms)
[ PASS ]  MathTests.FloatingPoint     (0ms)
[ PASS ]  Exceptions.ThrowsOnBadInput (0ms)
[ PASS ]  PrimeTests.IsPrime/0        (0ms)
...

══════════════════════════════════════
  Results:  8 passed  |  0 failed
  Total time: 2ms
══════════════════════════════════════
```

---

## Build

```bash
git clone https://github.com/swastikpattjoshi17/CppUnitX
cd CppUnitX
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

**Run self-tests:**
```bash
./build/self_tests
```

**Run demo tests (lock-free queue):**
```bash
./build/demo_tests
```

**With filtering and XML output:**
```bash
./build/self_tests --filter="LockFreeQueue.*" --xml=results.xml
./build/demo_tests --tag=slow --verbose
./build/self_tests --list   # list all registered tests
```

---

## Architecture

```
CppUnit-X/
├── include/cppunitx/
│   ├── cppunitx.h      # Single-include umbrella header
│   ├── registry.h      # TestCase + Registry singleton + Registrar RAII
│   ├── macros.h        # TEST(), TEST_TAGGED(), TEST_TIMEOUT(), PARAMETERIZED_TEST()
│   ├── assertions.h    # All ASSERT_* macros + AssertionFailure exception
│   ├── runner.h        # Crash isolation (fork), timeout (SIGALRM), result collection
│   └── reporter.h      # Terminal output, JUnit XML, JSON
│
├── tests/
│   ├── self_tests/     # Framework tests itself
│   │   ├── test_assertions.cpp
│   │   └── test_isolation.cpp   ← intentional crashes/hangs
│   └── demo/
│       └── test_lockfree_queue.cpp
│
├── examples/
│   └── lockfree_queue/
│       └── lockfree_queue.h    # SPSC wait-free circular buffer
│
└── .github/workflows/ci.yml    # GitHub Actions CI
```

### How Test Registration Works

```
                  static init
  TEST(Suite, Name) { ... }
        │
        ▼
  Registrar ctor  →  Registry::add(TestCase)   (at program startup)
        │
        ▼
  Runner::run()   →  Registry::get_tests()
        │
        ├── for each test:
        │       fork()
        │       ├── child: alarm(timeout), run test, write result to pipe, _exit(0)
        │       └── parent: waitpid(), read pipe, detect SIGSEGV/SIGALRM
        │
        └── Reporter: terminal + XML + JSON
```

### Crash Isolation Flow

```
Parent Process                    Child Process
─────────────────                 ─────────────────
fork() ──────────────────────────► [new child]
waitpid(child, WNOHANG)           alarm(timeout_ms / 1000)
                                  run test fn()
                                    ├── PASS: write result to pipe
                                    ├── FAIL: write failure msg to pipe
                                    └── CRASH: SIGSEGV → child dies
                                  _exit(0)
read pipe ◄───────────────────────
WIFSIGNALED? → Status::CRASH
WTERMSIG==SIGALRM? → Status::TIMEOUT
```

---

## Assertion Reference

| Macro | Checks |
|---|---|
| `ASSERT_TRUE(expr)` | expr is truthy |
| `ASSERT_FALSE(expr)` | expr is falsy |
| `ASSERT_EQ(a, b)` | a == b |
| `ASSERT_NE(a, b)` | a != b |
| `ASSERT_LT(a, b)` | a < b |
| `ASSERT_LE(a, b)` | a <= b |
| `ASSERT_GT(a, b)` | a > b |
| `ASSERT_GE(a, b)` | a >= b |
| `ASSERT_NEAR(a, b, eps)` | \|a - b\| <= eps |
| `ASSERT_NULL(ptr)` | ptr == nullptr |
| `ASSERT_NOT_NULL(ptr)` | ptr != nullptr |
| `ASSERT_THROWS(expr, Type)` | expr throws Type |
| `ASSERT_NO_THROW(expr)` | expr doesn't throw |
| `ASSERT_STR_EQ(a, b)` | string equality |
| `ASSERT_CONTAINS(str, sub)` | str contains sub |
| `SKIP(msg)` | Skip this test |

---

## CLI Reference

```
Usage: ./runner [options]

  --filter=<pattern>    Run only tests matching pattern (supports "Suite.*")
  --tag=<tag>           Run only tests with matching tag
  --xml=<path>          Write JUnit XML report
  --json=<path>         Write JSON report
  --list                List all registered tests and exit
  --no-fork             Disable crash isolation (useful for debuggers)
  --verbose, -v         Print more detail
```

---

## Demo: Lock-Free SPSC Queue

The demo project tests a **wait-free, single-producer single-consumer queue** — a data structure commonly used in low-latency trading systems.

Key properties tested:
- FIFO ordering correctness
- Boundary conditions (full/empty)
- Wrap-around on circular buffer
- Concurrent correctness: producer pushes 100,000 items; consumer reads them; sum must match

---

## What This Project Demonstrates

- **Deep C++ knowledge**: SFINAE-free template metaprogramming, variadic macros, `__attribute__`, `alignas`, atomic orderings
- **Systems programming**: `fork()`, `pipe()`, `waitpid()`, `SIGALRM`, signal handlers
- **SDET thinking**: testing the tester, crash domains, flakiness prevention, CI integration
- **Software design**: singleton registry, RAII registration, interface segregation

---

## License

MIT
