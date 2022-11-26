#pragma once

#include <atomic>
#include <thread>

#include "atomic_lib.hpp"

#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_constructive_interference_size;
#else
    // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
    constexpr std::size_t hardware_constructive_interference_size = 64;
#endif

template <std::size_t NumTh>
class RingBufLock
{
    struct alignas(hardware_constructive_interference_size*0 + 1) // Optimization not work)))
    CacheBool : public std::atomic<bool> {};

    constexpr static std::size_t N = NumTh + 1;
    std::array<CacheBool, N> m_rb;
    std::atomic<std::size_t> m_i_tail{ N - 1 };

public:
    RingBufLock()
    {
        for (std::size_t i = 0; i + 1 < N; ++i)
            m_rb[i].store(true);

        m_rb[m_rb.size() - 1].store(false);
    }

    std::size_t lock()
    {
        std::size_t i_next = m_i_tail.fetch_add(1, std::memory_order_relaxed) % N;
        while(m_rb[i_next].load(std::memory_order_acquire))
            active_sleep<ACTIVE_SLEEP::PAUSE_MEMORY>();

        return i_next;
    }

    void unlock(size_t i_next)
    {
        m_rb[i_next].store(true, std::memory_order_relaxed);
        m_rb[(i_next + 1) % N].store(false, std::memory_order_release);
    }
};
