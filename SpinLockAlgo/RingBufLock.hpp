#pragma once

#include <atomic>
#include <thread>

#include "atomic_lib.hpp"

template <std::size_t NumTh>
class RingBufLock
{
    constexpr static std::size_t N = NumTh + 1;
    std::atomic<std::size_t> m_i_tail{ N - 1 };
    std::array<std::atomic<bool>, N> m_rb;

public:
    RingBufLock()
    {
        for (std::size_t i = 0; i + 1 < N; ++i)
            m_rb[i].store(true);

        m_rb[m_rb.size() - 1].store(false);
    }

    std::size_t lock()
    {
        std::size_t i_next = m_i_tail.fetch_add(1, std::memory_order_release) % N;
        while(m_rb[i_next].load(std::memory_order_acquire))
            active_sleep<ACTIVE_SLEEP::PAUSE_MEMORY>();

        return i_next;
    }

    void unlock(size_t i_next)
    {
        m_rb[i_next].store(true, std::memory_order_release);
        m_rb[(i_next + 1) % N].store(false, std::memory_order_release);
    }
};
