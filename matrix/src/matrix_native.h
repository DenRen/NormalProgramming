#pragma once

#include <stdexcept>
#include <vector>
#include <iosfwd>

namespace mxnv
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

    Matrix(SizeT num_rows, SizeT num_cols);
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

private:
    PositionT m_num_rows, m_num_cols;
    std::vector<T> m_buf;
};

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

template <typename T>
Matrix<T>::Matrix(SizeT num_rows, SizeT num_cols)
    : m_num_rows{ num_rows }
    , m_num_cols{ num_cols }
    , m_buf( num_rows * num_cols )
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
Matrix<T>& Matrix<T>::operator *=(const Matrix& rhs)
{
    CheckCorrectMultSize(rhs);

    Matrix<T> res_matrix{m_num_rows, rhs.m_num_cols};

    for (PositionT i_left_row = 0; i_left_row < m_num_rows; ++i_left_row)
    {
        const auto& row = (*this)[i_left_row];
        
        for (PositionT i_right_col = 0; i_right_col < rhs.m_num_cols; ++i_right_col)
        {
            T res{};
            for (PositionT k = 0; k < m_num_cols; ++k)
            {
                res += row[k] * rhs[k][i_right_col];
            }

            res_matrix[i_left_row][i_right_col] = res;
        }
    }

    *this = std::move(res_matrix);

    return *this;
}

} // namespace mxnv
