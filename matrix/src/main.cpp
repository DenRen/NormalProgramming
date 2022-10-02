#include "matrix.h"
#include "perf_test.h"

template <typename ValueT>
void VsAll(const std::vector<std::pair<unsigned, unsigned>>& test_conf)
{
    // Print test conf
    std::ios_base::sync_with_stdio(false);
    std::cout << "Test conf:" << std::endl;
    for (const auto [num_cols, num_repeats] : test_conf)
    {
        const std::size_t size_bytes = num_cols * num_cols * sizeof(ValueT);
        const std::size_t size_Mbytes = size_bytes >> 20ul;
        std::cout << " " << num_cols << 'x' << num_cols << ": " << size_Mbytes << " Mbyte" << std::endl;
    }
    std::cout << std::endl;

    // Run tests
    PerfTest perf_test {test_conf};
    std::vector<std::vector<double>> test_times;

    std::cout << "native:" << std::endl;
    test_times.push_back(perf_test.Run<mxnv::Matrix<ValueT>>());
    std::cout << std::endl;

    std::cout << "native transpose:" << std::endl;
    test_times.push_back(perf_test.Run<mxtr::Matrix<ValueT>>());
    std::cout << std::endl;

    std::cout << "cache like:" << std::endl;
    test_times.push_back(perf_test.Run<mxcl::Matrix<ValueT, 64>>());
    std::cout << std::endl;

    std::cout << "native parallel:" << std::endl;
    test_times.push_back(perf_test.Run<mxnvpl::Matrix<ValueT>>());
    std::cout << std::endl;

    std::cout << "cache like parallel:" << std::endl;
    test_times.push_back(perf_test.Run<mxclpl::Matrix<ValueT, 64>>());
    std::cout << std::endl;

    // Analyze results
    std::cout << "Speed-up:" << std::endl;
    const auto& time_native = test_times[0];
    for (std::size_t i = 1; i < test_times.size(); ++i)
    {
        const auto& time = test_times[i];
        for (std::size_t j = 0; j < time_native.size(); ++j)
        {
            auto perf = time_native[j] / time[j] - 1;
            std::cout << "  " << std::setw(5) << std::left << std::setprecision(3) << perf;
            if (j + 1 < time_native.size())
            {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
}

int main()
{
    // { num_cols, num_test_repeats }
    std::vector<std::pair<unsigned, unsigned>> test_conf = {
        { 20 * 64, 2 },
        { 21 * 64, 2 },
        { 22 * 64, 1 },
        { 24 * 64, 1 },
        { 25 * 64, 1 }
    };
    using ValueT = double;

#if 1
    VsAll<ValueT>(test_conf);
#else
    PerfTest perf_test {test_conf};
    auto time = perf_test.Run<mxclpl::Matrix<ValueT, 64>>();
    // auto time = perf_test.Run<mxtr::Matrix<ValueT>>();
#endif
}