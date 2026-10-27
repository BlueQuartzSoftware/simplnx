#pragma once

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <optional>
#include <type_traits>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief Specifies the maximum number of values in an elementwise staging chunk.
 * @tparam T Specifies the scalar value type.
 *
 * The 32 MiB target bounds each buffer while keeping bulk transfers large enough for out-of-core stores.
 */
template <class T>
inline constexpr usize k_ElementwiseChunkValues = std::max<usize>(usize{1}, (usize{32} << 20) / sizeof(T));

namespace detail
{
/**
 * @brief Applies an elementwise operation to a resident range in parallel.
 * @tparam T Specifies the scalar value type.
 * @tparam OperationT Specifies the operation for one index.
 * @param inputA First resident input buffer.
 * @param inputB Second resident input buffer. A unary operation can ignore this buffer.
 * @param output Resident output buffer.
 * @param count Number of values to process.
 * @param operation Applies the required arithmetic to one index.
 *
 * Worker threads use only raw resident pointers. Each index reads its operands before a possible aliased output write.
 */
template <class T, class OperationT>
void ApplyElementwiseResidentRange(const T* inputA, const T* inputB, T* output, usize count, OperationT operation)
{
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, count);
  parallelAlgorithm.execute([=](const Range& range) {
    const T* const inputAValuesPtr = inputA;
    const T* const inputBValuesPtr = inputB;
    T* const outputValuesPtr = output;
    const OperationT applyOperation = operation;
    for(usize index = range.min(); index < range.max(); ++index)
    {
      applyOperation(inputAValuesPtr, inputBValuesPtr, outputValuesPtr, index);
    }
  });
}

/**
 * @brief Subtracts two stores through bounded resident chunks.
 * @tparam T Specifies the scalar value type.
 * @param a First input store.
 * @param b Second input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation between chunks.
 * @param chunkValues Maximum number of values in each chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Serial bulk transfers isolate store access from parallel arithmetic. Both inputs are read before an aliased output write.
 * The arithmetic matches the resident path exactly.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 */
template <class T>
Result<> SubtractStoresChunked(const AbstractDataStore<T>& a, const AbstractDataStore<T>& b, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues)
{
  const usize size = a.getSize();
  chunkValues = std::max<usize>(usize{1}, chunkValues);
  auto bufferA = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  auto bufferB = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkValues, size - start);
    if(Result<> result = a.copyIntoBuffer(start, nonstd::span<T>(bufferA.get(), count)); result.invalid())
    {
      return result;
    }
    if(Result<> result = b.copyIntoBuffer(start, nonstd::span<T>(bufferB.get(), count)); result.invalid())
    {
      return result;
    }
    ApplyElementwiseResidentRange(bufferA.get(), bufferB.get(), bufferA.get(), count, [](const T* aValues, const T* bValues, T* outValues, usize index) {
      const T aValue = aValues[index];
      const T bValue = bValues[index];
      outValues[index] = static_cast<T>(aValue - bValue);
    });
    if(Result<> result = out.copyFromBuffer(start, nonstd::span<const T>(bufferA.get(), count)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

/**
 * @brief Inverts a binary store through bounded resident chunks.
 * @tparam T Specifies the scalar value type.
 * @param in Input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation between chunks.
 * @param chunkValues Maximum number of values in each chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Serial bulk transfers isolate store access from parallel arithmetic. Each input chunk is resident before an aliased output write.
 * The arithmetic matches the resident path exactly.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 */
template <class T>
Result<> InvertBinaryStoreChunked(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues)
{
  const usize size = in.getSize();
  chunkValues = std::max<usize>(usize{1}, chunkValues);
  auto buffer = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkValues, size - start);
    if(Result<> result = in.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); result.invalid())
    {
      return result;
    }
    ApplyElementwiseResidentRange(buffer.get(), buffer.get(), buffer.get(), count, [](const T* inValues, const T*, T* outValues, usize index) {
      const T inValue = inValues[index];
      outValues[index] = (inValue != T{0}) ? T{0} : T{1};
    });
    if(Result<> result = out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

} // namespace detail

/**
 * @brief Computes @c out[i] = a[i] - b[i] for equal-length scalar stores.
 * @tparam T Specifies the scalar value type.
 * @param a First input store.
 * @param b Second input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation before resident work or between chunks.
 * @param chunkValues Maximum number of values in each fallback chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Concrete in-memory stores use parallel resident spans. Other stores use bounded chunks with serial bulk transfers.
 * Both paths read each index before an aliased output write and use identical arithmetic.
 * Morphology callers order operands so valid neighborhoods produce nonnegative differences. A center-excluding kernel can have no in-bounds neighbor on a small image.
 * In that case, subtraction preserves ITK's extremum-seeded boundary result.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 * @pre The three stores have equal lengths.
 */
template <class T>
Result<> SubtractStores(const AbstractDataStore<T>& a, const AbstractDataStore<T>& b, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues = k_ElementwiseChunkValues<T>)
{
  const auto* residentAPtr = dynamic_cast<const DataStore<T>*>(&a);
  const auto* residentBPtr = dynamic_cast<const DataStore<T>*>(&b);
  auto* residentOutPtr = dynamic_cast<DataStore<T>*>(&out);
  if(residentAPtr != nullptr && residentBPtr != nullptr && residentOutPtr != nullptr)
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto aValues = residentAPtr->createSpan();
    const auto bValues = residentBPtr->createSpan();
    auto outValues = residentOutPtr->createSpan();
    detail::ApplyElementwiseResidentRange(aValues.data(), bValues.data(), outValues.data(), aValues.size(), [](const T* aData, const T* bData, T* outData, usize index) {
      const T aValue = aData[index];
      const T bValue = bData[index];
      outData[index] = static_cast<T>(aValue - bValue);
    });
    return {};
  }
  return detail::SubtractStoresChunked(a, b, out, shouldCancel, chunkValues);
}

