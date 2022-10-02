#include <fstream>
#include "matrix.h"
#include "perf_test.h"

/*
1) время умножения матриц от размера матриц для наивной однопоточной реализации
2) время умножения матриц от размера матриц для блочного умножения в 1 поток
3) время умножения матриц от количества тредов для не-блочного разделения задач по тредам (например, по строкам / столбцам)
4) время умножения матриц от количества тредов для блочной реализации
*/

template <typename ValueT>
void TimeSizeForNativeAndCacheLike(std::vector<std::pair<unsigned, unsigned>>& test_conf)
{
    using MatrixNative = mxnv::Matrix<ValueT>;
    using MatrixCacheLike = mxcl::Matrix<ValueT, 64>;

    std::fstream fs_native{"time_native", std::ios::out};
    std::fstream fs_cachelike{"time_cachelike", std::ios::out};

    if (fs_native.bad() || fs_cachelike.bad())
    {
        throw std::runtime_error("fstream");
    }

    PerfTest perf_test{test_conf};
    perf_test.Run<MatrixNative>(fs_native);
    perf_test.Run<MatrixCacheLike>(fs_cachelike);
}

// #define TIME_SIZE
#define TIME_NUM_THREADS


template <typename ValueT>
void TimeNumThreadsForNativeAndCacheLike(int num_threads_min, int num_threads_max,
                                         unsigned num_cols, unsigned num_repeats)
{
    using MatrixNative = mxnvpl::Matrix<ValueT>;
    using MatrixCacheLike = mxclpl::Matrix<ValueT, 64>;

    std::fstream fs_native{"time_native_parallel", std::ios::out};
    std::fstream fs_cachelike{"time_cachelike_parallel", std::ios::out};

    if (fs_native.bad() || fs_cachelike.bad())
    {
        throw std::runtime_error("fstream");
    }

    PerfTestMultiThreads perf_test{num_threads_min, num_threads_max, num_cols, num_repeats};
    perf_test.Run<MatrixNative>(fs_native);
    perf_test.Run<MatrixCacheLike>(fs_cachelike);
}

int main()
{
    using ValueT = double;

#ifdef TIME_SIZE
    // { num_cols_rows, num_test_repeats }
    std::vector<std::pair<unsigned, unsigned>> test_conf = {
        { 10 * 64, 3 },
        { 11 * 64, 3 },
        { 12 * 64, 3 },
        { 13 * 64, 3 },
        { 14 * 64, 3 },
        { 15 * 64, 3 },
        { 16 * 64, 3 },
        { 17 * 64, 3 },
        { 18 * 64, 2 },
        { 19 * 64, 2 },
        { 20 * 64, 2 },
        { 21 * 64, 2 },
        { 22 * 64, 1 },
        { 24 * 64, 1 },
        { 25 * 64, 1 }
    };
    TimeSizeForNativeAndCacheLike<ValueT>(test_conf);
#endif

#ifdef TIME_NUM_THREADS
    TimeNumThreadsForNativeAndCacheLike<ValueT>(1, 8, 24 * 64, 3);
#endif
}