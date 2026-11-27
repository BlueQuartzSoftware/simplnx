#pragma once

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageTransferResult.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

namespace nx::core::ImageProcessing::detail
{
/**
 * @brief Subtracts HConvex reconstruction values in one owned resident range without signed overflow.
 * @tparam T Signed integer scalar type.
 * @param inputValues Original image values.
 * @param reconstructionValues Reconstruction values, each no greater than the corresponding input.
 * @param outputValues Contrast output.
 * @param start Global tuple offset for diagnostics.
 * @param inputPath Original array path for diagnostics.
 * @pre All spans have the same size. Output is disjoint from each input or starts at the same element.
 * @return The first contrast overflow, or a valid result.
 */
template <class T>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- The two read-only spans have the same type but distinct subtraction roles.
Result<> SubtractHConvexContrastRange(nonstd::span<const T> inputValues, nonstd::span<const T> reconstructionValues, nonstd::span<T> outputValues, usize start, const DataPath& inputPath)
{
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
  const usize count = inputValues.size();
  std::atomic<usize> firstInvalid{count};
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, count);
  parallelAlgorithm.execute([=, &firstInvalid](const Range& range) {
    for(usize index = range.min(); index < range.max(); ++index)
    {
      const T inputValue = inputValues[index];
      const T reconstructionValue = reconstructionValues[index];
      // max + a negative value is representable. Check before subtracting, including for int64 minima.
      if(reconstructionValue < 0 && inputValue > std::numeric_limits<T>::max() + reconstructionValue)
      {
        PublishSmallerIndex(firstInvalid, index);
        continue;
      }
      outputValues[index] = static_cast<T>(inputValue - reconstructionValue);
    }
  });
  const usize invalid = firstInvalid.load(std::memory_order_relaxed);
  if(invalid != count)
  {
    constexpr int32 k_ContrastOverflowError = -29102;
    return MakeErrorResult(
        k_ContrastOverflowError,
        fmt::format(
            "H Convex contrast at tuple {} of input array '{}' exceeds the {} output maximum {:d} (input {:d}, reconstruction {:d}). Reduce Height or explicitly convert the input to a wider or "
            "floating-point type before this filter.",
            start + invalid, inputPath.toString(), DataTypeToString(GetDataType<T>()).view(), std::numeric_limits<T>::max(), inputValues[invalid], reconstructionValues[invalid]));
  }
  return {};
}

/**
 * @brief Computes signed HConvex contrast through bounded chunks with serial store transfers.
 * @tparam T Signed integer scalar type.
 * @param input Original image store.
 * @param reconstruction Reconstruction store.
 * @param output Contrast output; may alias reconstruction.
 * @param inputPath Original array path for diagnostics.
 * @param shouldCancel Requests cancellation between chunks.
 * @param chunkValues Maximum number of values in each owned buffer.
 * @pre All three stores have the same size.
 * @return The first transfer or contrast error, with warnings from completed transfers.
 */
template <class T>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- The input and reconstruction are distinct stores of the same element type.
Result<> SubtractHConvexContrastChunked(const AbstractDataStore<T>& input, const AbstractDataStore<T>& reconstruction, AbstractDataStore<T>& output, const DataPath& inputPath,
                                        const std::atomic_bool& shouldCancel, usize chunkValues = k_ElementwiseChunkValues<T>)
{
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
  const usize size = input.getSize();
  chunkValues = std::max<usize>(1, chunkValues);
  auto inputBuffer = std::make_unique<std::vector<T>>(std::min(chunkValues, size));
  auto reconstructionBuffer = std::make_unique<std::vector<T>>(std::min(chunkValues, size));
  Result<> accumulatedResult;
  for(usize start = 0; start < size;)
  {
    if(shouldCancel)
    {
      return accumulatedResult;
    }
    const usize count = std::min(chunkValues, size - start);
    AccumulateTransferResult(accumulatedResult, input.copyIntoBuffer(start, nonstd::span<T>(inputBuffer->data(), count)));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    AccumulateTransferResult(accumulatedResult, reconstruction.copyIntoBuffer(start, nonstd::span<T>(reconstructionBuffer->data(), count)));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    AccumulateTransferResult(accumulatedResult, SubtractHConvexContrastRange<T>(nonstd::span<const T>(inputBuffer->data(), count), nonstd::span<const T>(reconstructionBuffer->data(), count),
                                                                                nonstd::span<T>(inputBuffer->data(), count), start, inputPath));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    AccumulateTransferResult(accumulatedResult, output.copyFromBuffer(start, nonstd::span<const T>(inputBuffer->data(), count)));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    start += count;
  }
  return accumulatedResult;
}

/**
 * @brief Computes HConvex contrast, preserving the resident fast path and bounded serial store transfers.
 * @tparam T Numeric scalar type.
 * @param input Original image store.
 * @param reconstruction Reconstruction store.
 * @param output Contrast output; may alias reconstruction.
 * @param inputPath Original array path for diagnostics.
 * @param shouldCancel Requests cancellation before resident work or between chunks.
 * @pre All three stores have the same size.
 * @return The first transfer or signed contrast error, or a valid result.
 *
 * Signed integer contrast checks each subtraction and retains warnings from completed transfers.
 * Unsigned and floating arrays use the existing subtraction.
 * Completed chunks may already contain contrast when a later chunk fails; this operation does not promise atomic output.
 */
template <class T>
Result<> SubtractHConvexContrast(const AbstractDataStore<T>& input, const AbstractDataStore<T>& reconstruction, AbstractDataStore<T>& output, const DataPath& inputPath,
                                 const std::atomic_bool& shouldCancel)
{
  if constexpr(!std::is_integral_v<T> || !std::is_signed_v<T>)
  {
    return SubtractStores<T>(input, reconstruction, output, shouldCancel);
  }
  else
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto* residentInputPtr = dynamic_cast<const DataStore<T>*>(&input);
    const auto* residentReconstructionPtr = dynamic_cast<const DataStore<T>*>(&reconstruction);
    auto* residentOutputPtr = dynamic_cast<DataStore<T>*>(&output);
    if(residentInputPtr != nullptr && residentReconstructionPtr != nullptr && residentOutputPtr != nullptr)
    {
      const auto inputValues = residentInputPtr->createSpan();
      const auto reconstructionValues = residentReconstructionPtr->createSpan();
      auto outputValues = residentOutputPtr->createSpan();
      return SubtractHConvexContrastRange<T>(inputValues, reconstructionValues, outputValues, 0, inputPath);
    }

    return SubtractHConvexContrastChunked<T>(input, reconstruction, output, inputPath, shouldCancel);
  }
}
} // namespace nx::core::ImageProcessing::detail
