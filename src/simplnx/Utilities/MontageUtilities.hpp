#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <map>
#include <vector>

namespace nx::core::MontageUtilities
{
// -----------------------------------------------------------------------------
std::map<int32, std::vector<usize>> Burn(int32 tolerance, const std::vector<int32>& input);

int32 CalculatePaddingDigits(usize count);
} // namespace nx::core::MontageUtilities