/**
 * @brief Maps each nonzero input value to zero and each zero input value to one.
 * @tparam T Specifies the scalar value type.
 * @param in Input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation before resident work or between chunks.
 * @param chunkValues Maximum number of values in each fallback chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Concrete in-memory stores use parallel resident spans. Other stores use bounded chunks with serial bulk transfers.
 * Both paths read each index before an aliased output write and use identical arithmetic.
 * Signed Danielsson uses this operation to build the input for its second distance transform.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 * @pre The two stores have equal lengths.
 */
template <class T>
Result<> InvertBinaryStore(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues = k_ElementwiseChunkValues<T>)
{
  const auto* residentInPtr = dynamic_cast<const DataStore<T>*>(&in);
  auto* residentOutPtr = dynamic_cast<DataStore<T>*>(&out);
  if(residentInPtr != nullptr && residentOutPtr != nullptr)
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto inValues = residentInPtr->createSpan();
    auto outValues = residentOutPtr->createSpan();
    detail::ApplyElementwiseResidentRange(inValues.data(), inValues.data(), outValues.data(), inValues.size(), [](const T* inData, const T*, T* outData, usize index) {
      const T inValue = inData[index];
      outData[index] = (inValue != T{0}) ? T{0} : T{1};
    });
    return {};
  }
  return detail::InvertBinaryStoreChunked(in, out, shouldCancel, chunkValues);
}

/**
 * @brief In-place negates every value of a scalar store (out[i] = -out[i]) using bounded chunks.
 */
