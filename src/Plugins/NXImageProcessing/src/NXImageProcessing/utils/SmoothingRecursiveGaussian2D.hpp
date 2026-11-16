#pragma once

#include "TerminalConversionDiagnostics.hpp"

#include "simplnx/Utilities/ImageProcessing/RecursiveGaussianEngine.hpp"

#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing::detail
{
/** @brief Smooths a 2D image with the provided work and checkpoint stores, then converts the output. */
template <class T, class WorkStoreT, class CheckpointStoreT>
Result<> ApplySmoothingRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, WorkStoreT& work, CheckpointStoreT& checkpoints, const SizeVec3& dims,
                                           const FloatVec3& spacing, const std::vector<float64>& sigma, bool normalizeAcrossScale, const detail::RecursiveGaussian2DBufferPlan& plan,
                                           const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  detail::TerminalConversionState conversionState;
  std::vector<T> outputBuffer;
  auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32>) -> Result<> {
    if constexpr(std::is_same_v<T, float32>)
    {
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(output, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()), true);
    }
    outputBuffer.resize(filtered.size());
    if(!detail::ConvertSmoothingGaussianBuffer<T>(nonstd::span<const float32>(filtered.data(), filtered.size()), nonstd::span<T>(outputBuffer.data(), outputBuffer.size()), (yBegin * dims[0]) + xBegin,
                                                  conversionState, shouldCancel, columnCount, dims[0]))
    {
      return detail::SmoothingGaussianNonFiniteResult<T>(conversionState);
    }
    return ImageProcessing::WriteRecursiveGaussian2DBlock<T>(output, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const T>(outputBuffer.data(), outputBuffer.size()), true);
  };
  const RecursiveGaussianPlaneAxis smoothY{
      .axis = 1, .sigma = sigma[1], .spacing = static_cast<double>(spacing[1]), .order = 0, .normalizeAcrossScale = normalizeAcrossScale, .CheckedSmoothing = true};
  const RecursiveGaussianPlaneAxis smoothX{
      .axis = 0, .sigma = sigma[0], .spacing = static_cast<double>(spacing[0]), .order = 0, .normalizeAcrossScale = normalizeAcrossScale, .CheckedSmoothing = true};
  Result<> result = RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, smoothY, smoothX, plan, shouldCancel, messageHandler, terminalSink);
  return detail::ReportSmoothingGaussianConversion<T>(conversionState, std::move(result));
}
} // namespace nx::core::ImageProcessing::detail
