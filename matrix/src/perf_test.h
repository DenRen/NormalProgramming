#pragma once

#include <random>
#include <chrono>
#include <vector>
#include <iostream>
#include <string>

namespace detail_valtest
{

template <typename L, typename R>
std::ostream& print_err(std::ostream& os, std::string err_msg, const L& lhs, const R& rhs
                        std::size_t line, std::string file)
{
    return os << file << ':' << line << " " << err_msg << ", lhs: " << lhs << ", rhs: " << rhs;
}

#define CHECK_EQ(lhs, rhs)                                              \
    do                                                                  \
    {                                                                   \
        if (lhs != rhs)                                                 \
        {                                                               \
            print_err(os, "NOT EQUAL", lhs, rhs, __LINE__, __FILE__);\
        }\
    } while(0);

template <template<typename> typename M>
void StaticTest()
{
    // const std::size_t size_pow_min = 7;
    // const std::size_t size_pow_max = 12;
    // const std::size_t size_pow_step = 1;

    // for (auto size_pow = size_pow_min; size_pow <= size_pow_max; size_pow += size_pow_step)
    // {
    //     const auto size = 1ul << size_pow;



    // }

    {
        
        if ()
    }
}

} // namespace detail_valtest

template <typename M>
bool Validate(std::ostream& os = std::cout)
{

}

template <typename M>
std::size_t PerfTest(const M& matrix_lhs, const M& matrix_rhs, const std::size_t num_repeats)
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

    return std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() / num_repeats;
}
