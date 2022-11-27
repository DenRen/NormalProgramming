#include <iostream>
#include <vector>
#include <algorithm>

#include "SplitOrderedList.hpp"

int main()
{
    const int num_threads = 4;
    const int num_repeats = 30'000;

    lf::SortedList<int> list;

    std::vector <std::thread> threads;
    for (int i_th = 0; i_th < num_threads; ++i_th)
    {
        threads.emplace_back([&list, num_repeats](){
            #if 0
                list.PushFront(0);
                for (int i = 1; i < num_repeats; ++i)
                {
                #if 1
                    auto it = list.begin();
                    list.InsertAfter(it, 10 * i);
                #else
                    list.PushFront(10 * i);
                #endif
                }
            #else
                for (int i = 0; i < num_repeats; ++i)
                {
                    list.Insert(10 * i);
                }

                for (int i = num_repeats - 100; i < num_repeats; ++i)
                {
                    list.Erase(10 * i);
                }
            #endif
        });
    }

    for (auto& th : threads)
        th.join();

    for (auto& th : threads)
        th = std::thread{[&list, num_repeats](){
            for (int i = num_repeats - 100; i < num_repeats; ++i)
            {
                list.Erase(10 * i);
            }
        }};

    for (auto& th : threads)
        th.join();

#if 1
    long sum = 0;
    for (auto val : list)
    {
        // std::cout << val << std::endl;
        sum += val;
    }
    const long ref_sum = 10l * (num_repeats - 100) * ((num_repeats - 100) - 1) / 2;

    std::cout << "Result: ";
    if (sum == ref_sum && std::is_sorted(list.begin(), list.end()))
    {
        std::cout << "Good!\n";
    }
    else
    {
        std::cout << sum << " != Ref: " << ref_sum << '\n';
        std::cout << "Fail!\n";
    }
#endif

#if 0
    std::ios_base::sync_with_stdio(false);
    for (auto val : list)
        std::cout << val << '\n';
    std::cout.flush();
#endif
}