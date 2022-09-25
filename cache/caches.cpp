#include <cstdio>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cmath>

template <typename T>
std::size_t Work(std::vector<T>& buf)
{
    const auto num_repeats = 1000;
    
    const auto size = buf.size();
    
    T res{};
    for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
    {
        for (std::size_t i = 1; i < size; ++i)
        {
            buf[i-1] = buf[i] + buf[i-1];
        }
    }

    volatile auto tmp_buf = res;

    const std::size_t num_ops = num_repeats * size;
    return num_ops;
}

int main ()
{
    std::ios_base::sync_with_stdio(false);
    const std::size_t size_pow_min = 10;
    const std::size_t size_pow_max = 26;

    for (std::size_t size_pow = size_pow_min; size_pow <= size_pow_max; ++size_pow)
    {
        const std::size_t size = 1ul << size_pow;
        std::vector<int> vec(size);

        auto begin = std::chrono::high_resolution_clock::now();
        
        const auto num_ops = Work(vec);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        double time =  double(dt) / num_ops;

        std::cout << std::setw(10) << size_pow << ' ' << std::log10(time) << std::endl;
    }
}