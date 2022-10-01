#pragma once

#include <random>
#include <chrono>
#include <vector>
#include <iostream>
#include <string>

template <typename M>
double RunPerfTest(const M& matrix_lhs, const M& matrix_rhs, const std::size_t num_repeats)
{
    std::vector<M> ls(num_repeats, matrix_lhs), rs(num_repeats, matrix_rhs);

    auto time_begin = std::chrono::high_resolution_clock::now();
    for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
    {
        ls[i_repeat] *= rs[i_repeat];

        const auto& res = ls[i_repeat];
        volatile auto tmp = ls[res.GetNumRows() - 1][res.GetNumCols() - 1];
    }
    auto time_end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() / double(num_repeats);
}
