#include <iostream>
#include <vector>
#include <algorithm>

#include "SplitOrderedList.hpp"

int main(int argc, char* argv[])
{
    int num_threads = argc == 2 ? atoi(argv[1]) : -1;
    if (num_threads <= 0)
    {
        std::cerr << "Inter the number of threads, please" << std::endl;
        return 0;
    }

    const int num_repeats = 30'000;
    const int num_erase = 13'000;

    lf::SortedList<int> list;

    std::vector <std::thread> threads;
    for (int i_th = 0; i_th < num_threads; ++i_th)
    {
        threads.emplace_back([&list, num_repeats](){
            for (int i = 0; i < num_repeats; ++i)
            {
                list.Insert(10 * i);
            }
        });
    }

    for (auto& th : threads)
        th.join();

    for (int i = 0; i < num_repeats; ++i)
    {
        if (list.Find(10 * i) == nullptr)
        {
            std::cout << "Test Insert\tfailed!\n";
            return 0;
        }
    }
    std::cout << "Test Insert\tpassed!\n";

    for (auto& th : threads)
    {
        th = std::thread{[&list, num_repeats](){
            for (int i = 0; i < num_erase; ++i)
            {
                list.Erase(10 * i);
            }
        }};
    }

    for (auto& th : threads)
        th.join();

    for (int i = 0; i < num_repeats; ++i)
    {
        auto* res = list.Find(10 * i);
        if ((i <  num_erase && res != nullptr) ||
            (i >= num_erase && res == nullptr))
        {
            std::cout << "Test Erase\tfailed!\n";
            return 0;
        }
    }
    std::cout << "Test Erase\tpassed!\n";
}