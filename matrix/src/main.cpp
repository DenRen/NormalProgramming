#include <iostream>
#include <iomanip>
#include "matrix.h"
#include "perf_test.h"

template <typename M>
void StartPerfTest()
{
    // { num_cols, num_test_repeats }
    std::vector<std::pair<unsigned, unsigned>> test_conf = {
        { 20 * 64, 2 },
        { 24 * 64, 1 }
    };

    for (const auto [num_cols, num_repeats] : test_conf)
    {
        std::cout << std::setw(5) << num_cols << ' ';
        std::flush(std::cout);

        M a{ num_cols, num_cols }, b{ num_cols, num_cols };

        std::cout << PerfTest(a, b, num_repeats) << std::endl;
    }
}

int main()
{
    std::ios_base::sync_with_stdio(false);

    // StartPerfTest<mxnv::Matrix<double>>();
    StartPerfTest<mxtr::Matrix<double>>();
}