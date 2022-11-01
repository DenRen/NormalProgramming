#include <atomic>
#include <array>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

// #define ENABLE_YIELD
// #define ENABLE_PAUSE
#define ENABLE_PAUSE_MEMORY

/*
YIELD
{
    MEMORY_ORDER_SEQ
    {
        0:00.48     0:00.60
        0:00.47     0:00.51
        0:00.48     0:00.51
        0:00.48     0:00.49
        0:00.51     0:00.47
        0:00.48     0:00.49
        0:00.49     0:00.47
        0:00.48     0:00.56
        0:00.49     0:00.47
        0:00.50     0:00.55
    }

    !MEMORY_ORDER_SEQ
    {
        0:00.49     0:00.59
        0:00.50     0:00.56
        0:00.58     0:00.49
        0:00.50     0:00.50
        0:00.49     0:00.47
        0:00.53     0:00.54
        0:00.49     0:00.49
        0:00.66     0:00.47
        0:00.49     0:00.48
        0:00.50     0:00.52
    }
}

PAUSE_MEMORY
{
    MEMORY_ORDER_SEQ
    {
        0:00.10     0:00.21
        0:00.10     0:00.11
        0:00.09     0:00.12
        0:00.10     0:00.10
        0:00.10     0:00.10
        0:00.09     0:00.09
        0:00.11     0:00.09
        0:00.10     0:00.09
        0:00.10     0:00.10
        0:00.13     0:00.10
    }

    !MEMORY_ORDER_SEQ
    {
        0:00.07     0:00.10
        0:00.08     0:00.06
        0:00.06     0:00.08
        0:00.06     0:00.06
        0:00.06     0:00.05
        0:00.06     0:00.08
        0:00.06     0:00.06
        0:00.07     0:00.07
        0:00.07     0:00.10
        0:00.06     0:00.07
    }
}

*/
template <std::size_t NumTh>
class CLHLock
{
    constexpr static std::size_t N = NumTh + 1;
    std::atomic<std::size_t> m_i_tail{ N - 1 };
    std::array<std::atomic<bool>, N> m_rb;

    void active_sleep()
    {
        #ifdef ENABLE_YIELD
            std::this_thread::yield();
        #endif

        #ifdef ENABLE_MEMORY
            __asm volatile("pause" :::);
        #endif

        #ifdef ENABLE_PAUSE_MEMORY
            __asm volatile("pause" ::: "memory");
        #endif
    }

public:
    CLHLock()
    {
        for (std::size_t i = 0; i + 1 < N; ++i)
        {
            m_rb[i].store(true);
        }

        m_rb[m_rb.size() - 1].store(false);
    }

    std::size_t lock()
    {
        std::size_t i_next = m_i_tail.fetch_add(1, std::memory_order_release) % N;
        while(m_rb[i_next].load(std::memory_order_acquire))
        {
            active_sleep();
        }

        return i_next;
    }

    void unlock(size_t i_next)
    {
        m_rb[i_next].store(true, std::memory_order_release);
        m_rb[(i_next + 1) % N].store(false, std::memory_order_release);
    }
};

int main ()
{
    constexpr std::size_t num_threads = 8;
    constexpr std::size_t num_repeats = 100000;

    std::ios_base::sync_with_stdio(false);
    std::size_t ctr = 0;
    CLHLock<num_threads> clh;

    std::array <std::thread, num_threads> threads;
    for (std::size_t i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread([&clh, &ctr, i, num_repeats](){
            for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
            {
                const auto index = clh.lock();
                ++ctr;
                clh.unlock(index);
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    if (ctr != num_threads * num_repeats)
    {
        throw std::runtime_error("ctr invalid");
    }

    // std::cout << "Result:\n" << ctr << '\n';
}