#pragma once

#include <atomic>
#include <thread>

#include "atomic_lib.hpp"

class TicketLock
{
    std::atomic<int> m_current{ 0 }, m_last{ 0 };

public:
    void lock() noexcept
    {
        const auto index = m_last.fetch_add(1, std::memory_order_relaxed);
        while (m_current.load(std::memory_order_acquire) != index)
            active_sleep<ACTIVE_SLEEP::PAUSE_MEMORY>();
    }

    void unlock() noexcept
    {
        // I use relaxed, because lock() method already was invoked =>
        // if I already get the my index, I don't get other index. (relaxed guaanted, that
        // I not see an “earlier” value for m_current variable)
        const auto next_index = m_current.load(std::memory_order_relaxed) + 1;
        m_current.store(next_index, std::memory_order_release);
    }
};

// -----------------------
// Q: *

// TODO:
// 1) Stack (lock-free) or/and Queue (lock-free)
// 2) Skip list (l-f) and Hash-Table Split orderid list (Herlich)
// 3) C-P problems. TLA+ (Sempha -> Cond var) and check correction
// 4) Shared systems (FLP Theorem)