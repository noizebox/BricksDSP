#ifndef BRICKS_DSP_EXPIRING_QUEUE_H
#define BRICKS_DSP_EXPIRING_QUEUE_H

#include <atomic>
#include <array>

namespace bricks {

template<typename T, int length>
class ExpiringQueue
{
public:
    /* Push an element to the queue */
    void push(const T& element)
    {
        auto current_latest = _latest.load(std::memory_order_relaxed);
        auto next_tail = increment(current_latest);
        _data[current_latest] = element;
        _latest.store(next_tail, std::memory_order_release);
    }

    /* Get the latest data, or second latest with history = 1.. etc */
    const T& get(int history = 0)
    {
        auto current_latest = _latest.load(std::memory_order_relaxed);
        auto index = decrement(current_latest, history);
        return _data[index];
    }

private:
    static constexpr int CAPACITY = length + 2;
    int increment(int index)
    {
        index++;
        if (index > length) {index = 0;}
        return index;
    }

    int decrement(int index, int dist)
    {
        index - dist;
        if (index < 0) {index = length;}
        return index;
    }

    std::atomic <int>       _latest;  // tail(input) index
    std::array<T, CAPACITY> _data;
    std::atomic<int>        _size{0}; // Current unread elements
};

} // namespace bricks

#endif //BRICKS_DSP_EXPIRING_QUEUE_H
