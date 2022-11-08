#pragma once

#include <atomic>
#include <thread>

#include "atomic_lib.hpp"

class TAS
{
    std::atomic<int> m_flag{ 0 };

public:
    void lock() noexcept
    {
        int expected = 0;
        while(!m_flag.compare_exchange_weak(expected, 1, std::memory_order_relaxed))
        {
            active_sleep<ACTIVE_SLEEP::YIELD>();
            expected = 0;
        }
    }

    void unlock() noexcept
    {
        m_flag.store(0, std::memory_order_relaxed);
    }
};

// todo: ticket_lock
// todo: single a.out -> all test with all num_threads