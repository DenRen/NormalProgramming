#include "gtest/gtest.h"

#include "test_common.h"
#include "../matrix.h"
#include "../other_func.hpp"

// Here implemented the test only for square matrices

template <typename T>
using RefMatrixT = mxtr::Matrix<T>;

constexpr mxcmn::SizeT QMatrixSize = 64;

template <typename M, typename... Args>
auto GetRandomSquareMatrix(const mxcmn::SizeT num_rows_cols, Args... args)
{
    const auto size = num_rows_cols;
    M m{size, size};

    seclib::RandomGenerator rand;
    for (mxcmn::PositionT i_row = 0; i_row < size; ++i_row)
    {
        for (mxcmn::PositionT i_col = 0; i_col < size; ++i_col)
        {
            using ValueT = std::remove_cv_t<std::remove_reference_t<decltype(m[0][0])>>;
            m[i_row][i_col] = rand.get_rand_val<ValueT>(args...);
        }
    }

    return m;
}

template <typename M>
auto CreateRefMatrix(const M& m)
{
    const auto [num_rows, num_cols] = GetNumRowsCols(m);

    using ValueT = std::remove_cv_t<std::remove_reference_t<decltype(m[0][0])>>;
    RefMatrixT<ValueT> res{num_rows, num_cols};
    for (mxcmn::PositionT i_row = 0; i_row < num_rows; ++i_row)
    {
        for (mxcmn::PositionT i_col = 0; i_col < num_cols; ++i_col)
        {
            res[i_row][i_col] = m[i_row][i_col];
        }
    }

    return res;
}

template<typename M>
void RandomTest()
{
    const mxcmn::SizeT num_row_min = QMatrixSize;
    const std::size_t num_step = 3;
    const std::size_t num_repeat = 5;

    for (std::size_t i_repeat = 0; i_repeat < num_repeat; ++i_repeat)
    {
        for (std::size_t i_step = 1; i_step <= num_step; ++i_step)
        {
            const auto num_row = i_step * num_row_min;
            auto a = GetRandomSquareMatrix<M>(num_row, -1000, 1000);
            auto b = GetRandomSquareMatrix<M>(num_row, -1000, 1000);

            auto a_ref = CreateRefMatrix(a);
            auto b_ref = CreateRefMatrix(b);

            for (int i_op = 0; i_op < 3; ++i_op)
            {
                a *= b;
                a_ref *= b_ref;
                MATRIX_IS_EQ(a, a_ref);

                a *= a;
                a_ref *= a_ref;
                MATRIX_IS_EQ(a, a_ref);

                b *= a;
                b_ref *= a_ref;
                MATRIX_IS_EQ(b, b_ref);

                b *= b;
                b_ref *= b_ref;
                MATRIX_IS_EQ(b, b_ref);
            }
        }
    }
}

TEST(MatrixCacheLike, RandomTest)
{
    RandomTest <mxcl::Matrix<long,  32>>();
    RandomTest <mxcl::Matrix<long,  64>>();
    RandomTest <mxcl::Matrix<long, 128>>();
}

TEST(MatrixCacheLikeParallel, RandomTest)
{
    RandomTest <mxclpl::Matrix<long,  32>>();
    RandomTest <mxclpl::Matrix<long,  64>>();
    RandomTest <mxclpl::Matrix<long, 128>>();
}
