#pragma once

#include <stdexcept>
#include <vector>
#include <array>
#include <iosfwd>
#include <algorithm>

namespace mxcl
{

namespace detail
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

} // namespace detail

template <typename T, std::size_t QSize>
class Matrix
{
    using QMatrix = detail::QMatrix<T, QSize>;

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

    Matrix(SizeT num_rows, SizeT num_cols);
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

private:
    SizeT m_num_rows, m_num_cols;
    SizeT m_num_qrows, m_num_qcols;
    std::vector<QMatrix> m_qbuf;
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
typename Matrix<T, QSize>::SizeT Matrix<T, QSize>::CalcQNumFromNum(SizeT size)
{
    return size / QSize + (size % QSize != 0);
}

template <typename T, std::size_t QSize>
Matrix<T, QSize>::Matrix(SizeT num_rows, SizeT num_cols)
    : m_num_rows{ num_rows },
      m_num_cols{ num_cols },
      m_num_qrows{ CalcQNumFromNum(num_rows) },
      m_num_qcols{ CalcQNumFromNum(num_cols) },
      m_qbuf(m_num_qrows * m_num_qcols)
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
Matrix<T, QSize>& Matrix<T, QSize>::operator*=(const Matrix& rhs)
{
    CheckCorrectMultSize(rhs);
    
    Matrix<T, QSize> res{GetNumRows(), rhs.GetNumCols()};
    res.Fill(0);

    const auto K = m_num_qrows;
    QMatrix qm_tmp;
    for (PositionT i_rhs_qrow = 0; i_rhs_qrow < rhs.GetNumQRows(); ++i_rhs_qrow)
    {
        for (PositionT i_rhs_qcol = 0; i_rhs_qcol < rhs.GetNumQCols(); ++i_rhs_qcol)
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

    *this = std::move(res);
    return *this;
}

} // namespace mxcl
