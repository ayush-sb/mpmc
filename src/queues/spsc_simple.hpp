#ifndef SPSC_SIMPLE_H
#define SPSC_SIMPLE_H

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
    bool empty() const;
    bool full() const;
    std::size_t size() const;
    std::size_t capacity() const;

private:
    std::vector<T> buffer_;
    std::size_t capacity_;
    std::size_t head_;
    std::size_t tail_;
    std::size_t size_;
};

template <typename T>
SimpleSPSCQueue<T>::SimpleSPSCQueue(std::size_t capacity) : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0) {}

template <typename T>
bool SimpleSPSCQueue<T>::push(const T &value)
{
    if (size_ == capacity_)
    {
        return false;
    }

    buffer_[tail_] = value;
    tail_ = (tail_ + 1);
    ++size_;
    return true;
}

template <typename T>
bool SimpleSPSCQueue<T>::pop(T &out)
{
    if (size_ == 0)
    {
        return false;
    }

    out = buffer_[head_];
    head_ = (head_ + 1) % capacity_;
    --size_;
    return true;
}

template <typename T>
bool SimpleSPSCQueue<T>::empty() const
{
    return size_ == 0;
}

template <typename T>
bool SimpleSPSCQueue<T>::full() const
{
    return size_ == capacity_;
}

template <typename T>
std::size_t SimpleSPSCQueue<T>::size() const
{
    return size_;
}

template <typename T>
std::size_t SimpleSPSCQueue<T>::capacity() const
{
    return capacity_;
}

#endif // SPSC_SIMPLE_H