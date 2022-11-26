#include <iostream>
#include <chrono>
#include <functional>

#include "RingBufLock.hpp"
#include "TAS.hpp"
#include "TTAS.hpp"
#include "TicketLock.hpp"

template <std::size_t num_threads>
void TrueOrderLockPerfTest(std::size_t num_repeats)
{
    auto num_repeats_per_thread = num_repeats / num_threads;

    std::size_t ctr = 0;
    RingBufLock<num_threads> tol;

    std::array <std::thread, num_threads> threads;
    for (auto& th : threads)
    {
        th = std::thread([&tol, &ctr, num_repeats](){
            for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
            {
                const auto index = tol.lock();
                ++ctr;
                tol.unlock(index);
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    if (ctr != num_threads * num_repeats)
        throw std::runtime_error("ctr invalid");
}

template <typename SpinLock, std::size_t num_threads>
void StdPerfTest(std::size_t num_repeats)
{ 
    auto num_repeats_per_thread = num_repeats / num_threads;

    std::size_t ctr = 0;
    SpinLock sl;

    std::array <std::thread, num_threads> threads;
    for (auto& th : threads)
    {
        th = std::thread([&sl, &ctr, num_repeats](){
            for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
            {
                sl.lock();
                ++ctr;
                sl.unlock();
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    if (ctr != num_threads * num_repeats)
        throw std::runtime_error("ctr invalid");
}

enum class SPIN_LOCK
{
    RB_LOCK,
    TAS,
    TTAS,
    TICKET_LOCK
};

template<SPIN_LOCK spin_lock, std::size_t num_threads>
void RunPerfTest(std::size_t num_repeats)
{
    switch(spin_lock)
    {
        case SPIN_LOCK::RB_LOCK:
            TrueOrderLockPerfTest<num_threads>(num_repeats);
            break;
        case SPIN_LOCK::TAS:
            StdPerfTest<TAS, num_threads>(num_repeats);
            break;
        case SPIN_LOCK::TTAS:
            StdPerfTest<TTAS, num_threads>(num_repeats);
            break;
        case SPIN_LOCK::TICKET_LOCK:
            StdPerfTest<TicketLock, num_threads>(num_repeats);
            break;
        default:
            throw std::runtime_error("Unknown spin lock type");
    }
}

template <typename Func, typename... Args>
auto CalcTimeExecution(Func&& func, Args&&... args)
{
    const auto time_begin = std::chrono::high_resolution_clock::now();
    std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    const auto time_end = std::chrono::high_resolution_clock::now();
    
    const auto dtime = time_end - time_begin;
    const auto dtime_ms = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();
    return dtime_ms;
}

template<SPIN_LOCK spin_lock, std::size_t num_threads>
void PrintTimePerfTest(std::size_t counter_end, std::size_t num_repeats, std::size_t num_skip)
{
    std::cout << num_threads;
    for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
    {
        auto dt_ms = CalcTimeExecution(RunPerfTest<spin_lock, num_threads>, counter_end);
        if (i_repeat < num_skip)
            continue;
        
        std::cout << ' ' << dt_ms;
    }
    std::cout << std::endl;
}

int main()
{
    std::ios_base::sync_with_stdio(false);

    constexpr auto spin_lock = SPIN_LOCK::RB_LOCK;
    constexpr std::size_t counter_end  = 1'000'000;
    constexpr std::size_t num_repeats = 13;
    constexpr std::size_t num_skip = 3;
    static_assert(num_skip < num_repeats);

    std::cout << num_repeats - num_skip << '\n';

    // PrintTimePerfTest<spin_lock, 1>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 2>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 3>(counter_end, num_repeats, num_skip);
    PrintTimePerfTest<spin_lock, 4>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 5>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 6>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 7>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 8>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 9>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 10>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 11>(counter_end, num_repeats, num_skip);
    // PrintTimePerfTest<spin_lock, 12>(counter_end, num_repeats, num_skip);
}