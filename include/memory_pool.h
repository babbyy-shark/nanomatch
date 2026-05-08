#pragma once
#include <cstdlib>
#include <vector>
#include <stdexcept>

template<typename T, size_t N>
class MemoryPool {
public:
    MemoryPool() {
        // Allocate N objects in one contiguous aligned block
        storage_ = static_cast<T*>(
            aligned_alloc(alignof(T), sizeof(T) * N));
        if (!storage_) throw std::bad_alloc();

        // Pre-populate free list with all slots
        free_list_.reserve(N);
        for (size_t i = 0; i < N; ++i)
            free_list_.push_back(&storage_[i]);
    }

    ~MemoryPool() { free(storage_); }

    // No copy or move — pool owns its memory
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    T* Acquire() {
        if (free_list_.empty()) throw std::bad_alloc();
        T* slot = free_list_.back();
        free_list_.pop_back();
        return new(slot) T();   // placement new — no heap allocation
    }

    void Return(T* obj) {
        obj->~T();              // explicit destructor call
        free_list_.push_back(obj);
    }

    size_t Available() const { return free_list_.size(); }
    size_t Capacity()  const { return N; }

private:
    T* storage_;
    std::vector<T*> free_list_;
};
