#ifndef MPSC_QUEUE_MUTEX_H
#define MPSC_QUEUE_MUTEX_H

#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>

template <typename T>
class MPSCQueueMutex
{
public:
    using value_type = T;

    explicit MPSCQueueMutex(std::mutex *mtx, std::size_t capacity);
    bool push(const T &value);
    bool pop(T &out);

private:
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
    std::vector<T> buffer_;
    std::size_t capacity_;
    std::size_t mod_;
    std::mutex *push_mutex_;
};

template <typename T>
MPSCQueueMutex<T>::MPSCQueueMutex(std::mutex *mtx, std::size_t capacity) : push_mutex_(mtx), head_(0), tail_(0)
{
    // Initialize with power of 2
    std::size_t cap = 1;
    while (cap < capacity)
    {
        cap <<= 1;
    }

    buffer_.resize(cap);
    capacity_ = cap;
    mod_ = cap - 1;
}

template <typename T>
bool MPSCQueueMutex<T>::push(const T &value)
{
    std::lock_guard<std::mutex> lock(*push_mutex_);

    std::size_t tail = tail_.load(std::memory_order_relaxed);
    std::size_t head = head_.load(std::memory_order_acquire);

    if ((tail - head) == capacity_)
    {
        return false;
    }

    buffer_[tail & mod_] = value;
    // release store on tail
    // ensures the value is visible to the consumer before the tail is updated
    tail_.store(tail + 1ull, std::memory_order_release);
    return true;
}

template <typename T>
bool MPSCQueueMutex<T>::pop(T &out)
{
    std::size_t tail = tail_.load(std::memory_order_acquire);
    std::size_t head = head_.load(std::memory_order_relaxed);

    // acquire load on tail
    // ensures we see the latest tail value updated by the producer
    if (head == tail)
    {
        return false;
    }

    out = buffer_[head & mod_];
    // release store on head
    // ensures the reads become visible before new head becomes visible
    head_.store(head + 1ull, std::memory_order_release);
    return true;
}

#endif // MPSC_QUEUE_MUTEX_H