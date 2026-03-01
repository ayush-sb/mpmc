
#ifndef SPSC_SIMPLE_H
#define SPSC_SIMPLE_H

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
    std::atomic<std::size_t> head_;
    std::atomic<std::size_t> tail_;
};

template <typename T>
SimpleSPSCQueue<T>::SimpleSPSCQueue(std::size_t capacity) : buffer_(capacity), capacity_(capacity), head_(0), tail_(0) {}

template <typename T>
bool SimpleSPSCQueue<T>::push(const T &value)
{
    std::size_t tail = tail_.load(std::memory_order_relaxed);
    std::size_t head = head_.load(std::memory_order_relaxed);

    if ((tail - head) == capacity_)
    {
        return false;
    }

    buffer_[tail % capacity_] = value;
    // release store on tail
    // ensures the value is visible to the consumer before the tail is updated
    tail_.store(tail + 1, std::memory_order_release);
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

    out = buffer_[head % capacity_];
    // release store on head
    // ensures the reads become visible before new head becomes visible
    head_.store(head + 1, std::memory_order_release);
    return true;
}

template <typename T>
std::size_t SimpleSPSCQueue<T>::capacity() const
{
    return capacity_;
}

#endif // SPSC_SIMPLE_H