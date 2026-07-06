#include <cppunitx/cppunitx.h>
#include "../../examples/lockfree_queue/lockfree_queue.h"

#include <thread>
#include <vector>
#include <numeric>
#include <atomic>

// ─────────────────────────────────────────────
// Basic functional tests
// ─────────────────────────────────────────────

TEST(LockFreeQueue, InitiallyEmpty) {
    LockFreeQueue<int, 8> q;
    ASSERT_TRUE(q.empty());
    ASSERT_FALSE(q.full());
    ASSERT_EQ(q.size_approx(), 0u);
}

TEST(LockFreeQueue, PushAndPop) {
    LockFreeQueue<int, 8> q;
    ASSERT_TRUE(q.push(42));
    ASSERT_FALSE(q.empty());

    auto val = q.pop();
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value(), 42);
    ASSERT_TRUE(q.empty());
}

TEST(LockFreeQueue, FIFOOrdering) {
    LockFreeQueue<int, 16> q;
    for (int i = 0; i < 5; ++i)
        ASSERT_TRUE(q.push(i));

    for (int i = 0; i < 5; ++i) {
        auto v = q.pop();
        ASSERT_TRUE(v.has_value());
        ASSERT_EQ(v.value(), i);
    }
    ASSERT_TRUE(q.empty());
}

TEST(LockFreeQueue, PopFromEmptyReturnsNullopt) {
    LockFreeQueue<int, 8> q;
    auto result = q.pop();
    ASSERT_FALSE(result.has_value());
}

TEST(LockFreeQueue, FillsToCapacity) {
    LockFreeQueue<int, 8> q;   // holds 7 items (capacity - 1)
    int i = 0;
    while (q.push(i)) ++i;
    ASSERT_TRUE(q.full());
    ASSERT_EQ(static_cast<size_t>(i), q.capacity());
}

TEST(LockFreeQueue, PushToFullFails) {
    LockFreeQueue<int, 4> q;  // holds 3 items
    ASSERT_TRUE(q.push(1));
    ASSERT_TRUE(q.push(2));
    ASSERT_TRUE(q.push(3));
    ASSERT_TRUE(q.full());
    ASSERT_FALSE(q.push(4));  // must fail
}

TEST(LockFreeQueue, RoundTrip) {
    LockFreeQueue<int, 16> q;
    // Push half, pop half, push again — exercises wrap-around
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 7; ++i)
            ASSERT_TRUE(q.push(i * 10));
        for (int i = 0; i < 7; ++i) {
            auto v = q.pop();
            ASSERT_TRUE(v.has_value());
            ASSERT_EQ(v.value(), i * 10);
        }
    }
    ASSERT_TRUE(q.empty());
}

// ─────────────────────────────────────────────
// Works with non-int trivially-copyable types
// ─────────────────────────────────────────────

struct Point { float x, y; };

TEST(LockFreeQueue, StructType) {
    LockFreeQueue<Point, 8> q;
    ASSERT_TRUE(q.push({1.5f, 2.5f}));
    auto v = q.pop();
    ASSERT_TRUE(v.has_value());
    ASSERT_NEAR(v->x, 1.5f, 1e-6f);
    ASSERT_NEAR(v->y, 2.5f, 1e-6f);
}

// ─────────────────────────────────────────────
// Concurrent correctness test (SPSC)
//
// Producer pushes N items; consumer pops them.
// Final sum must match expected sum.
// ─────────────────────────────────────────────

TEST_TIMEOUT(LockFreeQueue, ConcurrentSPSC, 2000) {
    static constexpr int N = 100'000;
    LockFreeQueue<int, 1024> q;

    std::atomic<long long> consumer_sum{0};
    std::atomic<bool>      producer_done{false};

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i) {
            while (!q.push(i)) { /* spin — wait for space */ }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        int received = 0;
        long long sum = 0;
        while (received < N) {
            auto v = q.pop();
            if (v) {
                sum += *v;
                ++received;
            }
        }
        consumer_sum.store(sum, std::memory_order_release);
    });

    producer.join();
    consumer.join();

    long long expected = static_cast<long long>(N) * (N - 1) / 2;
    ASSERT_EQ(consumer_sum.load(), expected);
}

// ─────────────────────────────────────────────
// Parameterized: test multiple capacities
// ─────────────────────────────────────────────

// We can't easily parameterize template args, but we can
// parameterize the number of items pushed/popped.
PARAMETERIZED_TEST(LockFreeQueue, PushPopN, int, 1, 7, 15, 31) {
    LockFreeQueue<int, 64> q;
    for (int i = 0; i < param; ++i)
        ASSERT_TRUE(q.push(i));
    ASSERT_EQ(q.size_approx(), static_cast<size_t>(param));
    for (int i = 0; i < param; ++i) {
        auto v = q.pop();
        ASSERT_TRUE(v.has_value());
        ASSERT_EQ(v.value(), i);
    }
    ASSERT_TRUE(q.empty());
}
