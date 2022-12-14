#pragma once

#include <stdexcept>
#include <vector>
#include <thread>
#include <iostream>
#include <iosfwd>

#include "qmatrix.h"

namespace mxclpl
{

template <typename T, std::size_t QSize>
class Matrix
{
    using QMatrix = qmx::QMatrix<T, QSize>;

public:
    using PositionT = mxcmn::PositionT;
    using SizeT = mxcmn::SizeT;

    static_assert(std::is_unsigned_v<PositionT>, "PositionT must be unsigned");
    static_assert(std::is_unsigned_v<SizeT>, "SizeT must be unsigned");

    class ProxyRow
    {
    public:
        ProxyRow(PositionT i_row, QMatrix* qrow_ptr) noexcept;
        T& operator[](PositionT col) const noexcept;

    private:
        PositionT m_i_row;
        QMatrix* m_qrow_ptr;
    };

    class ProxyRowConst
    {
    public:
        ProxyRowConst(PositionT i_row, const QMatrix* qrow_ptr) noexcept;
        const T& operator[](PositionT col) const noexcept;

    private:
        PositionT m_i_row;
        const QMatrix* m_qrow_ptr;
    };

    Matrix(SizeT num_rows, SizeT num_cols, int num_threads = -1);
    void Fill(T value) noexcept;
    ProxyRow operator[](PositionT row) noexcept;
    ProxyRowConst operator[](PositionT row) const noexcept;
    Matrix& operator*=(const Matrix& rhs);

    inline SizeT GetNumCols() const noexcept { return m_num_cols; }
    inline SizeT GetNumRows() const noexcept { return m_num_rows; }

private:
    inline SizeT GetNumQCols() const noexcept { return m_num_qcols; }
    inline SizeT GetNumQRows() const noexcept { return m_num_qrows; }

    template <typename U>
    bool IsCorrectMultSize(const Matrix<U, QSize>& rhs) const noexcept;

    template <typename U>
    void CheckCorrectMultSize(const Matrix<U, QSize>& rhs) const;

private:
    // size -> qsize
    SizeT CalcQNumFromNum(SizeT size);
    const QMatrix& GetQMatrix(PositionT i_qrow, PositionT i_qcol) const noexcept;
    QMatrix& GetQMatrix(PositionT i_qrow, PositionT i_qcol) noexcept;

