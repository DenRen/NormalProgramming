#include "gtest/gtest.h"
#include "../matrix.h"

template <typename T>
using Matrix = mxnv::Matrix<T>;

template <typename M>
void Test_MatrixNative_StaticCorrectMult()
{
    {
        M a{2, 2}, b{2, 1}, res{2, 1};
        
        a[0][0] = 1; a[0][1] = 2;
        a[1][0] = 3; a[1][1] = 4;

        b[0][0] = 10;
        b[0][1] = 15;

        res[0][0] = 40;
        res[0][1] = 90;

        a *= b;
        ASSERT_EQ(a, res);
    }

    {
        M a{1, 2}, b{2, 1}, res{1, 1};
        
        a[0][0] = 3; a[0][1] = -2;

        b[0][0] = 7;
        b[0][1] = 0;

        res[0][0] = 21;

        a *= b;
        ASSERT_EQ(a, res);
    }
}

template <typename M>
void Test_MatrixNative_IdentityMult()
{
    M a{10, 200}, b{200, 17}, res{10, 17};

    auto fill_ident = [](auto& m)
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
    ASSERT_EQ(a, res);
}

TEST(MatrixNative, Static)
{
    Test_MatrixNative_StaticCorrectMult<mxnv::Matrix<long>>();
    Test_MatrixNative_IdentityMult<mxnv::Matrix<long>>();
}

TEST(MatrixCacheLike, Static)
{
    Test_MatrixNative_StaticCorrectMult<mxtr::Matrix<long>>();
    Test_MatrixNative_IdentityMult<mxtr::Matrix<long>>();
}