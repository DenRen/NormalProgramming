#pragma once

#include <stdexcept>
#include <vector>
#include <thread>
#include <iosfwd>
#include <iostream>

namespace mxnvpl
{

template <typename T>
class Matrix
{
public:
    using PositionT = mxcmn::PositionT;
    using SizeT = mxcmn::SizeT;

    static_assert(std::is_unsigned_v<PositionT>, "PositionT must be unsigned");
    static_assert(std::is_unsigned_v<SizeT>, "SizeT must be unsigned");

    class ProxyRow
    {
    public:
        ProxyRow(T* row_ptr) noexcept;
        T& operator[] (PositionT col) const noexcept;

    private:
        T* m_row_ptr;
    };

    class ProxyRowConst
    {
    public:
        ProxyRowConst(const T* row_ptr) noexcept;
        const T& operator[] (PositionT col) const noexcept;

    private:
        const T* m_row_ptr;
    };

    Matrix(SizeT num_rows, SizeT num_cols, int num_threads = -1);
    ProxyRow operator[](PositionT row) noexcept;
    ProxyRowConst operator[](PositionT row) const noexcept;
    Matrix& operator *= (const Matrix& rhs);

    template <typename U>
    bool operator ==(const Matrix<U>& rhs) const noexcept;

    inline SizeT GetNumCols() const noexcept;
    inline SizeT GetNumRows() const noexcept;

private:
    template <typename U>
    bool IsCorrectMultSize(const Matrix<U>& rhs) const noexcept;

    template <typename U>
    void CheckCorrectMultSize(const Matrix<U>& rhs) const;

    // Multithreading
    static unsigned CalcNumThreads(int num_threads) noexcept;
    void MultRow(const Matrix& rhs, Matrix& res, PositionT i_rhs_col_begin, PositionT i_rhs_col_end) noexcept;

private:
    PositionT m_num_rows, m_num_cols;
    std::vector<T> m_buf;
    unsigned m_num_threads;
};

// ProxyRow implementation ------------------------------------------------------------------------

template <typename T>
Matrix<T>::ProxyRow::ProxyRow(T* row_ptr) noexcept
    : m_row_ptr{ row_ptr }
{}

template <typename T>
T& Matrix<T>::ProxyRow::operator[](PositionT col) const noexcept
{
    return m_row_ptr[col];
}

template <typename T>
Matrix<T>::ProxyRowConst::ProxyRowConst(const T* row_ptr) noexcept
    : m_row_ptr{ row_ptr }
{}

template <typename T>
const T& Matrix<T>::ProxyRowConst::operator[](PositionT col) const noexcept
{
    return m_row_ptr[col];
}

// Matrix implementation --------------------------------------------------------------------------
template <typename T>
unsigned Matrix<T>::CalcNumThreads(int num_threads) noexcept
{
    unsigned delim = 1;
    #ifdef __amd64__
        // Divide by 2 special for hypertraiding
        delim = 2;
    #endif

    return num_threads <= 0 ? std::max(std::thread::hardware_concurrency() / delim, 1u) : num_threads;
}

template <typename T>
Matrix<T>::Matrix(SizeT num_rows, SizeT num_cols, int num_threads)
    : m_num_rows{ num_rows }
    , m_num_cols{ num_cols }
    , m_buf( num_rows * num_cols )
    , m_num_threads{ CalcNumThreads(num_threads) }
{
    if (!num_rows || !num_cols)
    {
        throw std::invalid_argument("num_rows and num_cols must be above zero");
    }
}

template<typename T>
typename Matrix<T>::ProxyRow Matrix<T>::operator[] (PositionT row) noexcept
{
    return { m_buf.data() + row * m_num_cols };
}

template<typename T>
typename Matrix<T>::ProxyRowConst Matrix<T>::operator[] (PositionT row) const noexcept
{
    return { m_buf.data() + row * m_num_cols };
}

template<typename T>
template <typename U>
bool Matrix<T>::operator ==(const Matrix<U>& rhs) const noexcept
{
    return m_buf == rhs.m_buf;
}

template<typename T>
typename Matrix<T>::SizeT Matrix<T>::GetNumCols() const noexcept
{
    return m_num_cols;
}

template<typename T>
typename Matrix<T>::SizeT Matrix<T>::GetNumRows() const noexcept
{
    return m_num_rows;
}

template <typename T>
std::ostream& operator <<(std::ostream& os, const Matrix<T>& matrix)
{
    const std::size_t num_cols = matrix.GetNumCols();
    const std::size_t num_rows = matrix.GetNumRows();

    os << '{';
    for (std::size_t i_row = 0; i_row < num_rows; ++i_row)
    {
        const auto& row = matrix[i_row];
        os << row[0];
        for (std::size_t i_col = 1; i_col < num_cols; ++i_col)
        {
            os << ',' << row[i_col];
        }
        os << ';';
    }

    return os << '}';
}

template <typename T>
template <typename U>
bool Matrix<T>::IsCorrectMultSize(const Matrix<U>& rhs) const noexcept
{
    return m_num_cols == rhs.m_num_rows;
}

template <typename T>
template <typename U>
void Matrix<T>::CheckCorrectMultSize(const Matrix<U>& rhs) const
{
    if (!IsCorrectMultSize(rhs))
    {
        throw std::invalid_argument("Invalide mult sizes");
    }
}

template <typename T>
void Matrix<T>::MultRow(const Matrix& rhs, Matrix& res, PositionT i_rhs_col_begin, PositionT i_rhs_col_end) noexcept
{
    const auto K = GetNumCols();
    for (PositionT i_left_row = 0; i_left_row < GetNumRows(); ++i_left_row)
    {
        const auto& row = (*this)[i_left_row];
        
        for (PositionT i_right_col = i_rhs_col_begin; i_right_col < i_rhs_col_end; ++i_right_col)
        {
            T value{};
            for (PositionT k = 0; k < K; ++k)
            {
                value += row[k] * rhs[k][i_right_col];
            }

            res[i_left_row][i_right_col] = value;
        }
    }
}

template <typename T, typename U>
auto CalcChunkSize(T size, U step)
{
    return size / step + (size % step != 0);
}

template <typename T>
Matrix<T>& Matrix<T>::operator *=(const Matrix& rhs)
{
    CheckCorrectMultSize(rhs);

    Matrix<T> res{GetNumRows(), rhs.GetNumCols()};
    
    std::vector <std::thread> workers;

    const auto i_rhs_col_begin_step = CalcChunkSize(rhs.GetNumCols(), m_num_threads);
    for (PositionT i_rhs_col_begin = 0; i_rhs_col_begin < rhs.GetNumCols(); i_rhs_col_begin += i_rhs_col_begin_step)
    {
        const auto i_rhs_col_end = std::min(i_rhs_col_begin + i_rhs_col_begin_step, rhs.GetNumCols());
        workers.emplace_back(&Matrix<T>::MultRow, this,
                             std::cref(rhs), std::ref(res), i_rhs_col_begin, i_rhs_col_end);
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    *this = std::move(res);
    return *this;
}

} // namespace mxnv