    // Multithreading
    static unsigned CalcNumThreads(int num_threads) noexcept;
    void MultRow(const Matrix& rhs, Matrix& res, PositionT i_rhs_qcol_begin, PositionT i_rhs_qcol_end) noexcept;

private:
    SizeT m_num_rows, m_num_cols;
    SizeT m_num_qrows, m_num_qcols;
    std::vector<QMatrix> m_qbuf;
    unsigned m_num_threads;
};

// ProxyRow implementation ------------------------------------------------------------------------

template <typename T, std::size_t QSize>
Matrix<T, QSize>::ProxyRow::ProxyRow(PositionT i_row, QMatrix* qrow_ptr) noexcept
    : m_i_row{ i_row }, m_qrow_ptr{ qrow_ptr }
{}

template <typename T, std::size_t QSize>
T& Matrix<T, QSize>::ProxyRow::operator[](PositionT col) const noexcept
{
    return m_qrow_ptr[col / QSize].m_buf[m_i_row][col % QSize];
}

template <typename T, std::size_t QSize>
Matrix<T, QSize>::ProxyRowConst::ProxyRowConst(PositionT i_row, const QMatrix* qrow_ptr) noexcept
    : m_i_row{ i_row }, m_qrow_ptr{ qrow_ptr }
{}

template <typename T, std::size_t QSize>
const T& Matrix<T, QSize>::ProxyRowConst::operator[](PositionT col) const noexcept
{
    return m_qrow_ptr[col / QSize].m_buf[m_i_row][col % QSize];
}

// Matrix implementation --------------------------------------------------------------------------

template <typename T, std::size_t QSize>
unsigned Matrix<T, QSize>::CalcNumThreads(int num_threads) noexcept
{
    unsigned delim = 1;
    #ifdef __amd64__
        // Divide by 2 special for hypertraiding
        delim = 2;
    #endif

    return num_threads <= 0 ? std::max(std::thread::hardware_concurrency() / delim, 1u) : num_threads;
}

template <typename T, typename U>
auto CalcChunkSize(T size, U step)
{
    return size / step + (size % step != 0);
}

template <typename T, std::size_t QSize>
typename Matrix<T, QSize>::SizeT Matrix<T, QSize>::CalcQNumFromNum(SizeT size)
{
    return CalcChunkSize(size, QSize);
}

template <typename T, std::size_t QSize>
Matrix<T, QSize>::Matrix(SizeT num_rows, SizeT num_cols, int num_threads)
    : m_num_rows{ num_rows },
      m_num_cols{ num_cols },
      m_num_qrows{ CalcQNumFromNum(num_rows) },
      m_num_qcols{ CalcQNumFromNum(num_cols) },
      m_qbuf(m_num_qrows * m_num_qcols),
      m_num_threads{ CalcNumThreads(num_threads) }
{
    if (num_rows == 0 || num_cols == 0)
    {
        throw std::invalid_argument("num_rows and num_cols must be above zero");
    }
}
template <typename T, std::size_t QSize>
void Matrix<T, QSize>::Fill(T value) noexcept
{
    for (PositionT i_qrow = 0; i_qrow < m_num_qrows; ++i_qrow)
    {
        for (PositionT i_qcol = 0; i_qcol < m_num_qcols; ++i_qcol)
        {
            GetQMatrix(i_qrow, i_qcol).Fill(value);
        }
    }
}

template <typename T, std::size_t QSize>
typename Matrix<T, QSize>::ProxyRow Matrix<T, QSize>::operator[](PositionT row) noexcept
{
    PositionT i_row = row % QSize;
    PositionT i_qrow = row / QSize;
    return { i_row, &GetQMatrix(i_qrow, 0) };
}

template <typename T, std::size_t QSize>
typename Matrix<T, QSize>::ProxyRowConst Matrix<T, QSize>::operator[](PositionT row) const noexcept
{
    PositionT i_row = row % QSize;
    PositionT i_qrow = row / QSize;
    return { i_row, &GetQMatrix(i_qrow, 0) };
}

template <typename T, std::size_t QSize>
std::ostream& operator<<(std::ostream& os, const Matrix<T, QSize>& matrix)
{
    const std::size_t num_cols = matrix.GetNumCols();
    const std::size_t num_rows = matrix.GetNumRows();

    for (std::size_t i_row = 0; i_row < num_rows; ++i_row)
    {
        const auto& row = matrix[i_row];
        os << row[0];
        for (std::size_t i_col = 1; i_col < num_cols; ++i_col)
        {
            os << ' ' << row[i_col];
        }
        os << '\n';
    }

    return os;
}

template <typename T, std::size_t QSize>
template <typename U>
bool Matrix<T, QSize>::IsCorrectMultSize(const Matrix<U, QSize>& rhs) const noexcept
{
    return m_num_cols == rhs.m_num_rows;
}

template <typename T, std::size_t QSize>
template <typename U>
void Matrix<T, QSize>::CheckCorrectMultSize(const Matrix<U, QSize>& rhs) const
{
    if (!IsCorrectMultSize(rhs))
    {
        throw std::invalid_argument("Invalide mult sizes");
    }
}

template <typename T, std::size_t QSize>
const typename Matrix<T, QSize>::QMatrix&
Matrix<T, QSize>::GetQMatrix(PositionT i_qrow, PositionT i_qcol) const noexcept
{
    return m_qbuf[m_num_qcols * i_qrow + i_qcol];
}

template <typename T, std::size_t QSize>
typename Matrix<T, QSize>::QMatrix&
Matrix<T, QSize>::GetQMatrix(PositionT i_qrow, PositionT i_qcol) noexcept
{
    return m_qbuf[m_num_qcols * i_qrow + i_qcol];
}

template <typename T, std::size_t QSize>
void Matrix<T, QSize>::MultRow(const Matrix& rhs, Matrix& res,
                               PositionT i_rhs_qcol_begin, PositionT i_rhs_qcol_end) noexcept
{
    const auto K = m_num_qrows;
    QMatrix qm_tmp;
    for (PositionT i_rhs_qcol = i_rhs_qcol_begin; i_rhs_qcol < i_rhs_qcol_end; ++i_rhs_qcol)
    {
        for (PositionT i_rhs_qrow = 0; i_rhs_qrow < rhs.GetNumQRows(); ++i_rhs_qrow)
        {
            // Write transposed block from rhs to qm_tmp
            rhs.GetQMatrix(i_rhs_qrow, i_rhs_qcol).Transpose(qm_tmp);

            for (PositionT k_qrow = 0; k_qrow < K; ++k_qrow)
            {
                const auto& lqm = GetQMatrix(k_qrow, i_rhs_qrow);
                res.GetQMatrix(k_qrow, i_rhs_qcol).MultAddToTransposed(lqm, qm_tmp);
            }
        }
    }
}

template <typename T, std::size_t QSize>
Matrix<T, QSize>& Matrix<T, QSize>::operator*=(const Matrix& rhs)
{
    CheckCorrectMultSize(rhs);
    
    Matrix<T, QSize> res{GetNumRows(), rhs.GetNumCols()};
    res.Fill(0);

    std::vector <std::thread> workers;

    const auto i_rhs_qcol_begin_step = CalcChunkSize(rhs.GetNumQCols(), m_num_threads);
    for (PositionT i_rhs_qcol_begin = 0; i_rhs_qcol_begin < rhs.GetNumQCols(); i_rhs_qcol_begin += i_rhs_qcol_begin_step)
    {
        const auto i_rhs_qcol_end = std::min(i_rhs_qcol_begin + i_rhs_qcol_begin_step, rhs.GetNumQCols());
        workers.emplace_back(&Matrix<T, QSize>::MultRow, this,
                             std::cref(rhs), std::ref(res), i_rhs_qcol_begin, i_rhs_qcol_end);
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    *this = std::move(res);
    return *this;
}

} // namespace mxclpl
