#pragma once
#include <atomic>
#include <cstddef>
#include <stdexcept>

template<typename T, size_t N>
class SPSCRingBuffer {
    static_assert((N & (N-1)) == 0, "N must be power of 2");

public:
    SPSCRingBuffer() : head_(0), tail_(0) {}

    // Called from producer thread (matching engine)
    bool Push(const T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) & (N - 1);
        if (next == head_.load(std::memory_order_acquire))
            return false;  // buffer full
        buffer_[tail] = item;
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // Called from consumer thread (logger)
    bool Pop(T& out) {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire))
            return false;  // buffer empty
        out = buffer_[head];
        head_.store((head + 1) & (N - 1), std::memory_order_release);
        return true;
    }

    size_t Size() const {
        size_t tail = tail_.load(std::memory_order_acquire);
        size_t head = head_.load(std::memory_order_acquire);
        return (tail - head) & (N - 1);
    }

    bool Empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    // Each atomic on its own cache line — prevents false sharing
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
    T buffer_[N];
};
