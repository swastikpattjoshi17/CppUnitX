#pragma once

/**
 * LockFreeQueue<T, Capacity>
 *
 * A Single-Producer Single-Consumer (SPSC) lock-free queue
 * implemented using a circular buffer and atomic indices.
 *
 * Properties:
 *   - Wait-free for both producer and consumer
 *   - No dynamic allocation after construction
 *   - Capacity must be a power of 2 (enforced at compile time)
 *   - T must be trivially copyable
 *
 * This is used as the demo data structure that CppUnit-X tests.
 */

#include <atomic>
#include <array>
#include <optional>
#include <type_traits>
#include <stdexcept>

template<typename T, size_t Capacity>
class LockFreeQueue {
    static_assert(std::is_trivially_copyable_v<T>,
                  "LockFreeQueue<T>: T must be trivially copyable");
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                  "LockFreeQueue: Capacity must be a power of 2");

public:
    LockFreeQueue() : head_(0), tail_(0) {}

    // Non-copyable, non-movable
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    // ── push: called by the producer thread ─────────────────────────────────
    // Returns false if the queue is full.
    bool push(const T& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next = (head + 1) & MASK;

        if (next == tail_.load(std::memory_order_acquire))
            return false;  // queue full

        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // ── pop: called by the consumer thread ──────────────────────────────────
    // Returns nullopt if the queue is empty.
    std::optional<T> pop() noexcept {
        const size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire))
            return std::nullopt;  // queue empty

        T item = buffer_[tail];
        tail_.store((tail + 1) & MASK, std::memory_order_release);
        return item;
    }

    // ── Queries ──────────────────────────────────────────────────────────────
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    bool full() const noexcept {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return ((head + 1) & MASK) == tail;
    }

    // Approximate size — only safe to call from a single thread
    size_t size_approx() const noexcept {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail + Capacity) & MASK;
    }

    static constexpr size_t capacity() { return Capacity - 1; } // usable slots

private:
    static constexpr size_t MASK = Capacity - 1;

    // Cache-line padding to prevent false sharing between producer/consumer
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
    std::array<T, Capacity>         buffer_;
};