template <class T>
Result<> NegateStore(AbstractDataStore<T>& store, const std::atomic_bool& shouldCancel)
{
  const usize size = store.getSize();
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(std::min(k_ChunkValues, size));
  for(usize start = 0; start < size; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, size - start);
    if(Result<> r = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize i = 0; i < count; ++i)
    {
      buffer[i] = static_cast<T>(-buffer[i]);
    }
    if(Result<> r = store.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

namespace detail
{
/**
 * @brief Publishes a candidate index when it is smaller than the current index.
 * @param smallest Atomic index that receives the minimum published value.
 * @param candidate Index to compare with the current minimum.
 *
 * The compare-and-exchange loop makes the result independent of the order in which workers publish candidates.
 */
inline void PublishSmallerIndex(std::atomic<usize>& smallest, usize candidate) noexcept
{
  usize current = smallest.load(std::memory_order_relaxed);
  while(candidate < current && !smallest.compare_exchange_weak(current, candidate, std::memory_order_relaxed))
  {
  }
}

/**
 * @brief Finds the first value that does not match either binary value.
 * @tparam T Specifies the resident value type.
 * @param values Caller-owned resident values to compare.
 * @param fg Foreground value.
 * @param bg Background value.
 * @return Smallest invalid index, or no value when the span is empty or all values are binary.
 *
 * The parallel comparison is exact because an atomic minimum combines the first violation from each range.
 * A range that starts at or after the known minimum cannot produce a smaller index, so the worker safely skips it.
 * Worker threads read only the caller-owned resident span, so they do not access a data store.
 * Cancellation is checked by the caller between resident chunks, so a chunk that contains a violation always reports it.
 */
template <class T>
std::optional<usize> FindFirstNonBinaryValue(nonstd::span<const T> values, T fg, T bg)
{
  constexpr usize k_ScanBlockValues = 4096;
  std::atomic<usize> firstInvalid{values.size()};
  // The block OR-reduction has no data-dependent exit, so compilers can vectorize it.
  // The first-violation search runs only in the first block of each range that contains a violation.
  // The per-block check bounds unused work after another range publishes a smaller index.
  const T* const data = values.data();
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, values.size());
  parallelAlgorithm.execute([&](const Range& range) {
    const usize known = firstInvalid.load(std::memory_order_relaxed);
    if(range.min() >= known)
    {
      return;
    }
    // Local copies keep both reference values in registers; a by-reference capture is reloaded inside the loop.
    const T foreground = fg;
    const T background = bg;
    for(usize blockBegin = range.min(); blockBegin < range.max(); blockBegin += k_ScanBlockValues)
    {
      if(blockBegin >= firstInvalid.load(std::memory_order_relaxed))
      {
        return;
      }
      const usize blockEnd = std::min(range.max(), blockBegin + k_ScanBlockValues);
      uint32 invalidMask = 0;
      for(usize i = blockBegin; i < blockEnd; ++i)
      {
        invalidMask |= static_cast<uint32>(data[i] != foreground) & static_cast<uint32>(data[i] != background);
      }
      if(invalidMask != 0)
      {
        for(usize i = blockBegin; i < blockEnd; ++i)
        {
          if(data[i] != foreground && data[i] != background)
          {
            PublishSmallerIndex(firstInvalid, i);
            return;
          }
        }
      }
    }
  });

  const usize invalid = firstInvalid.load(std::memory_order_relaxed);
  if(invalid == values.size())
  {
    return std::nullopt;
  }
  return invalid;
}

/**
 * @brief Specifies the default number of resident values for a binary input scan.
 * @tparam T Specifies the input value type that determines the 64 MiB value count.
 */
template <class T>
inline constexpr usize k_BinaryScanChunkValues = std::max<usize>(usize{1}, (usize{64} << 20) / sizeof(T));

/**
 * @brief Validates that a binary morphology input contains only the foreground and background values.
 * @tparam T Specifies the input value type.
 * @param inStore Store that contains the original input values.
 * @param fg Foreground value that the binary morphology engine accepts.
 * @param bg Background value that the binary morphology engine accepts.
 * @param inputArrayPath Path included in a non-binary input error.
 * @param shouldCancel Shared cancellation flag.
 * @param chunkValues Requested maximum number of values in each resident chunk. Values less than one select one value.
 * @return Error for a store-read failure or the first non-binary value. A binary or cancelled scan returns a valid result.
 *
 * The engine emits a strict {foreground, background} image. ITK preserves each original non-foreground value, so equivalent behavior requires a strict binary input.
 * Store reads are serial and bounded. The function compares each resident chunk in parallel.
 * Standalone binary Dilate and Erode, binary Opening and Closing composites, and Binary Opening By Reconstruction scan the original input exactly once.
 * The callers do not scan engine-created intermediate stores because those stores contain only the foreground and background values.
 */
template <class T>
Result<> ScanBinaryInput(const AbstractDataStore<T>& inStore, T fg, T bg, const DataPath& inputArrayPath, const std::atomic_bool& shouldCancel, usize chunkValues = k_BinaryScanChunkValues<T>)
{
  const usize size = inStore.getSize();
  chunkValues = std::max<usize>(usize{1}, chunkValues);
  auto scanBuffer = std::make_unique<T[]>(std::min(chunkValues, size));
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkValues, size - start);
    if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(scanBuffer.get(), count)); r.invalid())
    {
      return r;
    }
    if(const std::optional<usize> invalid = FindFirstNonBinaryValue<T>(nonstd::span<const T>(scanBuffer.get(), count), fg, bg); invalid.has_value())
    {
      return MakeErrorResult(k_NonBinaryInput,
                             fmt::format("Binary morphology requires a binary image containing only the foreground ({}) or background ({}) value, but input array '{}' contains the value {} at "
                                         "index {}. Threshold or relabel the input first.",
                                         static_cast<int64>(fg), static_cast<int64>(bg), inputArrayPath.toString(), static_cast<int64>(scanBuffer[*invalid]), start + *invalid));
    }
  }
  return {};
}

/**
 * @brief Per-axis non-negative structuring-element radius (a negative radius means an empty axis; clamp to 0).
 *        This is exactly the pad/crop amount ITK's SafeBorder open/close uses (@c GetKernel().GetRadius()).
 */
