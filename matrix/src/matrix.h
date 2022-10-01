#pragma once

namespace mxcmn
{
    using PositionT = unsigned;
    using SizeT = unsigned;
}

#include "matrix_native.h"
#include "matrix_native_parallel.h"
#include "matrix_cachelike.h"
#include "matrix_tr.h"
#include "matrix_cachelike_parallel.h"

template <typename M>
std::pair<mxcmn::SizeT, mxcmn::SizeT> GetNumRowsCols(const M& m)
{
    return { m.GetNumRows(), m.GetNumCols() };
}
