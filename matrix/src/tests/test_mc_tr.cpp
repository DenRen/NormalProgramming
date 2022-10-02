#include <type_traits>

#include "gtest/gtest.h"

#include "test_common.h"
#include "../matrix.h"

template <typename T>
using Matrix = mxnv::Matrix<T>;

template <template <typename> typename M, typename T>
void Divide(M<T> &m, T delim)
{
    const auto [num_rows, num_cols] = GetNumRowsCols(m);

    for (mxcmn::PositionT i_row = 0; i_row < num_rows; ++i_row)
    {
        for (mxcmn::PositionT i_col = 0; i_col < num_cols; ++i_col)
        {
            m[i_row][i_col] /= delim;
        }
    }
}

template <template <typename> typename M1,
          template <typename> typename M2, typename T>
void Divide(M1<T> &m1, M2<T> &m2, T delim)
{
    Divide(m1, delim);
    Divide(m2, delim);
}

template <template <typename> typename M1,
          template <typename> typename M2,
          template <typename> typename M3, typename T>
void Divide(M1<T> &m1, M2<T> &m2, M3<T> &m3, T delim)
{
    Divide(m1, m2, delim);
    Divide(m3, delim);
}

template <template <typename> typename M1,
          template <typename> typename M2,
          template <typename> typename M3,
          template <typename> typename M4, typename T>
void Divide(M1<T> &m1, M2<T> &m2, M3<T> &m3, M4<T> &m4, T delim)
{
    Divide(m1, m2, m3, delim);
    Divide(m4, delim);
}

template <template <typename> typename M, typename T>
void Test_Matrix_StaticCorrectMult(T delim = 1)
{
    // Pow 2 of 2x2
    {
        M<T> m{2, 2}, res{2, 2};

        // Test 1
        m[0][0] = 15;
        m[0][1] = 156;
        m[1][0] = 4894;
        m[1][1] = 123;

        res[0][0] = 763689;
        res[0][1] = 21528;
        res[1][0] = 675372;
        res[1][1] = 778593;

        Divide(m, res, res, delim);
        m *= m;
        MATRIX_IS_EQ(m, res);
        return;
        // Test 2
        m[0][0] = -15;
        m[0][1] = 156;
        m[1][0] = 4894;
        m[1][1] = 123;

        res[0][0] = 763689;
        res[0][1] = 16848;
        res[1][0] = 528552;
        res[1][1] = 778593;

        Divide(m, res, res, delim);
        m *= m;
        MATRIX_IS_EQ(m, res);

        // Test 3
        m[0][0] = -15;
        m[0][1] = 156;
        m[1][0] = -4894;
        m[1][1] = 123;

        res[0][0] = -763239;
        res[0][1] = 16848;
        res[1][0] = -528552;
        res[1][1] = -748335;

        Divide(m, res, res, delim);
        m *= m;
        MATRIX_IS_EQ(m, res);
    }

    // Mult of 2x2 * 2x1 = 2x2
    {
        M<T> a{2, 2}, b{2, 1}, res{2, 1};

        a[0][0] = 1;
        a[0][1] = 2;
        a[1][0] = 3;
        a[1][1] = 4;

        b[0][0] = 10;
        b[0][1] = 15;

        res[0][0] = 40;
        res[0][1] = 90;

        Divide(a, b, res, res, delim);
        a *= b;
        MATRIX_IS_EQ(a, res);
    }

    // Mult of 1x2 * 2x1 = 1x1
    {
        M<T> a{1, 2}, b{2, 1}, res{1, 1};

        a[0][0] = 3;
        a[0][1] = -2;

        b[0][0] = 7;
        b[0][1] = 0;

        res[0][0] = 21;

        Divide(a, b, res, res, delim);
        a *= b;
        MATRIX_IS_EQ(a, res);
    }

    // Mult of 3x2 * 2x3 = 3x3
    {
        M<T> a{3, 2}, b{2, 3}, res{3, 3};

        a[0][0] = -15;
        a[0][1] = 156;
        a[1][0] = -4894;
        a[1][1] = 123;
        a[2][0] = 0;
        a[2][1] = 0;

        b[0][0] = 0;
        b[0][1] = 8;
        b[0][2] = 2;
        b[1][0] = 63;
        b[1][1] = 0;
        b[1][2] = 9;

        res[0][0] = 9828;
        res[0][1] = -120;
        res[0][2] = 1374;
        res[1][0] = 7749;
        res[1][1] = -39152;
        res[1][2] = -8681;
        res[2][0] = 0;
        res[2][1] = 0;
        res[2][2] = 0;

        Divide(a, b, res, res, delim);
        a *= b;
        MATRIX_IS_EQ(a, res);
    }
}

template <typename M>
void Test_MatrixNative_IdentityMult()
{
    M a{10, 200}, b{200, 17}, res{10, 17};

    auto fill_ident = [](auto &m)
    {
        const auto size = std::min(m.GetNumRows(), m.GetNumCols());
        for (size_t i = 0; i < size; ++i)
        {
            m[i][i] = 1;
        }
    };

    fill_ident(a);
    fill_ident(b);
    fill_ident(res);

    a *= b;
    MATRIX_IS_EQ(a, res);
}

template <template <typename> typename M>
void TestStatic()
{
    Test_Matrix_StaticCorrectMult<M, int>();
    Test_Matrix_StaticCorrectMult<M, long>();
    Test_Matrix_StaticCorrectMult<M, long long>();

    for (float delim = 1; delim < 50; delim += 2)
    {
        Test_Matrix_StaticCorrectMult<M, float>(delim);
        Test_Matrix_StaticCorrectMult<M, double>(delim);
    }
    Test_MatrixNative_IdentityMult<M<long>>();
}

TEST(MatrixNative, Static)
{
    TestStatic<mxnv::Matrix>();
}

TEST(MatrixNativeTranspose, Static)
{
    TestStatic<mxtr::Matrix>();
}

TEST(MatrixNativeParallel, Static)
{
    TestStatic<mxnvpl::Matrix>();
}