inline std::array<usize, 3> SERadius(const StructuringElement& se)
{
  return {se.radius[0] > 0 ? static_cast<usize>(se.radius[0]) : 0, se.radius[1] > 0 ? static_cast<usize>(se.radius[1]) : 0, se.radius[2] > 0 ? static_cast<usize>(se.radius[2]) : 0};
}

/**
 * @brief The SafeBorder pad/crop amount per axis: @ref SERadius, but with the Z-axis pad forced to 0 when the image
 *        is 2D (@p dims[2] == 1). ITK's ONLY dimensional collapse is Z==1 => 2D (ITKArrayHelper.hpp: the
 *        tDims.getZ()==1 branch runs an itk::Image<T,2> with a 2D kernel), so its SafeBorder ConstantPad pads X and
 *        Y but never Z on a 2D image. Padding Z here would give it real depth that the structuring element's
 *        out-of-plane (dz!=0) offsets then read, diverging from ITK on 2D Ball/Cross SEs (the composite Safe Border
 *        defect this reproduces).
 *
 *        Only Z is clamped, mirroring ITK exactly. Do NOT generalize this to "any axis whose extent is 1": ITK does
 *        not collapse X or Y, so on an image ITK keeps 3D (dims[2] > 1) a size-1 X or Y axis is still padded by its
 *        SE radius -- clamping it would introduce a new divergence. The reported bug is only the Z axis of a 2D
 *        image, and that is all this clamps.
 */
inline std::array<usize, 3> SafeBorderPadRadius(const StructuringElement& se, const SizeVec3& dims)
{
  const std::array<usize, 3> r = SERadius(se);
  return {r[0], r[1], dims[2] == 1 ? usize{0} : r[2]};
}

/**
 * @brief Copies @p in (@p dims) into the interior of @p padded (dims + 2*@p r per axis), filling the
 *        @p r-thick border on every side with @p padValue. Streams one padded Z-plane at a time (bounded ==
 *        two planes), so it is identical in-core and out-of-core. This reproduces ITK's SafeBorder
 *        ConstantPadImageFilter step (pad by the kernel radius with the op's extremum) ahead of the two
 *        morphology passes.
 */
template <class T>
Result<> PadIntoStore(const AbstractDataStore<T>& in, AbstractDataStore<T>& padded, const SizeVec3& dims, const std::array<usize, 3>& r, T padValue, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * r[0];
  const usize pdimY = dimY + 2 * r[1];
  const usize pdimZ = dimZ + 2 * r[2];
  const usize inSlice = dimX * dimY;
  const usize pSlice = pdimX * pdimY;

  auto paddedPlane = std::make_unique<T[]>(pSlice);
  auto inPlane = std::make_unique<T[]>(inSlice);

  for(usize pz = 0; pz < pdimZ; ++pz)
  {
    if(shouldCancel)
    {
      return {};
    }
    // Border-fill the whole padded plane; interior rows are overwritten below when this plane maps to input.
    std::fill(paddedPlane.get(), paddedPlane.get() + pSlice, padValue);
    if(pz >= r[2] && pz < r[2] + dimZ)
    {
      const usize iz = pz - r[2];
      if(Result<> res = in.copyIntoBuffer(iz * inSlice, nonstd::span<T>(inPlane.get(), inSlice)); res.invalid())
      {
        return res;
      }
      for(usize iy = 0; iy < dimY; ++iy)
      {
        const usize py = iy + r[1];
        std::copy(inPlane.get() + iy * dimX, inPlane.get() + (iy + 1) * dimX, paddedPlane.get() + py * pdimX + r[0]);
      }
    }
    if(Result<> res = padded.copyFromBuffer(pz * pSlice, nonstd::span<const T>(paddedPlane.get(), pSlice)); res.invalid())
    {
      return res;
    }
  }
  return {};
}

/**
 * @brief Copies the original-extent interior (offset @p r on every side) of @p padded (dims + 2*@p r) back
 *        into @p out (@p dims). Streams one Z-plane at a time (bounded), identical in-core and OOC. This is
 *        ITK's SafeBorder CropImageFilter step (crop by the kernel radius) after the two morphology passes.
 */
