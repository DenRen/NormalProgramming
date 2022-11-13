#pragma once

#include <atomic>
#include <thread>

#include "atomic_lib.hpp"

class TTAS
{
    std::atomic<int> m_flag{ 0 };

public:
    void lock() noexcept
    {
        int expected = 0;
        while(!m_flag.compare_exchange_weak(expected, 1, std::memory_order_acquire,
                                                         std::memory_order_relaxed))
        {
            expected = 0;

            // Effective spin on RO
            while(m_flag.load(std::memory_order_relaxed))
                active_sleep<ACTIVE_SLEEP::YIELD>();
        }
    }

    void unlock() noexcept
    {
        m_flag.store(0, std::memory_order_release);
    }
};
