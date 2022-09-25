#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

#include "other_func.hpp"

int main()
{
    const std::size_t buf_size_mb = 200;

    const std::size_t buf_num_elems = buf_size_mb * 1'000'000;
    seclib::RandomGenerator RandGen;

    std::vector<std::pair<unsigned, double>> res;
    res.reserve(1024);

    const auto step_pow_min = 5;
    const auto step_pow_max = 8;
    const auto num_points_around = 10;
    const auto step_step = 1;

    for (std::size_t step_pow = step_pow_min; step_pow <= step_pow_max; ++step_pow)
    {
        const auto step_middle = 1ul << step_pow;
        const auto step_min = step_middle - num_points_around / 2;
        const auto step_max = step_min + num_points_around;
        for (std::size_t step = step_min; step <= step_max; step += step_step)
        {
            auto buf = RandGen.get_vector<uint8_t>(buf_num_elems);
            const auto size_buf = buf.size();

            auto begin = std::chrono::high_resolution_clock::now();

            uint64_t acc{};
            for (std::size_t pos = 0; pos < size_buf; pos += step)
            {
                acc += buf[pos];
            }
            
            // Save result in memory
            {
                volatile auto _ {acc};
            }
            auto end = std::chrono::high_resolution_clock::now();

            auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            double time =  double(dt) / (double (size_buf) / step);
            res.emplace_back(step, time);
        }

    }

    for (const auto[step, time] : res)
    {
        std::cout << std::setw(5) << step << ' ' << time << std::endl;
    }
}