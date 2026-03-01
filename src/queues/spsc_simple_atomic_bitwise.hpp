#ifndef SPSC_SIMPLE_ATOMIC_BITWISE_H
#define SPSC_SIMPLE_ATOMIC_BITWISE_H

#include <atomic>
#include <cstddef>
#include <vector>

template <typename T>
class SimpleSPSCQueue
{
public:
    using value_type = T;

    explicit SimpleSPSCQueue(std::size_t capacity);
    bool push(const T &value);
    bool pop(T &out);
    std::size_t capacity() const;

private:
    std::vector<T> buffer_;
    std::size_t capacity_;
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
};

template <typename T>
SimpleSPSCQueue<T>::SimpleSPSCQueue(std::size_t capacity) : head_(0), tail_(0) {
    // Initialize with power of 2
    std::size_t cap = 1;
    while (cap < capacity) {
        cap <<= 1;
    }

    buffer_.resize(cap);
    capacity_ = cap;
}

template <typename T>
bool SimpleSPSCQueue<T>::push(const T &value)
{
    std::size_t tail = tail_.load(std::memory_order_relaxed);
    std::size_t head = head_.load(std::memory_order_acquire);

    if ((tail - head) == capacity_)
    {
        return false;
    }

    buffer_[tail & (capacity_ - 1ull)] = value;
    // release store on tail
    // ensures the value is visible to the consumer before the tail is updated
    tail_.store(tail + 1ull, std::memory_order_release);
    return true;
}

template <typename T>
bool SimpleSPSCQueue<T>::pop(T &out)
{
    std::size_t tail = tail_.load(std::memory_order_acquire);
    std::size_t head = head_.load(std::memory_order_relaxed);

    // acquire load on tail
    // ensures we see the latest tail value updated by the producer
    if (head == tail)
    {
        return false;
    }

    out = buffer_[head & (capacity_ - 1ull)];
    // release store on head
    // ensures the reads become visible before new head becomes visible
    head_.store(head + 1ull, std::memory_order_release);
    return true;
}

template <typename T>
std::size_t SimpleSPSCQueue<T>::capacity() const
{
    return capacity_;
}

#endif // SPSC_SIMPLE_ATOMIC_BITWISE_H