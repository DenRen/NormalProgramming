#include <iostream>
#include <vector>
#include <algorithm>
#include <barrier>

#include "HazardPointer.hpp"
#include "SplitOrderedList.hpp"
#include "List.hpp"

void TestListNative()
{
    List<int> list(16);
    std::thread th {
        [&list]() {
            list.Insert(1);
            list.Insert(1);
            list.Insert(2);
            list.Insert(7);
            list.Insert(-1);

            list.Dump();
            std::cout << std::endl;

            list.Erase(1);
            list.Erase(1);
            list.Erase(1);
            list.Insert(1);
            // list.Erase(1);
            list.Dump();
            std::cout << std::endl;

            list.Erase(-1);
            list.Erase(2);
            list.Erase(7);
            list.Erase(1);
            list.Dump();
            std::cout << std::endl;

            list.Erase(1);
            list.Dump();
            std::cout << std::endl;
        }
    };

    th.join();
}

void TestMultiThredinfList(int num_threads)
{
    #ifdef ENABLE_FUCKING_LEAK
    std::ios_base::sync_with_stdio(false);
    #endif

    const int num_repeats = 30'00;
    const int num_erase = 13'00;

    List<int> list{(unsigned)num_threads};

    std::vector<std::thread> threads;
    for (int i_th = 0; i_th < num_threads; ++i_th)
    {
        threads.emplace_back([&list, num_repeats](){
            for (int i = 0; i < num_repeats; ++i)
            {
                list.Insert(10 * i);
                // std::cout << std::this_thread::get_id() << " inserted " << 10 * i << '\n';
            }
        });
    }

    for (auto& th : threads)
        th.join();
    std::cout << std::endl;
    std::thread{[&list, num_repeats](){
        for (int i = 0; i < num_repeats; ++i)
        {
            if (!list.Find(10 * i))
            {
                std::cout << "Test Insert\tfailed!\n";
                return;
            }
            // std::cout << std::this_thread::get_id() << " finded " << 10 * i << '\n';
        }
        std::cout << "Test Insert\tpassed!\n";
    }}.join();

    std::cout << std::endl;
    for (auto& th : threads)
    {
        th = std::thread{[&list, num_repeats](){
            // const auto id = std::this_thread::get_id();
            for (int i = 0; i < num_erase; ++i)
            {
                // std::cout << id << " beg erased " << 10 * i << '\n';
                list.Erase(10 * i);
                // std::cout << id << " end erased " << 10 * i << '\n';
            }
        }};
    }

    for (auto& th : threads)
        th.join();

    std::cout << "Erase ended\n";

    std::cout << std::endl;
    std::thread{[&list, num_repeats](){
        for (int i = 0; i < num_repeats; ++i)
        {
            auto res = list.Find(10 * i);
            if ((i <  num_erase && res == true) ||
                (i >= num_erase && res == false))
            {
                std::cout << "Test Erase\tfailed!\n";
                return;
            }
        }
        std::cout << "Test Erase\tpassed!\n";
    }}.join();
}

void TestUniqListNative()
{
    lf::UniqList<int> list(16);
    std::thread th {
        [&list]() {
            list.Insert(1);
            list.Insert(1);
            list.Insert(2);
            list.Insert(7);
            list.Insert(-1);

            list.Dump();
            std::cout << std::endl;

            list.Erase(1);
            list.Erase(1);
            list.Erase(1);
            list.Dump();
            std::cout << std::endl;

            list.Insert(1);
            // list.Erase(1);
            list.Dump();
            std::cout << std::endl;

            list.Erase(-1);
            list.Erase(2);
            list.Erase(7);
            list.Erase(1);
            list.Dump();
            std::cout << std::endl;

            list.Erase(1);
            list.Dump();
            std::cout << std::endl;
        }
    };

    th.join();
}

void TestMultiThredingSOL(int num_threads)
{
    const int num_repeats = 30'000;
    const int num_erase = 13'000;

    lf::SplitOrderedList<int> list{(unsigned)num_threads};

    std::vector<std::thread> threads;
    for (int i_th = 0; i_th < num_threads; ++i_th)
    {
        threads.emplace_back([&list, num_repeats](){
            for (int i = 0; i < num_repeats; ++i)
            {
                list.Insert(10 * i);
                // std::cout << std::this_thread::get_id() << " inserted " << 10 * i << '\n';
            }
        });
    }

    for (auto& th : threads)
        th.join();
    std::cout << std::endl;
    std::thread{[&list, num_repeats](){
        for (int i = 0; i < num_repeats; ++i)
        {
            if (!list.Find(10 * i))
            {
                std::cout << "Test Insert\tfailed!\n";
                return;
            }
            // std::cout << std::this_thread::get_id() << " finded " << 10 * i << '\n';
        }
        std::cout << "Test Insert\tpassed!\n";
    }}.join();

    std::cout << std::endl;
    for (auto& th : threads)
    {
        th = std::thread{[&list, num_repeats](){
            // const auto id = std::this_thread::get_id();
            for (int i = 0; i < num_erase; ++i)
            {
                // std::cout << id << " beg erased " << 10 * i << '\n';
                list.Erase(10 * i);
                // std::cout << id << " end erased " << 10 * i << '\n';
            }
        }};
    }

    for (auto& th : threads)
        th.join();

    std::cout << "Erase ended\n";

    std::cout << std::endl;
    std::thread{[&list, num_repeats](){
        for (int i = 0; i < num_repeats; ++i)
        {
            auto res = list.Find(10 * i);
            if ((i <  num_erase && res == true) ||
                (i >= num_erase && res == false))
            {
                std::cout << "Test Erase\tfailed!\n";
                return;
            }
        }
        std::cout << "Test Erase\tpassed!\n";
    }}.join();
}

int64_t PerfTestSOL(unsigned num_threads)
{
    const int num_insert = 50'000;
    const int num_erase = num_insert / 20;

    lf::SplitOrderedList<int> sol{(unsigned)num_threads};

    auto time_begin = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i_th = 0; i_th < num_threads; ++i_th)
    {
        threads.emplace_back([&sol, num_insert, i_th, num_threads](){
            int i_begin = i_th * num_insert / num_threads;
            int i_end = (i_th + 1) * num_insert / num_threads;

            for (int i = i_begin; i < i_end; ++i)
                sol.Insert(10 * i);

            for (int i_repeat = 0; i_repeat < 10; ++i_repeat)
            {
                for (int i = i_begin; i < i_end; ++i)
                    sol.Find(10 * i);
            }

            for (int i = i_begin; i < i_end; ++i)
                sol.Erase(10 * i);
        });
    }

    for (auto& th : threads)
        th.join();

    auto time_end = std::chrono::high_resolution_clock::now();
    auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count();

    return dtime;
}

int main(int argc, char* argv[])
{
    int num_threads = argc == 2 ? atoi(argv[1]) : -1;
    if (num_threads <= 0)
    {
        std::cerr << "Inter the number of threads, please" << std::endl;
        return 0;
    }

    for (int i = 1; i <= num_threads; ++i)
        std::cout << i << " " << PerfTestSOL(i) << std::endl;

    return 0;
}