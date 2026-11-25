#pragma once

#include <algorithm>

namespace fusion {
namespace debugviewpp {

constexpr int kMinProcedureStatsSplitPct = 40;
constexpr int kMaxProcedureStatsSplitPct = 100;

inline int ClampProcedureStatsSplitPct(int value)
{
    if (value <= 0)
        return kMinProcedureStatsSplitPct;
    return std::clamp(value, kMinProcedureStatsSplitPct, kMaxProcedureStatsSplitPct);
}

} // namespace debugviewpp
} // namespace fusion
