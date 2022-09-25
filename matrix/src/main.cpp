#include <iostream>
#include <iomanip>
#include "perf_test.h"

template <typename M>
void StartPerfTest()
{
    // { num_cols, num_test_repeats }
    std::vector<std::pair<unsigned, unsigned>> test_conf = {
        // { 80, 20 },
        // { 100, 10 },
        // { 200, 5 },
        // { 400, 5 },
        // { 500, 5 },
        // { 700, 5 },
        // { 900, 5 },
        { 1500, 1 }
    };

    for (const auto[num_cols, num_repeats] : test_conf)
    {
        std::cout << std::setw(5) << num_cols << ' ';
        std::flush(std::cout);

        M a{num_cols, num_cols}, b{num_cols, num_cols};

        std::cout << PerfTest(a, b, num_repeats) << std::endl;
    }
}

int main()
{
    std::ios_base::sync_with_stdio(false);

    // StartPerfTest<mxnv::Matrix<double>>();
    StartPerfTest<mxtr::Matrix<double>>();
}