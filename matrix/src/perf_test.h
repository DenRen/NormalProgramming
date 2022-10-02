#pragma once

#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

template <typename M>
double RunPerfTest(const M& matrix_lhs, const M& matrix_rhs, const std::size_t num_repeats)
{
    std::vector<M> ls(num_repeats, matrix_lhs), rs(num_repeats, matrix_rhs);

    auto time_begin = std::chrono::high_resolution_clock::now();
    for (std::size_t i_repeat = 0; i_repeat < num_repeats; ++i_repeat)
    {
        ls[i_repeat] *= rs[i_repeat];

        const auto& res = ls[i_repeat];
        volatile auto tmp = res[res.GetNumRows() - 1][res.GetNumCols() - 1];
    }
    auto time_end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() / double(num_repeats);
}

class PerfTest
{
public:
    // test_conf: { num_cols, num_test_repeats }
    PerfTest(const std::vector<std::pair<unsigned, unsigned>>& test_conf)
        : m_test_conf(test_conf)
    {}

    template <typename M>
    std::vector<double> Run(std::ostream& os = std::cout)
    {
        std::vector<double> res_time;
        for (const auto [num_cols, num_repeats] : m_test_conf)
        {
            os << std::setw(5) << num_cols << ' ';
            std::flush(os);

            M a{ num_cols, num_cols }, b{ num_cols, num_cols };

            res_time.push_back(RunPerfTest(a, b, num_repeats));
            os << *res_time.crbegin() << '\n';
        }

        return res_time;
    }

private:
    std::vector<std::pair<unsigned, unsigned>> m_test_conf;
};

class PerfTestMultiThreads
{
public:
    PerfTestMultiThreads(int num_threads_min, int num_threads_max,
                         unsigned num_cols, unsigned num_repeats)
        : m_num_threads_min{ num_threads_min }
        , m_num_threads_max{ num_threads_max }
        , m_num_cols{ num_cols }
        , m_num_repeats{ num_repeats }
    {}

    template <typename M>
    std::vector<std::pair<unsigned, double>> Run(std::ostream& os = std::cout) const
    {
        std::vector<std::pair<unsigned, double>> res_time;

        for (auto num_threads = m_num_threads_min; num_threads <= m_num_threads_max; ++num_threads)
        {
            os << std::setw(2) << num_threads << ' ';
            std::flush(os);

            M a{ m_num_cols, m_num_cols, num_threads }, b{ m_num_cols, m_num_cols, num_threads };

            double time = RunPerfTest(a, b, m_num_repeats);
            res_time.emplace_back(num_threads, time);
            os << time << '\n';
        }

        return res_time;
    }

private:
    std::vector<std::pair<unsigned, unsigned>> m_test_conf;
    int m_num_threads_min, m_num_threads_max;
    unsigned m_num_cols, m_num_repeats;
};