#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"

#include <fmt/format.h>

#include <atomic>
#include <string_view>
#include <utility>

namespace nx::core::ImageProcessing::detail
{
/**
 * @brief Retains completed conversion and transfer warnings when the public filter reports cancellation.
 * @param result Collected execution diagnostics.
 * @param shouldCancel Cancellation flag supplied by the caller.
 * @return Execution diagnostics including cancellation when previous warnings must be delivered.
 */
inline Result<> FinishCheckedImageExecution(Result<> result, const std::atomic_bool& shouldCancel)
{
  if(shouldCancel && result.valid() && !result.warnings().empty())
  {
    return MergeResults(std::move(result), MakeErrorResult(-1, "Filter cancelled"));
  }
  return result;
}

/**
 * @brief Formats up to one error and one warning after all terminal workers finish.
 * @param state Combined counts from final conversion.
 * @param result Transfer diagnostics already collected.
 * @param filterName Filter and optional output type to name in diagnostics.
 * @param nonFiniteCode Error code for a rejected integer conversion.
 * @param clippedCode Warning code for clipped integers.
 * @param nonFiniteAdvice Action to take after a rejected conversion.
 * @param clippingAdvice Action to take after clipping.
 * @return Transfer diagnostics with the conversion error or warning, if present.
 */
inline Result<> ReportTerminalImageConversion(const TerminalConversionState& state, Result<> result, std::string_view filterName, int32 nonFiniteCode, int32 clippedCode,
                                              std::string_view nonFiniteAdvice, std::string_view clippingAdvice)
{
  const auto partial = state.partial() ? " Partial count from the inspected output." : "";
  if(state.nonFiniteCount() != 0)
  {
    const auto location = state.nonFiniteCount() == 1 ? fmt::format(" at flat value index {}", state.nonFiniteIndex()) : "";
    Result<> error = MakeErrorResult(nonFiniteCode, fmt::format("{} computed {} nonfinite {}{} for integer output.{} {}", filterName, state.nonFiniteCount(),
                                                                state.nonFiniteCount() == 1 ? "value" : "values", location, partial, nonFiniteAdvice));
    result = MergeResults(std::move(result), std::move(error));
  }
  if(state.clippedCount() != 0)
  {
    result.warnings().push_back({.code = clippedCode,
                                 .message = fmt::format("{} clipped {} finite {} to the integer output range after truncation.{} {}", filterName, state.clippedCount(),
                                                        state.clippedCount() == 1 ? "value" : "values", partial, clippingAdvice)});
  }
  return result;
}

inline Result<> ReportCurvatureFlowConversion(const TerminalConversionState& state, Result<> result)
{
  return ReportTerminalImageConversion(state, std::move(result), "Curvature Flow", k_CurvatureFlowNonFiniteIntegerOutput, k_CurvatureFlowClippedIntegerOutput,
                                       "Reduce Time Step or use a floating-point input array to retain nonfinite results.",
                                       "The evolution may be unstable. Reduce Time Step and check downstream masks and segmentation for clipping plateaus.");
}

template <class T>
Result<> ReportSmoothingGaussianConversion(const TerminalConversionState& state, Result<> result)
{
  if(state.nonFiniteCount() == 0 && state.clippedCount() == 0)
  {
    return result;
  }
  const auto name = fmt::format("Smoothing Recursive Gaussian (output type '{}')", DataTypeToString(GetDataType<T>()));
  return ReportTerminalImageConversion(state, std::move(result), name, k_SmoothingGaussianNonFiniteIntegerOutput, k_SmoothingGaussianClippedIntegerOutput,
                                       "Use a floating-point input array to retain nonfinite results.",
                                       "Review the output before thresholding or segmentation. Use a floating-point input to retain these values.");
}
} // namespace nx::core::ImageProcessing::detail