template <class T>
Result<> CropFromStore(const AbstractDataStore<T>& padded, AbstractDataStore<T>& out, const SizeVec3& dims, const std::array<usize, 3>& r, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * r[0];
  const usize pdimY = dimY + 2 * r[1];
  const usize outSlice = dimX * dimY;
  const usize pSlice = pdimX * pdimY;

  auto paddedPlane = std::make_unique<T[]>(pSlice);
  auto outPlane = std::make_unique<T[]>(outSlice);

  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize pz = z + r[2];
    if(Result<> res = padded.copyIntoBuffer(pz * pSlice, nonstd::span<T>(paddedPlane.get(), pSlice)); res.invalid())
    {
      return res;
    }
    for(usize y = 0; y < dimY; ++y)
    {
      const usize py = y + r[1];
      std::copy(paddedPlane.get() + py * pdimX + r[0], paddedPlane.get() + py * pdimX + r[0] + dimX, outPlane.get() + y * dimX);
    }
    if(Result<> res = out.copyFromBuffer(z * outSlice, nonstd::span<const T>(outPlane.get(), outSlice)); res.invalid())
    {
      return res;
    }
  }
  return {};
}

/**
 * @brief Selects which operand appears first in @ref CropSubtractFromStore.
 */
enum class CropSubtractOrder
{
  OriginalMinusCropped,
  CroppedMinusOriginal
};

/// @brief Fuses the SafeBorder top-hat crop and subtraction.
/// @details Reads both source z-planes before writing the output z-plane.
/// Therefore, @p out may alias @p original safely.
/// Subtraction uses scalar @p T arithmetic and casts back to @p T.
/// @pre Every store has component shape {1}.
/// @pre Original and output tuple shapes are {dims[2], dims[1], dims[0]}.
/// @pre Padded has that tuple shape plus twice @p r on the corresponding axes.
/// @pre @p padded does not alias @p original or @p out.
template <class T>
Result<> CropSubtractFromStore(const AbstractDataStore<T>& padded, const AbstractDataStore<T>& original, AbstractDataStore<T>& out, const SizeVec3& dims, const std::array<usize, 3>& r,
                               CropSubtractOrder order, const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return {};
  }

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * r[0];
  const usize pdimY = dimY + 2 * r[1];
  const usize outSlice = dimX * dimY;
  const usize pSlice = pdimX * pdimY;

  auto paddedPlane = std::make_unique<T[]>(pSlice);
  auto originalPlane = std::make_unique<T[]>(outSlice);

  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize pz = z + r[2];
    if(Result<> res = padded.copyIntoBuffer(pz * pSlice, nonstd::span<T>(paddedPlane.get(), pSlice)); res.invalid())
    {
      return res;
    }
    if(Result<> res = original.copyIntoBuffer(z * outSlice, nonstd::span<T>(originalPlane.get(), outSlice)); res.invalid())
    {
      return res;
    }
    for(usize y = 0; y < dimY; ++y)
    {
      const usize py = y + r[1];
      const T* croppedRow = paddedPlane.get() + py * pdimX + r[0];
      T* outputRow = originalPlane.get() + y * dimX;
      if(order == CropSubtractOrder::OriginalMinusCropped)
      {
        for(usize x = 0; x < dimX; ++x)
        {
          outputRow[x] = static_cast<T>(outputRow[x] - croppedRow[x]);
        }
      }
      else
      {
        for(usize x = 0; x < dimX; ++x)
        {
          outputRow[x] = static_cast<T>(croppedRow[x] - outputRow[x]);
        }
      }
    }
    if(Result<> res = out.copyFromBuffer(z * outSlice, nonstd::span<const T>(originalPlane.get(), outSlice)); res.invalid())
    {
      return res;
    }
  }
  return {};
}

/**
 * @brief Build the reconstruction marker for @p kind into @p markerStore from @p inStore. Streams so peak memory
 *        stays bounded (a fixed chunk for the height markers, one z-plane for Fillhole), so an out-of-core input
 *        yields an out-of-core-bounded marker. @p dims is (X,Y,Z); @p height feeds MinusHeight/PlusHeight only.
 *
 * MinusHeight/PlusHeight use @ref SaturateCastFromDouble by default. The exactIntegerHeight opt-in preserves native
 * integer units with saturating addition or subtraction. Floating values retain the default arithmetic.
 *
 * The Fillhole/GrindPeak marker border is the boundary in ITK's EFFECTIVE dimensionality: the legacy filter
 * processes a (W,H,1) volume as a 2D ITK image (ITKArrayHelper selects Dimension 2 when Z==1), so X and Y are always
 * real axes (border at their extremes) while Z is a border axis only when it is not collapsed (dimZ>1). The border
 * carries the input value; the interior carries the global max (Fillhole, reconstruct-by-erosion → fills dark holes)
 * or the global min (GrindPeak, reconstruct-by-dilation → grinds bright peaks). This reproduces ITK's inset-region
 * rule, including "needle" geometries where a non-collapsed axis has extent 1 (every voxel is then a border voxel
 * and the image is left unchanged).
 *
 * @tparam T Scalar marker type.
 * @param inStore Original mask values.
 * @param markerStore Receives marker values; has the input's tuple and element layout.
 * @param dims Image dimensions in X, Y, Z order.
 * @param kind Selects a height or border marker.
 * @param height Finite offset in intensity units; nonnegative and whole for the integer opt-in.
 * @param shouldCancel Requests cancellation between transfers.
 * @param exactIntegerHeight Opt-in to exact, saturating integer offsets for a validated whole Height.
 * @return The first transfer or dimension error, or a valid result.
 * @pre @p markerStore has the same tuple/element layout as @p inStore (X fastest).
 */
