#pragma once

#include <array>
#include <algorithm>

namespace qmx
{

template <typename T>
constexpr bool IsPow2(T num) noexcept
{
    return num && (num & (num - 1)) == 0;
}

template <typename T, std::size_t N>
struct QMatrix
{
    static_assert(IsPow2(N), "Size must be power of 2");

    void Transpose(QMatrix<T, N>& res) const noexcept;
    QMatrix& MultAddToTransposed(const QMatrix& lhs, const QMatrix& rhs) noexcept;
    void Fill(T value) noexcept;

    std::array<std::array<T, N>, N> m_buf;
};

template <typename T, std::size_t N>
void QMatrix<T, N>::Transpose(QMatrix<T, N>& res) const noexcept
{
    for (std::size_t i_row = 0; i_row < N; ++i_row)
    {
        for (std::size_t i_col = 0; i_col < N; ++i_col)
        {
            res.m_buf[i_col][i_row] = m_buf[i_row][i_col];
        }
    }
}

template <typename T, std::size_t N>
QMatrix<T, N>& QMatrix<T, N>::MultAddToTransposed(const QMatrix& lhs, const QMatrix& rhs) noexcept
{
    for (std::size_t i_row = 0; i_row < N; ++i_row)
    {
        const auto& row = lhs.m_buf[i_row];
        for (std::size_t i_col = 0; i_col < N; ++i_col)
        {
            const auto& col = rhs.m_buf[i_col];

            T value{};
            for (std::size_t k = 0; k < N; ++k)
            {
                value += row[k] * col[k];
            }
            m_buf[i_row][i_col] += value;
        }
    }

    return *this;
}


template <typename T, std::size_t N>
void QMatrix<T, N>::Fill(T value) noexcept
{
    std::for_each(std::begin(m_buf), std::end(m_buf), [=](auto& buf) { buf.fill(value); });
}

} // namespcae qmx