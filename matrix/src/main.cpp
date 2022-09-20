#include <iostream>
#include "perf_test.h"

int main()
{
    std::ios_base::sync_with_stdio(false);

    const auto num = 10;
    for (mxcmn::SizeT n = 2; n < 2000; n += 1.25 * n)
    {        
        std::cout << n << ' ';
        std::flush(std::cout);

        mxnv::Matrix<double> a{n, n}, b{n, n};

        std::cout << PerfTest(a, b) << std::endl;
    }
}