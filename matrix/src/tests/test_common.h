#pragma once

#define MATRIX_IS_EQ(lhs, rhs)                                                                         \
    do                                                                                                 \
    {                                                                                                  \
        const auto [num_rows, num_cols] = GetNumRowsCols(lhs);                                         \
                                                                                                       \
        {                                                                                              \
            ASSERT_EQ(num_rows, rhs.GetNumRows());                                                     \
            ASSERT_EQ(num_cols, rhs.GetNumCols());                                                     \
        }                                                                                              \
                                                                                                       \
        for (mxcmn::PositionT i_row = 0; i_row < num_rows; ++i_row)                                    \
        {                                                                                              \
            for (mxcmn::PositionT i_col = 0; i_col < num_cols; ++i_col)                                \
            {                                                                                          \
                const auto &lhs_value = lhs[i_row][i_col];                                             \
                const auto &rhs_value = rhs[i_row][i_col];                                             \
                                                                                                       \
                using lhs_value_t = std::remove_const_t<std::remove_reference_t<decltype(lhs_value)>>; \
                using rhs_value_t = std::remove_const_t<std::remove_reference_t<decltype(rhs_value)>>; \
                using common_t = std::common_type_t<lhs_value_t, rhs_value_t>;                         \
                                                                                                       \
                if constexpr (std::is_floating_point_v<common_t>)                                      \
                {                                                                                      \
                    if constexpr (std::is_same_v<common_t, float>)                                     \
                    {                                                                                  \
                        ASSERT_FLOAT_EQ(lhs_value, rhs_value) << lhs << rhs;                           \
                    }                                                                                  \
                    else                                                                               \
                    {                                                                                  \
                        ASSERT_DOUBLE_EQ(lhs_value, rhs_value) << lhs << rhs;                          \
                    }                                                                                  \
                }                                                                                      \
                else                                                                                   \
                {                                                                                      \
                    ASSERT_EQ(lhs_value, rhs_value) << lhs << rhs;                                     \
                }                                                                                      \
            }                                                                                          \
        }                                                                                              \
    } while (0)
