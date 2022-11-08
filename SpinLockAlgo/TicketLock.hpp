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
        while (m_current.load(std::memory_order_relaxed) != index)
            active_sleep<ACTIVE_SLEEP::PAUSE_MEMORY>();
    }

    void unlock() noexcept
    {
        const auto next_index = m_current.load(std::memory_order_relaxed) + 1;
        m_current.store(next_index, std::memory_order_relaxed);
    }
};
