#pragma once

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

#include <map>
#include <vector>

namespace complex::MontageUtilities
{
// -----------------------------------------------------------------------------
std::map<int32, std::vector<usize>> Burn(int32 tolerance, std::vector<int32>& input);

int32 CalculatePaddingDigits(usize count);
} // namespace complex::MontageUtilities
