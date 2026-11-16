#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"

#include <fmt/format.h>
#include <limits>
#include <utility>

namespace nx::core::ImageProcessing::detail
{
/**
 * @brief Adds Curvature Flow conversion diagnostics after the transfer diagnostics.
 * @param state Conversion outcomes from the completed buffers.
 * @param result Diagnostics received before finalization.
 * @return The transfer and conversion diagnostics in their original order.
 */
inline Result<> ReportCurvatureFlowConversion(const TerminalConversionState& state, Result<> result)
{
  Result<> conversion;
  const usize failedIndex = state.nonFiniteIndex();
  if(failedIndex != std::numeric_limits<usize>::max())
  {
    conversion = MakeErrorResult(
        k_CurvatureFlowNonFiniteIntegerOutput,
        fmt::format("Curvature Flow produced a nonfinite integer output at flat value index {}. Reduce Time Step or use a floating-point input array to retain nonfinite output.", failedIndex));
  }
  if(state.clipped())
  {
    conversion.warnings().push_back({.code = k_CurvatureFlowClippedIntegerOutput,
                                     .message = "Curvature Flow clipped final integer values to the output type's range after truncation. The evolution may be unstable. "
                                                "Reduce Time Step and check downstream masks and segmentation for clipping plateaus."});
  }
  return MergeResults(std::move(result), std::move(conversion));
}

/**
 * @brief Reports a conversion failure to stop the active smoothing pass.
 * @tparam T Output pixel type used in the error message.
 * @param state Conversion outcomes from the completed buffers.
 * @return An error at the lowest rejected index, or a valid result when there is no rejection.
 */
template <class T>
Result<> SmoothingGaussianNonFiniteResult(const TerminalConversionState& state)
{
  if(state.nonFiniteIndex() == std::numeric_limits<usize>::max())
  {
    return {};
  }
  return MakeErrorResult(k_SmoothingGaussianNonFiniteIntegerOutput, fmt::format("Smoothing Recursive Gaussian produced a nonfinite value at flat value index {} for output type '{}'. Use a "
                                                                                "floating-point input array to retain nonfinite output, or correct nonfinite input values.",
                                                                                state.nonFiniteIndex(), DataTypeToString(GetDataType<T>())));
}

/**
 * @brief Adds Smoothing Recursive Gaussian conversion diagnostics while retaining the first error.
 * @tparam T Output pixel type used in the diagnostics.
 * @param state Conversion outcomes from the completed buffers.
 * @param result Diagnostics received before finalization.
 * @return The first error and all diagnostics collected by the smoothing pass.
 */
template <class T>
Result<> ReportSmoothingGaussianConversion(const TerminalConversionState& state, Result<> result)
{
  if(result.valid())
  {
    result = MergeResults(std::move(result), SmoothingGaussianNonFiniteResult<T>(state));
  }
  if(state.clipped())
  {
    result.warnings().push_back({.code = k_SmoothingGaussianClippedIntegerOutput,
                                 .message = fmt::format("Smoothing Recursive Gaussian clipped finite final integer values to the '{}' range after truncation. Review the "
                                                        "output before thresholding or segmentation; use a floating-point input to retain these values.",
                                                        DataTypeToString(GetDataType<T>()))});
  }
  return result;
}
} // namespace nx::core::ImageProcessing::detail
