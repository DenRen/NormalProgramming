#pragma once

#include <thread>

enum class ACTIVE_SLEEP
{
    PAUSE,
    PAUSE_MEMORY,
    YIELD
};

template <ACTIVE_SLEEP AS = ACTIVE_SLEEP::PAUSE>
inline void active_sleep()
{
    __asm volatile("pause" :::);
}

template <>
inline void active_sleep<ACTIVE_SLEEP::PAUSE_MEMORY>()
{
    __asm volatile("pause" ::: "memory");
}

template <>
inline void active_sleep<ACTIVE_SLEEP::YIELD>()
{
    std::this_thread::yield();
}
