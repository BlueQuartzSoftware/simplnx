#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace ResamplingUtilities
{
inline constexpr nx::core::StringLiteral k_ExactDimensions = "Exact Dimensions (0)";
inline constexpr nx::core::StringLiteral k_ScalingMode = "Scaling (1)";
const nx::core::ChoicesParameter::Choices k_ResamplingChoices = {k_ExactDimensions, k_ScalingMode};
const nx::core::ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 0;
const nx::core::ChoicesParameter::ValueType k_ScalingModeIndex = 1;

namespace Calculations
{
std::vector<nx::core::uint64> ExactDimsFromScaling(const nx::core::ImageGeom& image, const std::vector<nx::core::float32>& scalingXY);
std::vector<nx::core::uint64> ExactSpacingFromScaling(const nx::core::ImageGeom& image, const std::vector<nx::core::float32>& scalingXY);
}
}