template <class T>
Result<> BuildReconstructionMarker(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& markerStore, const SizeVec3& dims, ReconMarker kind, float64 height, const std::atomic_bool& shouldCancel,
                                   bool exactIntegerHeight = false)
{
  if(kind == ReconMarker::MinusHeight || kind == ReconMarker::PlusHeight)
  {
    const float64 signedHeight = (kind == ReconMarker::MinusHeight) ? -height : height;
    const usize total = inStore.getSize();
    const usize chunkValues = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || markerStore.getStoreType() == IDataStore::StoreType::OutOfCore ?
                                  std::max<usize>(1, (64ULL * 1024ULL * 1024ULL) / sizeof(T)) :
                                  65536;
    auto buffer = std::make_unique<T[]>(std::min(chunkValues, total));
    for(usize start = 0; start < total; start += chunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(chunkValues, total - start);
      if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
      if(exactIntegerHeight && std::is_integral_v<T>)
      {
        if constexpr(std::is_integral_v<T>)
        {
          ApplyExactIntegerReconstructionHeight<T>(nonstd::span<T>(buffer.get(), count), height, kind == ReconMarker::MinusHeight);
        }
      }
      else
      {
        for(usize i = 0; i < count; ++i)
        {
          buffer[i] = SaturateCastFromDouble<T>(static_cast<float64>(buffer[i]) + signedHeight);
        }
      }
      if(Result<> r = markerStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }

  // FillholeMaxBorder / GrindPeakMinBorder: the input value on the effective-dimensionality border, and a uniform
  // interior fill -- the global MAX for Fillhole (reconstruct-by-erosion fills dark holes), the global MIN for
  // GrindPeak (reconstruct-by-dilation grinds bright peaks). The border streaming below is identical for both.
  // Only min/max are consumed, so the parallel min/max reduction replaces the full statistics pass.
  Result<ArrayMinMax<T>> statsResult = ComputeArrayMinMax<T>(inStore, shouldCancel);
  if(statsResult.invalid())
  {
    return {nonstd::make_unexpected(std::move(statsResult.errors()))};
  }
  if(shouldCancel)
  {
    return {};
  }
  const T interiorFill = (kind == ReconMarker::FillholeMaxBorder) ? statsResult.value().max : statsResult.value().min;

  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  if(nX <= 0 || nY <= 0 || nZ <= 0)
  {
    return {};
  }
  const usize dimX = static_cast<usize>(nX);
  const usize dimY = static_cast<usize>(nY);
  const usize dimZ = static_cast<usize>(nZ);
  if(inStore.getStoreType() != IDataStore::StoreType::OutOfCore && markerStore.getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    const usize planeSize = dimX * dimY;
    auto inputPlane = std::make_unique<T[]>(planeSize);
    auto markerPlane = std::make_unique<T[]>(planeSize);
    for(usize z = 0; z < dimZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize planeStart = z * planeSize;
      if(Result<> result = inStore.copyIntoBuffer(planeStart, nonstd::span<T>(inputPlane.get(), planeSize)); result.invalid())
      {
        return result;
      }
      const bool zBorder = dimZ > 1 && (z == 0 || z + 1 == dimZ);
      for(usize y = 0; y < dimY; ++y)
      {
        const bool yBorder = y == 0 || y + 1 == dimY;
        for(usize x = 0; x < dimX; ++x)
        {
          const usize index = y * dimX + x;
          const bool border = x == 0 || x + 1 == dimX || yBorder || zBorder;
          markerPlane[index] = border ? inputPlane[index] : interiorFill;
        }
      }
      if(Result<> result = markerStore.copyFromBuffer(planeStart, nonstd::span<const T>(markerPlane.get(), planeSize)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }
  const usize total = inStore.getSize();
  constexpr usize k_TargetBytes = 64ULL * 1024ULL * 1024ULL;
  constexpr usize k_ChunkValues = std::max<usize>(1, k_TargetBytes / sizeof(T));
  auto buffer = std::make_unique<T[]>(std::min(k_ChunkValues, total));
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, total - start);
    if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize index = 0; index < count; ++index)
    {
      const usize globalIndex = start + index;
      const usize x = globalIndex % dimX;
      const usize yz = globalIndex / dimX;
      const usize y = yz % dimY;
      const usize z = yz / dimY;
      const bool zBorder = (dimZ > 1) && (z == 0 || z + 1 == dimZ);
      const bool yBorder = (y == 0 || y + 1 == dimY);
      const bool border = (x == 0 || x + 1 == dimX) || yBorder || zBorder;
      buffer[index] = border ? buffer[index] : interiorFill;
    }
    if(Result<> r = markerStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Fill an entire data store with a single value, streamed in bounded chunks (out-of-core-safe).
 */
template <class T>
Result<> FillStore(AbstractDataStore<T>& out, T value, const std::atomic_bool& shouldCancel)
{
  const usize total = out.getSize();
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(std::min(k_ChunkValues, total));
  std::fill(buffer.get(), buffer.get() + std::min(k_ChunkValues, total), value);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, total - start);
    if(Result<> r = out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

inline constexpr int32 k_RelabelComponentInsufficientWorkingMemory = -8591;

/**
 * @brief Describes a contiguous batch of complete output chunk slabs.
 */
struct RelabelChunkSlabPlan
{
  usize slabValues = 0;
  usize slabBytes = 0;
};

/**
 * @brief Multiplies two sizes when the product fits in `usize`.
 * @param left The first factor.
 * @param right The second factor.
 * @return The product. Returns empty when either factor is zero or the product overflows.
 */
inline std::optional<usize> RelabelCheckedMultiply(usize left, usize right)
{
  if(left == 0 || right == 0 || left > std::numeric_limits<usize>::max() / right)
  {
    return std::nullopt;
  }
  return left * right;
}

/**
 * @brief Creates an output chunk-slab plan for a three-dimensional array.
 * @param tupleShape The output tuple shape in Z, Y, X order.
 * @param outputChunkShape The optional output chunk shape in Z, Y, X order.
 * @param numValues The number of scalar output values.
 * @param valueBytes The size of one scalar output value in bytes.
 * @return A plan for valid shapes. Returns empty for invalid shapes or overflow.
 */
inline std::optional<RelabelChunkSlabPlan> CreateRelabelChunkSlabPlan(const ShapeType& tupleShape, const std::optional<ShapeType>& outputChunkShape, usize numValues, usize valueBytes)
{
  if(tupleShape.size() != 3 || !outputChunkShape.has_value() || outputChunkShape->size() != tupleShape.size() || valueBytes == 0)
  {
    return std::nullopt;
  }

  const ShapeType& chunkShape = *outputChunkShape;
  for(usize dimension = 0; dimension < tupleShape.size(); ++dimension)
  {
    if(tupleShape[dimension] == 0 || chunkShape[dimension] == 0 || chunkShape[dimension] > tupleShape[dimension])
    {
      return std::nullopt;
    }
  }

  const auto planeValues = RelabelCheckedMultiply(tupleShape[1], tupleShape[2]);
  const auto slabValues = planeValues.has_value() ? RelabelCheckedMultiply(chunkShape[0], *planeValues) : std::nullopt;
  const auto tupleCount = planeValues.has_value() ? RelabelCheckedMultiply(tupleShape[0], *planeValues) : std::nullopt;
  const auto slabBytes = slabValues.has_value() ? RelabelCheckedMultiply(*slabValues, valueBytes) : std::nullopt;
  if(!slabValues.has_value() || !tupleCount.has_value() || !slabBytes.has_value() || *tupleCount != numValues)
  {
    return std::nullopt;
  }
  return RelabelChunkSlabPlan{.slabValues = *slabValues, .slabBytes = *slabBytes};
}

/**
 * @brief Selects a bounded staging value count for RelabelComponent.
 *
 * Uses complete output slabs when the grant holds one. Otherwise, uses the largest bounded value count.
 * @param numValues The number of scalar input values.
 * @param grantedBytes The bytes granted for the staging buffer.
 * @param valueBytes The size of one scalar value in bytes.
 * @param slabPlan The optional output chunk-slab plan.
 * @return The number of values that fit in the staging buffer.
 */
inline usize SelectRelabelStagingValues(usize numValues, uint64 grantedBytes, usize valueBytes, const std::optional<RelabelChunkSlabPlan>& slabPlan)
{
  if(numValues == 0 || valueBytes == 0)
  {
    return 0;
  }

  const usize grantedValues = static_cast<usize>(std::min<uint64>(grantedBytes / valueBytes, std::numeric_limits<usize>::max()));
  const usize fallbackValues = std::min(numValues, grantedValues);
  if(!slabPlan.has_value() || grantedBytes < slabPlan->slabBytes)
  {
    return fallbackValues;
  }

  const usize fullSlabs = numValues / slabPlan->slabValues;
  const usize grantSlabs = static_cast<usize>(std::min<uint64>(grantedBytes / slabPlan->slabBytes, std::numeric_limits<usize>::max()));
  const usize stagingSlabs = std::min(fullSlabs, grantSlabs);
  return stagingSlabs == 0 ? fallbackValues : stagingSlabs * slabPlan->slabValues;
}

/**
 * @brief Subtracts HConvex reconstruction values in one owned resident range without signed overflow.
 * @tparam T Signed integer scalar type.
 * @param inputValuesPtr Original image values.
 * @param reconstructionValuesPtr Reconstruction values, each no greater than the corresponding input.
 * @param outputValuesPtr Contrast output; may alias the reconstruction buffer.
 * @param count Number of values in the range.
 * @param start Global tuple offset for diagnostics.
 * @param inputPath Original array path for diagnostics.
 * @return The first contrast overflow, or a valid result.
 */
template <class T>
Result<> SubtractHConvexContrastRange(const T* inputValuesPtr, const T* reconstructionValuesPtr, T* outputValuesPtr, usize count, usize start, const DataPath& inputPath)
{
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
  std::atomic<usize> firstInvalid{count};
  ApplyElementwiseResidentRange(inputValuesPtr, reconstructionValuesPtr, outputValuesPtr, count, [&firstInvalid](const T* inputPtr, const T* reconstructionPtr, T* outputPtr, usize index) {
    const T inputValue = inputPtr[index];
    const T reconstructionValue = reconstructionPtr[index];
    // max + a negative value is representable. Check before subtracting, including for int64 minima.
    if(reconstructionValue < 0 && inputValue > std::numeric_limits<T>::max() + reconstructionValue)
    {
      PublishSmallerIndex(firstInvalid, index);
      return;
    }
    outputPtr[index] = static_cast<T>(inputValue - reconstructionValue);
  });
  const usize invalid = firstInvalid.load(std::memory_order_relaxed);
  if(invalid != count)
  {
    return MakeErrorResult(-1874102, fmt::format("H Convex contrast at tuple {} of input array '{}' exceeds the {} output maximum {}. Reduce Height or explicitly convert the input to a wider or "
                                                 "floating-point type before this filter.",
                                                 start + invalid, inputPath.toString(), DataTypeToString(GetDataType<T>()), std::numeric_limits<T>::max()));
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
 * @return The first transfer or contrast error, with warnings from completed transfers.
 */
template <class T>
Result<> SubtractHConvexContrastChunked(const AbstractDataStore<T>& input, const AbstractDataStore<T>& reconstruction, AbstractDataStore<T>& output, const DataPath& inputPath,
                                        const std::atomic_bool& shouldCancel, usize chunkValues = k_ElementwiseChunkValues<T>)
{
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
  const usize size = input.getSize();
  chunkValues = std::max<usize>(1, chunkValues);
  auto inputBuffer = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  auto reconstructionBuffer = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  Result<> accumulatedResult;
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return accumulatedResult;
    }
    const usize count = std::min(chunkValues, size - start);
    accumulatedResult = MergeResults(std::move(accumulatedResult), input.copyIntoBuffer(start, nonstd::span<T>(inputBuffer.get(), count)));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    accumulatedResult = MergeResults(std::move(accumulatedResult), reconstruction.copyIntoBuffer(start, nonstd::span<T>(reconstructionBuffer.get(), count)));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    accumulatedResult = MergeResults(std::move(accumulatedResult), SubtractHConvexContrastRange(inputBuffer.get(), reconstructionBuffer.get(), inputBuffer.get(), count, start, inputPath));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
    accumulatedResult = MergeResults(std::move(accumulatedResult), output.copyFromBuffer(start, nonstd::span<const T>(inputBuffer.get(), count)));
    if(accumulatedResult.invalid())
    {
      return accumulatedResult;
    }
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
 * @return The first transfer or signed contrast error, with warnings from completed transfers.
 *
 * Unsigned and floating arrays retain the existing subtraction. Signed arrays check before each subtraction.
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
      return SubtractHConvexContrastRange(inputValues.data(), reconstructionValues.data(), outputValues.data(), inputValues.size(), 0, inputPath);
    }

    return SubtractHConvexContrastChunked<T>(input, reconstruction, output, inputPath, shouldCancel);
  }
}

} // namespace detail
} // namespace nx::core::ImageProcessing
