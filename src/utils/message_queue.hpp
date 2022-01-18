/**
 * Copyright Claus Beckenbauer 2004 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <semaphore>
#include <tuple>

namespace utils
{

template <typename T, size_t depth>
class message_queue
{
   public:
    using msg     = T;
    using msg_ptr = std::shared_ptr<msg>;  // use a shared pointer for a zero-copy dequeu mechanism
    using fifo_t  = std::queue<msg_ptr, std::list<msg_ptr>>;

    struct watermark
    {
        using boundary = std::pair<int, std::function<void()>>;
        bool     raised{false};
        boundary low{0, nullptr};
        boundary high{depth, nullptr};
    };

    message_queue() = default;

    message_queue(const message_queue& other) = delete;
    message_queue& operator=(const message_queue& rhs) = delete;

    message_queue(message_queue&& rhs) noexcept = delete;
    message_queue& operator=(message_queue&& rhs) noexcept = delete;

    ~message_queue() = default;

    /** message queue construction with watermark observation
     */
    message_queue(watermark::boundary low, watermark::boundary high);

    /** enqueue the payload and signal the consumer
     * @return false if there is no space left in the queue
     */
    [[nodiscard]] auto enqueue(msg_ptr&& payload) -> bool;

    /** dequeue and signal the producer available slots
     * return nullopt if there is nothing more to dequeue
     */
    auto dequeue() -> msg_ptr;

    /** abort and return to the caller from ::dequeue() immediately
     */
    auto abort_queue() -> void;

   private:
    std::counting_semaphore<depth> occupied_slots{0};
    std::counting_semaphore<depth> available_slots{depth};
    std::mutex                     operation;
    fifo_t                         fifo;
    watermark                      wm;
};

template <typename T, size_t depth>
inline message_queue<T, depth>::message_queue(watermark::boundary low, watermark::boundary high) : wm{false, low, high}
{
    const auto& [low_mark, low_hook]   = low;
    const auto& [high_mark, high_hook] = high;
    if (low_mark >= high_mark)
    {
        throw std::invalid_argument("low watermark is greater or equal than high mark");
    }
    if (high_mark > depth)
    {
        throw std::invalid_argument("high watermark couldn't be greater than q's depth");
    }
}

template <typename T, size_t depth>
inline auto message_queue<T, depth>::enqueue(msg_ptr&& payload) -> bool
{
    if (!available_slots.try_acquire())
    {
        return false;
    }
    {
        auto guard = std::lock_guard<std::mutex>{operation};
        fifo.emplace(std::forward<msg_ptr>(payload));

        // Check if high watermark is observerd and reached.
        // Invoke the notification hook for the high mark and start observing the lower mark.
        if (const auto& [mark, hook] = wm.high; hook && fifo.size() >= mark)
        {
            wm.raised = true;
            hook();
        }
        else if (const auto& [mark, hook] = wm.low; hook && wm.raised && fifo.size() - 1 <= mark)
        {
            wm.raised = false;
            hook();
        }
    }
    occupied_slots.release();
    return true;
}

template <typename T, size_t depth>
inline auto message_queue<T, depth>::dequeue() -> msg_ptr
{
    occupied_slots.acquire();  // blocking, till message arrives
    
    auto msg = fifo.front();
    if (!msg)
    {  // noting in the queue, must be the stop sequence
        return msg;
    }
    else
    {
        auto g = std::lock_guard<std::mutex>{operation};
        fifo.pop();
    }

    available_slots.release();
    return msg;
}

template <typename T, size_t depth>
inline auto message_queue<T, depth>::abort_queue() -> void
{
    occupied_slots.release();
}

}  // namespace utils
