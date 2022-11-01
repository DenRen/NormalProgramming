#include <atomic>
#include <array>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#define ENABLE_YIELD

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
        #else
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
        std::size_t i_next = m_i_tail.fetch_add(1, std::memory_order_relaxed) % N;
        while(m_rb[i_next].load())
        {
            active_sleep();
        }

        return i_next;
    }

    void unlock(size_t i_next)
    {
        m_rb[i_next].store(true);
        m_rb[(i_next + 1) % N].store(false);
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

    std::cout << "Result:\n" << ctr << '\n';
}