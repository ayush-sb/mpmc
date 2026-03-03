#ifndef MPSC_QUEUE_BASIC_H
#define MPSC_QUEUE_BASIC_H

#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>

template <typename T>
class MPSCQueueBasic
{
public:
    using value_type = T;

    explicit MPSCQueueBasic(std::size_t capacity);
    bool push(const T &value);
    bool pop(T &out);

private:
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
    std::vector<T> buffer_;
    std::size_t capacity_;
    std::size_t mod_;
};

template <typename T>
MPSCQueueBasic<T>::MPSCQueueBasic(std::size_t capacity) : head_(0), tail_(0)
{
}

template <typename T>
bool MPSCQueueBasic<T>::push(const T &value)
{
}

template <typename T>
bool MPSCQueueBasic<T>::pop(T &out)
{
}

#endif // MPSC_QUEUE_BASIC_H