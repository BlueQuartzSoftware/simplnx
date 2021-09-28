#pragma once

#include "complex/Common/Types.hpp"

#define H5_USE_110_API

namespace complex
{
namespace H5
{
using IdType = int64;
using ErrorType = int32;
using SizeType = unsigned long long;

inline const std::string DataTypeTag = "DataType";
inline const std::string CompDims = "ComponentDimensions";
inline const std::string TupleDims = "TupleDimensions";
} // namespace H5
} // namespace complex
