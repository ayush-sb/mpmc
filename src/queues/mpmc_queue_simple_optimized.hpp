#ifndef MPMC_QUEUE_SIMPLE_OPTIMIZED_H
#define MPMC_QUEUE_SIMPLE_OPTIMIZED_H

#include <atomic>
#include <cstddef>
#include <vector>

struct alignas(64) CacheLinePaddedAtomicSizeT {
    std::atomic<std::size_t> value;
};

template <typename T>
struct alignas(64) CacheLinePaddedT {
    T value;
};

template <typename T>
class SimpleMPMCQueueOptimized
{
public:
    explicit SimpleMPMCQueueOptimized(std::size_t capacity);
    bool push(const T &value);
    bool pop(T &out);
private:
    std::vector<CacheLinePaddedAtomicSizeT> seq_;
    std::vector<CacheLinePaddedT<T>> buffer_;

    std::size_t capacity_;
    std::size_t power_;
    std::size_t index_mask_;

    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
};

template <typename T>
SimpleMPMCQueueOptimized<T>::SimpleMPMCQueueOptimized(std::size_t capacity) : head_(0), tail_(0)
{
    // Initialize with power of 2
    std::size_t power = 0;
    std::size_t cap = 1;
    while (cap < capacity)
    {
        cap <<= 1;
        power_++;
    }

    buffer_ = std::vector<CacheLinePaddedT<T>>(cap);
    seq_ = std::vector<CacheLinePaddedAtomicSizeT>(cap);

    for (std::size_t i = 0; i < cap; ++i)
    {
        seq_[i].value.store(i, std::memory_order_relaxed);
    }

    capacity_ = cap;
    index_mask_ = cap - 1ull;
}

template <typename T>
bool SimpleMPMCQueueOptimized<T>::push(const T &value)
{
    // reserve slot
    std::size_t tail = tail_.fetch_add(1ull, std::memory_order_relaxed);

    // wait until slot is ready for producer
    while (seq_[tail & index_mask_].value.load(std::memory_order_acquire) != tail) {}

    // store value in slot
    buffer_[tail & index_mask_].value = value;

    // update slot
    seq_[tail & index_mask_].value.store(tail + 1ull, std::memory_order_release);

    return true;
}

template <typename T>
bool SimpleMPMCQueueOptimized<T>::pop(T &out)
{
    // reserve head
    std::size_t head = head_.fetch_add(1ull, std::memory_order_relaxed);

    // wait until slot is ready for consumer
    while (seq_[head & index_mask_].value.load(std::memory_order_acquire) != (head + 1ull)) {}

    // read value from slot
    out = buffer_[head & index_mask_].value;

    // mark slot as consumed
    seq_[head & index_mask_].value.store(head + capacity_, std::memory_order_release);

    return true;
}

#endif // MPMC_QUEUE_SIMPLE_OPTIMIZED_H