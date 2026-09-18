#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief The grayscale morphology operation to apply through a flat structuring element.
 *
 * Dilate folds neighbors by max; Erode folds by min. The boundary is handled by SKIPPING
 * out-of-bounds structuring-element neighbors, which is bit-identical to ITK 5.4.4's grayscale
 * morphology substituting a constant type extremum for out-of-range voxels (Dilate ->
 * std::numeric_limits<T>::lowest(), Erode -> std::numeric_limits<T>::max()): because the fold is
 * seeded with that same extremum, an out-of-range neighbor filled with the extremum can never win
 * the max/min and so is equivalent to dropping it.
 */
enum class MorphOp
{
  Dilate,
  Erode
};

namespace detail
{
enum class MorphologyCompositeOutputMode
{
  Morphology,
  OriginalMinusMorphology,
  MorphologyMinusOriginal
};

template <class T>
void SubtractMorphologyPlaneRows(const T* originalPlane, T* morphologyPlane, usize originalStride, usize outputWidth, usize cropX, usize cropY, MorphologyCompositeOutputMode outputMode, usize yBegin,
                                 usize yEnd)
{
  assert(outputMode != MorphologyCompositeOutputMode::Morphology);
  for(usize y = yBegin; y < yEnd; ++y)
  {
    const T* originalRow = originalPlane + (cropY + y) * originalStride + cropX;
    T* morphologyRow = morphologyPlane + y * outputWidth;
    for(usize x = 0; x < outputWidth; ++x)
    {
      const T originalValue = originalRow[x];
      morphologyRow[x] = outputMode == MorphologyCompositeOutputMode::OriginalMinusMorphology ? static_cast<T>(originalValue - morphologyRow[x]) : static_cast<T>(morphologyRow[x] - originalValue);
    }
  }
}

inline constexpr usize k_MorphologySlabTargetBytes = 16ULL * 1024ULL * 1024ULL;
// ITK-compatible Ball r{2,2,2} has 81 offsets; Ball r{3,3,3} has 179 and Box r{2,2,2} has 125.
inline constexpr usize k_MaxMorphologyResidentFlatFoldOffsets = 96;
// Bounds for the composite pipeline's read-ahead/write-behind plane-transfer batching (see
// ApplyMorphologyCompositePipeline). A minimum of one plane reproduces today's unbatched per-plane transfer.
// The preferred cap keeps a single staging buffer's footprint bounded -- 32 planes of a 1200x900 uint8 slice
// is about 34.6 MiB -- while comfortably clearing the OOC backend's four-chunk parallel-scatter gate.
inline constexpr usize k_CompositeStagingMinPlanes = 1;
inline constexpr usize k_CompositeStagingPreferredPlanes = 32;

template <class T>
constexpr T MorphologyGradientDifference(T maximum, T minimum) noexcept
{
  static_assert(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>);
  if constexpr(std::is_floating_point_v<T>)
  {
    return maximum - minimum;
  }
  else
  {
    using UnsignedT = std::make_unsigned_t<T>;
    const UnsignedT difference = static_cast<UnsignedT>(static_cast<UnsignedT>(maximum) - static_cast<UnsignedT>(minimum));
    if constexpr(std::is_signed_v<T>)
    {
      return std::bit_cast<T>(difference);
    }
    else
    {
      return difference;
    }
  }
}

inline bool ShouldUseMorphologyResidentFlatFold(const SizeVec3& dims, const StructuringElement& se)
{
  // The flat fold is faster for moderate neighborhoods, but its work grows with the full offset count.
  // Keep larger kernels on the moving-histogram path, whose work grows with the entering/leaving boundary.
  return dims[2] > 1 && !se.offsets.empty() && se.offsets.size() <= k_MaxMorphologyResidentFlatFoldOffsets;
}

inline std::array<usize, 3> MorphologySafeBorderPadRadius(const StructuringElement& se, const SizeVec3& dims)
{
  const std::array<usize, 3> radius = {se.radius[0] > 0 ? static_cast<usize>(se.radius[0]) : usize{0}, se.radius[1] > 0 ? static_cast<usize>(se.radius[1]) : usize{0},
                                       se.radius[2] > 0 ? static_cast<usize>(se.radius[2]) : usize{0}};
  return {radius[0], radius[1], dims[2] == 1 ? usize{0} : radius[2]};
}

inline usize MorphologyCompositePipelineRadiusZ(const StructuringElement& se, const SizeVec3& dims)
{
  return dims[2] == 1 ? usize{0} : (se.radius[2] > 0 ? static_cast<usize>(se.radius[2]) : usize{0});
}

struct MorphologyResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool TryMultiplyMorphologySize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

template <class T>
Result<usize> CalculateMorphologyResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }
  usize planeValues = 0;
  usize volumeValues = 0;
  usize volumeBytes = 0;
  usize requiredBytes = 0;
  if(!TryMultiplyMorphologySize(dims[0], dims[1], planeValues) || !TryMultiplyMorphologySize(planeValues, dims[2], volumeValues) || !TryMultiplyMorphologySize(volumeValues, sizeof(T), volumeBytes) ||
     !TryMultiplyMorphologySize(volumeBytes, usize{2}, requiredBytes))
  {
    return MakeErrorResult<usize>(-8656, fmt::format("Morphology dimensions ({}) overflow while sizing resident input and output working memory. Input element bytes: {}.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<MorphologyResidentMemoryAllocation> ReserveMorphologyResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateMorphologyResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<MorphologyResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {MorphologyResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

struct MorphologySlabPlan
{
  usize sliceValues = 0;
  usize volumeValues = 0;
  usize outputBatchDepth = 0;
  usize maxInputDepth = 0;
};

/**
 * @brief Validates the scalar volume and computes a bounded input/output slab plan.
 *
 * The target guides the combined input-halo and output-slab size; it is not a hard memory cap. At least
 *
 * one output plane is retained, and every batch must include its complete Z halo. A wide plane can therefore
 * exceed the target, and a radiusZ greater than or equal to the image depth requires a
 * full-depth input slab.
 */
template <class T>
Result<> MakeMorphologySlabPlan(const AbstractDataStore<T>& in, const AbstractDataStore<T>& out, const SizeVec3& dims, usize radiusZ, usize targetBytes, MorphologySlabPlan& plan)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    plan = {};
    return {};
  }

  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dims[0] > k_Int64Max || dims[1] > k_Int64Max || dims[2] > k_Int64Max)
  {
    return MakeErrorResult(-8650, fmt::format("Morphology image dimensions exceed the supported signed index range. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  if(!checkedMultiply(dims[0], dims[1], plan.sliceValues) || !checkedMultiply(plan.sliceValues, dims[2], plan.volumeValues))
  {
    return MakeErrorResult(-8651, fmt::format("Morphology image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(in.getSize() != plan.volumeValues)
  {
    return MakeErrorResult(
        -8652, fmt::format("Morphology input store size ({}) does not match the expected image volume ({}) for dimensions {} x {} x {}.", in.getSize(), plan.volumeValues, dims[0], dims[1], dims[2]));
  }
  if(out.getSize() != plan.volumeValues)
  {
    return MakeErrorResult(-8653, fmt::format("Morphology output store size ({}) does not match the expected image volume ({}) for dimensions {} x {} x {}.", out.getSize(), plan.volumeValues, dims[0],
                                              dims[1], dims[2]));
  }

  usize planeBytes = 0;
  if(!checkedMultiply(plan.sliceValues, sizeof(T), planeBytes))
  {
    return MakeErrorResult(-8654, fmt::format("Morphology image plane size overflows the addressable byte count. Plane dimensions: {} x {}; value size: {} bytes.", dims[0], dims[1], sizeof(T)));
  }

  const usize maxExtraHaloPlanes = dims[2] - 1;
  const usize haloPlanes = radiusZ > maxExtraHaloPlanes / 2 ? maxExtraHaloPlanes : std::min(maxExtraHaloPlanes, 2 * radiusZ);
  const usize planeBudget = planeBytes == 0 ? 0 : targetBytes / planeBytes;
  const bool holdsFullInputAndOutput = planeBudget / dims[2] >= 2;
  const usize maximumOutputBatchDepth = holdsFullInputAndOutput ? dims[2] : std::min(planeBudget > haloPlanes ? std::max<usize>(1, (planeBudget - haloPlanes) / 2) : 1, dims[2]);
  const usize batchCount = 1 + (dims[2] - 1) / maximumOutputBatchDepth;
  plan.outputBatchDepth = 1 + (dims[2] - 1) / batchCount;
  plan.maxInputDepth = plan.outputBatchDepth + std::min(haloPlanes, dims[2] - plan.outputBatchDepth);
  return {};
}

/**
 * @brief ParallelDataAlgorithm body that fills a batch of output Z-planes for BINARY morphology. Binary morphology
 *        is a pure predicate over the structuring-element neighborhood -- Dilate is
 * any() (fg if ANY neighbor is foreground), Erode is all() (fg only if ALL neighbors are foreground) -- so the reduce SHORT-CIRCUITS: Dilate stops at the first foreground neighbor, Erode at the first
 * non-foreground one. A neighbor is foreground iff it is in-bounds and equals @c fg, or out-of-bounds and
 *        @c boundaryToForeground. Membership is equality-based (never ordered), so the input may carry
 *        arbitrary labels: anything other than @c fg is treated as background, and the output is strictly
 *        @c {fg, bg}. This yields the identical any/all result as counting every offset, at less per-voxel
 *        work. Kept SEPARATE from the grayscale moving-histogram morphology path so no binary branch is ever
 *        added to the grayscale hot loop. Stateless and side-effect-free apart from writing distinct
 *        @c outPlane slots, so it is safe to run concurrently across the plane.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
struct BinaryMorphPlaneBody
{
  const T* slab;           // read-only slab covering Z in [zLo, zHi], laid out [slabZ][y][x]
  T* outSlab;              // output slab, outputDepth*dimX*dimY
  const SEOffset* offsets; // structuring-element offsets (dx,dy,dz)
  usize numOffsets;
  usize dimX;
  usize dimY;
  usize dimZ;
  usize zLo;    // slab's first Z index
  usize zBegin; // first output Z index
  bool dilate;
  T fg;
  T bg;
  bool boundaryToForeground;

  void operator()(const Range& range) const
  {
    const usize sliceValues = dimX * dimY;
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    const int64 dimZi = static_cast<int64>(dimZ);

    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize planeTuple = tuple % sliceValues;
      const usize x = planeTuple % dimX;
      const usize y = planeTuple / dimX;
      const usize z = zBegin + tuple / sliceValues;
      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);
      const int64 zi = static_cast<int64>(z);

      if(dilate)
      {
        // any(): stop at the first foreground neighbor.
        bool anyFg = false;
        for(usize k = 0; k < numOffsets && !anyFg; ++k)
        {
          const SEOffset& off = offsets[k];
          const int64 nx = xi + off.dx;
          const int64 ny = yi + off.dy;
          const int64 nz = zi + off.dz;
          if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
          {
            anyFg = boundaryToForeground; // out-of-image neighbor is foreground iff boundaryToForeground
            continue;
          }
          const usize slabZ = static_cast<usize>(nz) - zLo; // nz in [zLo, zHi] once in-bounds, since the slab spans the SE's full z-reach
          anyFg = (slab[slabZ * sliceValues + static_cast<usize>(ny) * dimX + static_cast<usize>(nx)] == fg);
        }
        outSlab[tuple] = anyFg ? fg : bg;
      }
      else
      {
        // all(): stop at the first non-foreground neighbor.
        bool allFg = true;
        for(usize k = 0; k < numOffsets && allFg; ++k)
        {
          const SEOffset& off = offsets[k];
          const int64 nx = xi + off.dx;
          const int64 ny = yi + off.dy;
          const int64 nz = zi + off.dz;
          if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
          {
            allFg = boundaryToForeground; // out-of-image neighbor is foreground iff boundaryToForeground
            continue;
          }
          const usize slabZ = static_cast<usize>(nz) - zLo; // nz in [zLo, zHi] once in-bounds, since the slab spans the SE's full z-reach
          allFg = (slab[slabZ * sliceValues + static_cast<usize>(ny) * dimX + static_cast<usize>(nx)] == fg);
        }
        outSlab[tuple] = allFg ? fg : bg;
      }
    }
  }
};

template <class T>
struct Morphology2DBlockBody
{
  const T* input;
  T* output;
  const SEOffset* offsets;
  usize numOffsets;
  usize dimX;
  usize dimY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;
  bool dilate;

  void operator()(const Range& range) const
  {
    const T identity = dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize localX = tuple % outputWidth;
      const usize localY = tuple / outputWidth;
      const usize x = outputXBegin + localX;
      const usize y = outputYBegin + localY;
      T value = identity;
      for(usize offsetIndex = 0; offsetIndex < numOffsets; ++offsetIndex)
      {
        const SEOffset& offset = offsets[offsetIndex];
        if(offset.dz != 0)
        {
          continue;
        }
        const int64 neighborX = static_cast<int64>(x) + offset.dx;
        const int64 neighborY = static_cast<int64>(y) + offset.dy;
        if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
        {
          continue;
        }
        const usize inputIndex = (static_cast<usize>(neighborY) - inputYBegin) * inputWidth + (static_cast<usize>(neighborX) - inputXBegin);
        const T neighbor = input[inputIndex];
        value = dilate ? std::max(value, neighbor) : std::min(value, neighbor);
      }
      output[tuple] = value;
    }
  }
};

/**
 * @brief Parallel body for a bounded true-2D morphological-gradient block. It folds each in-bounds
 * structuring-element neighbor into simultaneous maximum and minimum accumulators, then writes their typed
 * difference. Datastore transfers remain outside this body; workers read one immutable local input buffer and
 * write disjoint slots in one local output buffer.
 */
template <class T>
struct MorphGradient2DBlockBody
{
  const T* input;
  T* output;
  const SEOffset* offsets;
  usize numOffsets;
  usize dimX;
  usize dimY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;

  void operator()(const Range& range) const
  {
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize localX = tuple % outputWidth;
      const usize localY = tuple / outputWidth;
      const usize x = outputXBegin + localX;
      const usize y = outputYBegin + localY;
      T maxValue = std::numeric_limits<T>::lowest();
      T minValue = std::numeric_limits<T>::max();
      for(usize offsetIndex = 0; offsetIndex < numOffsets; ++offsetIndex)
      {
        const SEOffset& offset = offsets[offsetIndex];
        if(offset.dz != 0)
        {
          continue;
        }
        const int64 neighborX = static_cast<int64>(x) + offset.dx;
        const int64 neighborY = static_cast<int64>(y) + offset.dy;
        if(neighborX < 0 || neighborX >= dimXi || neighborY < 0 || neighborY >= dimYi)
        {
          continue;
        }
        const usize inputIndex = (static_cast<usize>(neighborY) - inputYBegin) * inputWidth + (static_cast<usize>(neighborX) - inputXBegin);
        const T neighbor = input[inputIndex];
        if(neighbor > maxValue)
        {
          maxValue = neighbor;
        }
        if(neighbor < minValue)
        {
          minValue = neighbor;
        }
      }
      output[tuple] = MorphologyGradientDifference(maxValue, minValue);
    }
  }
};

template <class T>
struct BinaryMorphology2DBlockBody
{
  const T* input;
  T* output;
  const SEOffset* offsets;
  usize numOffsets;
  usize dimX;
  usize dimY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;
  bool dilate;
  T foreground;
  T background;
  bool boundaryToForeground;

  void operator()(const Range& range) const
  {
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize localX = tuple % outputWidth;
      const usize localY = tuple / outputWidth;
      const usize x = outputXBegin + localX;
      const usize y = outputYBegin + localY;
      usize foregroundCount = 0;
      for(usize offsetIndex = 0; offsetIndex < numOffsets; ++offsetIndex)
      {
        const SEOffset& offset = offsets[offsetIndex];
        const int64 neighborX = static_cast<int64>(x) + offset.dx;
        const int64 neighborY = static_cast<int64>(y) + offset.dy;
        const bool inBounds = offset.dz == 0 && neighborX >= 0 && neighborX < static_cast<int64>(dimX) && neighborY >= 0 && neighborY < static_cast<int64>(dimY);
        if(!inBounds)
        {
          foregroundCount += boundaryToForeground ? 1 : 0;
          continue;
        }
        const usize inputIndex = (static_cast<usize>(neighborY) - inputYBegin) * inputWidth + (static_cast<usize>(neighborX) - inputXBegin);
        foregroundCount += input[inputIndex] == foreground ? 1 : 0;
      }
      output[tuple] = dilate ? (foregroundCount > 0 ? foreground : background) : (foregroundCount == numOffsets ? foreground : background);
    }
  }
};

inline bool CanUseFixedMorphologyInput2D(usize dimX, usize totalValues, usize inputCapacityValues, usize outputCapacityValues)
{
  return totalValues <= inputCapacityValues && dimX <= outputCapacityValues;
}

template <class T, class BodyFactory>
Result<> ExecuteBoundedMorphology2D(const AbstractDataStore<T>& inputStore, AbstractDataStore<T>& outputStore, const SizeVec3& dims, const StructuringElement& se, const std::atomic_bool& shouldCancel,
                                    usize requestedTargetBytes, BodyFactory&& bodyFactory)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize total = dimX * dimY;
  const usize targetBytes = requestedTargetBytes == k_MorphologySlabTargetBytes ? 64ULL * 1024ULL * 1024ULL : requestedTargetBytes;
  const usize targetValues = targetBytes / sizeof(T);
  if(targetValues == 0)
  {
    return MakeErrorResult(-8655, "The bounded 2-D morphology target cannot hold one value.");
  }

  if(se.offsets.empty())
  {
    const usize chunkValues = std::min(total, targetValues);
    auto buffer = std::make_unique<T[]>(chunkValues);
    for(usize start = 0; start < total; start += chunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(chunkValues, total - start);
      if(Result<> result = inputStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); result.invalid())
      {
        return result;
      }
      if(Result<> result = outputStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  const usize radiusX = se.radius[0] > 0 ? static_cast<usize>(se.radius[0]) : 0;
  const usize radiusY = se.radius[1] > 0 ? static_cast<usize>(se.radius[1]) : 0;
  if(radiusX > std::numeric_limits<usize>::max() / 2 || radiusY > std::numeric_limits<usize>::max() / 2)
  {
    return MakeErrorResult(-8655, "The bounded 2-D morphology halo dimensions overflow.");
  }
  const usize xHalo = 2 * radiusX;
  const usize yHalo = 2 * radiusY;

  constexpr usize k_FixedInputBytes = 48ULL * 1024ULL * 1024ULL;
  constexpr usize k_FixedOutputBytes = 16ULL * 1024ULL * 1024ULL;
  constexpr usize k_FixedInputValues = std::max<usize>(1, k_FixedInputBytes / sizeof(T));
  constexpr usize k_FixedOutputValues = std::max<usize>(1, k_FixedOutputBytes / sizeof(T));
  const bool useFixedInput = inputStore.getStoreType() == IDataStore::StoreType::OutOfCore && requestedTargetBytes == k_MorphologySlabTargetBytes &&
                             CanUseFixedMorphologyInput2D(dimX, total, k_FixedInputValues, k_FixedOutputValues);

  std::unique_ptr<T[]> fixedInput;
  if(useFixedInput)
  {
    fixedInput = std::make_unique<T[]>(k_FixedInputValues);
    if(Result<> result = inputStore.copyIntoBuffer(0, nonstd::span<T>(fixedInput.get(), total)); result.invalid())
    {
      return result;
    }
  }

  bool fullWidth = useFixedInput;
  usize coreRows = 0;
  usize coreColumns = dimX;
  if(useFixedInput)
  {
    coreRows = std::max<usize>(1, std::min(dimY, k_FixedOutputValues / dimX));
  }
  else
  {
    const usize rowCapacity = targetValues / dimX;
    if(rowCapacity > yHalo + 1)
    {
      coreRows = std::max<usize>(1, std::min(dimY, (rowCapacity - yHalo) / 2));
      fullWidth = coreRows > 0;
    }
    if(!fullWidth)
    {
      const usize inputRows = std::min(dimY, yHalo + 1);
      if(inputRows > std::numeric_limits<usize>::max() - 1 || (xHalo != 0 && inputRows > targetValues / xHalo))
      {
        return MakeErrorResult(-8655, "The bounded 2-D morphology tile halo exceeds the resident target.");
      }
      const usize haloValues = inputRows * xHalo;
      if(haloValues >= targetValues)
      {
        return MakeErrorResult(-8655, "The bounded 2-D morphology target cannot hold one output column and its halo.");
      }
      coreColumns = (targetValues - haloValues) / (inputRows + 1);
      if(coreColumns == 0)
      {
        return MakeErrorResult(-8655, "The bounded 2-D morphology target cannot hold one output column.");
      }
      coreColumns = std::min(dimX, coreColumns);
      coreRows = 1;
    }
  }

  if(fullWidth)
  {
    const std::optional<ShapeType> outputChunkShape = outputStore.getChunkShape();
    if(outputChunkShape.has_value() && outputChunkShape->size() >= 3 && (*outputChunkShape)[0] == 1 && (*outputChunkShape)[1] > 0 && (*outputChunkShape)[1] <= coreRows &&
       (*outputChunkShape)[2] == dimX)
    {
      coreRows = (coreRows / (*outputChunkShape)[1]) * (*outputChunkShape)[1];
    }
  }

  const usize maximumInputRows = fullWidth ? std::min(dimY, coreRows + yHalo) : std::min(dimY, yHalo + 1);
  const usize maximumInputColumns = fullWidth ? dimX : std::min(dimX, coreColumns + xHalo);
  auto inputBuffer = useFixedInput ? nullptr : std::make_unique<T[]>(maximumInputRows * maximumInputColumns);
  auto outputBuffer = std::make_unique<T[]>(coreRows * coreColumns);

  const usize yStep = fullWidth ? coreRows : 1;
  for(usize outputYBegin = 0; outputYBegin < dimY; outputYBegin += yStep)
  {
    const usize outputRows = std::min(coreRows, dimY - outputYBegin);
    const usize xStep = fullWidth ? dimX : coreColumns;
    for(usize outputXBegin = 0; outputXBegin < dimX; outputXBegin += xStep)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize outputColumns = std::min(coreColumns, dimX - outputXBegin);
      const usize outputXEnd = outputXBegin + outputColumns;
      const usize outputYEnd = outputYBegin + outputRows;
      const usize inputXBegin = useFixedInput ? 0 : (outputXBegin > radiusX ? outputXBegin - radiusX : 0);
      const usize inputXEnd = useFixedInput ? dimX : outputXEnd + std::min(radiusX, dimX - outputXEnd);
      const usize inputYBegin = useFixedInput ? 0 : (outputYBegin > radiusY ? outputYBegin - radiusY : 0);
      const usize inputYEnd = useFixedInput ? dimY : outputYEnd + std::min(radiusY, dimY - outputYEnd);
      const usize inputWidth = inputXEnd - inputXBegin;
      T* input = useFixedInput ? fixedInput.get() : inputBuffer.get();
      if(!useFixedInput)
      {
        for(usize inputY = inputYBegin; inputY < inputYEnd; ++inputY)
        {
          if(Result<> result = inputStore.copyIntoBuffer(inputY * dimX + inputXBegin, nonstd::span<T>(input + (inputY - inputYBegin) * inputWidth, inputWidth)); result.invalid())
          {
            return result;
          }
        }
      }

      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, outputRows * outputColumns);
      parallelAlgorithm.execute(bodyFactory(input, outputBuffer.get(), inputXBegin, inputYBegin, inputWidth, outputXBegin, outputYBegin, outputColumns));

      const usize outputOffset = outputYBegin * dimX + outputXBegin;
      const usize outputValues = fullWidth ? outputRows * dimX : outputColumns;
      if(fullWidth)
      {
        if(Result<> result = outputStore.copyFromBuffer(outputOffset, nonstd::span<const T>(outputBuffer.get(), outputValues)); result.invalid())
        {
          return result;
        }
      }
      else
      {
        if(Result<> result = outputStore.copyFromBuffer(outputOffset, nonstd::span<const T>(outputBuffer.get(), outputValues)); result.invalid())
        {
          return result;
        }
      }
    }
  }
  return {};
}

/**
 * @brief Compile-time selector for the moving-histogram representation, following ITK 5.4.4's split in
 *        itkMorphologyHistogram.h between the dense bin-count VectorMorphologyHistogram and the
 *        std::map-based MorphologyHistogram, but restricted to 8-bit integer voxels ((u)int8) only;
 *        everything wider (16-bit ints, int32/uint32/int64, float, double) uses the map.
 *
 * This MATCHES ITK's own split: itk::Function::MorphologyHistogram partial-specializes the dense vector
 * representation only for the 8-bit types (unsigned char / signed char / bool) and uses the std::map primary
 * template for everything wider. The map is independently the right choice for our copy-based traversal: we
 * copy a histogram per line/plane (histX = histY, histY = histZ), so a dense 16-bit vector would copy O(range)
 * each scanline (~512 KB/line, e.g. ~128 GB of copies on a 512^3 image), whereas the map's per-line copy is
 * bounded by the window's current distinct-value count (<= |SE|), not the type range. The choice is a pure
 * performance detail -- both representations return identical min/max, and the cross-validation gates it.
 */
template <class T>
inline constexpr bool k_UseVectorHistogram = std::is_integral_v<T> && (sizeof(T) == 1);

/**
 * @brief Map-based grayscale-morphology histogram (ITK's Function::MorphologyHistogram).
 *
 * Keys are ordered by @c std::greater for Dilate (so @c begin() is the running max) or @c std::less for
 * Erode (so @c begin() is the running min). Unlike ITK's lazy cleanup, @ref removePixel erases a key the
 * instant its count reaches zero, so the map only ever holds the window's CURRENTLY-present distinct
 * values (<= |SE|). This keeps memory, the O(log n) lookups, and -- critically for our copy-based
 * traversal -- the per-line histogram copies bounded by window cardinality rather than by all values
 * ever seen. @ref getValue is therefore a trivial (const) @c begin() read.
 *
 * @ref addBoundary / @ref removeBoundary fold the type extremum in/out so an out-of-image neighbor can
 * never beat a real one (see @ref MorphOp). Counts stay non-negative because the moving histogram only
 * ever removes a value it previously added.
 *
 * Floating-point NaN is skipped on both add and remove (via @c if @c constexpr, floating T only): a NaN
 * key would violate the strict-weak-ordering @c std::map requires (undefined behavior), and skipping it
 * exactly matches MorphScanline, whose @c v>acc / @c v<acc comparisons never let a NaN win. Infinities
 * are well-ordered and kept, so they win/lose identically to Scanline.
 *
 * @tparam T scalar voxel type. @tparam Dilate true -> max fold, false -> min fold.
 */
template <class T, bool Dilate>
class MorphMapHistogram
{
  using Compare = std::conditional_t<Dilate, std::greater<T>, std::less<T>>;

public:
  void setBoundary(T value)
  {
    m_Boundary = value;
  }
  void addPixel(T p)
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      if(std::isnan(p))
      {
        return;
      }
    }
    ++m_Map[p];
  }
  void removePixel(T p)
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      if(std::isnan(p))
      {
        return;
      }
    }
    // Invariant: p is present (a value is only removed after being added). Erase the key as soon as its
    // count hits zero so the map stays bounded by the window's live distinct-value count.
    auto it = m_Map.find(p);
    assert(it != m_Map.end() && "removePixel of a value not present in the histogram");
    if(--(it->second) == 0)
    {
      m_Map.erase(it);
    }
  }
  void addBoundary()
  {
    addPixel(m_Boundary);
  }
  void removeBoundary()
  {
    removePixel(m_Boundary);
  }
  T getValue() const
  {
    // An empty map means every neighbor was NaN or skipped; return the boundary extremum, which is exactly
    // the seed MorphScanline keeps when nothing wins (Dilate -> lowest(), Erode -> max()).
    if(m_Map.empty())
    {
      return m_Boundary;
    }
    return m_Map.begin()->first;
  }

private:
  std::map<T, usize, Compare> m_Map;
  T m_Boundary{};
};

/**
 * @brief Dense bin-count grayscale-morphology histogram for 8-bit integer types (ITK's
 *        VectorMorphologyHistogram). One counter per representable value (256 bins, so copies are cheap);
 *        @c m_CurrentValue tracks the running extremum, bumped up/down on add and walked toward
 *        @c m_InitValue on remove when its bin empties. @ref addBoundary / @ref removeBoundary fold the
 *        type extremum, which (being the fold identity) never displaces a real neighbor. Counts stay
 *        non-negative (removes are always balanced by a prior add), so the unsigned bins never underflow.
 *
 * @tparam T 8-bit integer voxel type. @tparam Dilate true -> max fold, false -> min fold.
 */
template <class T, bool Dilate>
class MorphVectorHistogram
{
public:
  MorphVectorHistogram()
  {
    const int64 lo = static_cast<int64>(std::numeric_limits<T>::lowest());
    const int64 hi = static_cast<int64>(std::numeric_limits<T>::max());
    m_Vector.assign(static_cast<usize>(hi - lo + 1), 0);
    if constexpr(Dilate)
    {
      m_InitValue = std::numeric_limits<T>::lowest();
      m_Direction = -1;
    }
    else
    {
      m_InitValue = std::numeric_limits<T>::max();
      m_Direction = 1;
    }
    m_CurrentValue = m_InitValue;
  }
  void setBoundary(T value)
  {
    m_Boundary = value;
  }
  void addPixel(T p)
  {
    ++m_Vector[index(p)];
    if constexpr(Dilate)
    {
      if(p > m_CurrentValue)
      {
        m_CurrentValue = p;
      }
    }
    else
    {
      if(p < m_CurrentValue)
      {
        m_CurrentValue = p;
      }
    }
  }
  void removePixel(T p)
  {
    --m_Vector[index(p)];
    // Walk the running extremum toward the init value until its bin is non-empty again. The
    // m_CurrentValue != m_InitValue guard both stops the walk at the type extremum and keeps the index in
    // range; the histogram is never empty (it always holds one entry per SE offset), so a non-zero bin
    // always exists at or before m_InitValue.
    while(m_Vector[index(m_CurrentValue)] == 0 && m_CurrentValue != m_InitValue)
    {
      m_CurrentValue = static_cast<T>(m_CurrentValue + m_Direction);
    }
  }
  void addBoundary()
  {
    addPixel(m_Boundary);
  }
  void removeBoundary()
  {
    removePixel(m_Boundary);
  }
  T getValue() const
  {
    return m_CurrentValue;
  }

private:
  static usize index(T p)
  {
    return static_cast<usize>(static_cast<int64>(p) - static_cast<int64>(std::numeric_limits<T>::lowest()));
  }

  std::vector<usize> m_Vector;
  T m_InitValue{};
  T m_CurrentValue{};
  T m_Boundary{};
  int m_Direction{1};
};

/**
 * @brief Selects the dense or map-based morphology histogram for @p T at compile time (see
 *        @ref k_UseVectorHistogram). The non-selected template is only named, never instantiated.
 */
template <class T, bool Dilate>
using MorphHistogram = std::conditional_t<k_UseVectorHistogram<T>, MorphVectorHistogram<T, Dilate>, MorphMapHistogram<T, Dilate>>;

/**
 * @brief Map-based FUSED gradient histogram: the both-extremes analogue of @ref MorphMapHistogram, tracking the
 *        window's running max AND min in a single ascending @c std::map so @ref getGradient returns
 *        @c max - min in ONE moving pass (replacing the former dilate + erode + subtract three-pass sequence).
 *
 * Keys are ordered ascending, so @c begin() is the running min and @c rbegin() the running max. As in
 * @ref MorphMapHistogram, @ref removePixel erases a key the instant its count reaches zero, so the map holds
 * only the window's CURRENTLY-present distinct values (<= |SE|), keeping memory and the per-line copies bounded
 * by window cardinality. Floating-point NaN is skipped on add/remove (a NaN key would break the map's
 * strict-weak-ordering), exactly as the Scanline gradient body's @c v>max / @c v<min comparisons never let a
 * NaN win either extreme.
 *
 * Boundary handling differs from @ref MorphMapHistogram by design: an out-of-image neighbor would fold in
 * @c lowest() on the max side and @c max() on the min side -- i.e. it can never win EITHER extreme -- so the
 * fused gradient simply omits it (@ref addBoundary / @ref removeBoundary are no-ops). The empty-window case (a
 * center-EXCLUDING SE, e.g. Annulus on a tiny image, with NO in-bounds neighbor) then leaves the map empty, and
 * @ref getGradient returns @c static_cast<T>(lowest() - max()) -- bit-identical to the old 2-pass dilate(lowest())
 * - erode(max()) at that voxel (see @ref SubtractStores' extremum-seed note). For a non-empty window this is
 * exactly @c dilate - erode: the same two extremes of the same neighborhood, the same single integer subtraction.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
class MorphGradientMapHistogram
{
public:
  void setBoundary(T /*value*/)
  {
  } // no-op: the fused gradient never folds the boundary extremum in (see class docs / getGradient()).
  void addPixel(T p)
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      if(std::isnan(p))
      {
        return;
      }
    }
    ++m_Map[p];
  }
  void removePixel(T p)
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      if(std::isnan(p))
      {
        return;
      }
    }
    // Invariant: p is present (a value is only removed after being added). Erase the key as soon as its count
    // hits zero so the map stays bounded by the window's live distinct-value count.
    auto it = m_Map.find(p);
    assert(it != m_Map.end() && "removePixel of a value not present in the histogram");
    if(--(it->second) == 0)
    {
      m_Map.erase(it);
    }
  }
  void addBoundary()
  {
  } // no-op: an out-of-image neighbor wins neither extreme (lowest() on max side, max() on min side).
  void removeBoundary()
  {
  }
  T getGradient() const
  {
    // Empty window (center-excluding SE with no in-bounds neighbor): reproduce the 2-pass dilate(lowest()) -
    // erode(max()) seed exactly. Otherwise the range is max (rbegin) - min (begin), one exact subtraction.
    if(m_Map.empty())
    {
      return MorphologyGradientDifference(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max());
    }
    return MorphologyGradientDifference(m_Map.rbegin()->first, m_Map.begin()->first);
  }

private:
  std::map<T, usize> m_Map; // ascending keys: begin()->first is the window min, rbegin()->first the window max
};

/**
 * @brief Dense bin-count FUSED gradient histogram for 8-bit integer types: the both-extremes analogue of
 *        @ref MorphVectorHistogram. One counter per representable value (256 bins), plus TWO running extrema --
 *        @c m_Max (walked DOWN toward lowest() as values leave) and @c m_Min (walked UP toward max()) -- so
 *        @ref getGradient returns @c max - min in a single moving pass. @c m_Count tracks the number of live
 *        (real, in-bounds) entries; @ref addBoundary / @ref removeBoundary are no-ops for the same reason as
 *        @ref MorphGradientMapHistogram (a boundary neighbor wins neither extreme). When @c m_Count returns to
 *        zero (empty window), the extrema reset to their seeds so @ref getGradient yields
 *        @c static_cast<T>(lowest() - max()), matching the 2-pass empty-window result bit-for-bit.
 *
 * @tparam T 8-bit integer voxel type.
 */
template <class T>
class MorphGradientVectorHistogram
{
public:
  MorphGradientVectorHistogram()
  {
    const int64 lo = static_cast<int64>(std::numeric_limits<T>::lowest());
    const int64 hi = static_cast<int64>(std::numeric_limits<T>::max());
    m_Vector.assign(static_cast<usize>(hi - lo + 1), 0);
    m_Max = std::numeric_limits<T>::lowest(); // running window max; walks DOWN toward lowest() as values leave
    m_Min = std::numeric_limits<T>::max();    // running window min; walks UP toward max() as values leave
  }
  void setBoundary(T /*value*/)
  {
  }
  void addPixel(T p)
  {
    ++m_Vector[index(p)];
    ++m_Count;
    if(p > m_Max)
    {
      m_Max = p;
    }
    if(p < m_Min)
    {
      m_Min = p;
    }
  }
  void removePixel(T p)
  {
    --m_Vector[index(p)];
    --m_Count;
    if(m_Count == 0)
    {
      // Window emptied (only reachable for a center-excluding SE with no in-bounds neighbor): reset both
      // running extrema to their seeds so getGradient() reproduces the empty-window 2-pass lowest() - max().
      m_Max = std::numeric_limits<T>::lowest();
      m_Min = std::numeric_limits<T>::max();
      return;
    }
    // Walk each running extremum toward its neighbor bin until non-empty: max DOWN toward lowest(), min UP
    // toward max(). m_Count > 0 guarantees a live bin exists between the two seeds, so neither walk overruns;
    // the != seed guards also keep the index in range (mirrors MorphVectorHistogram's single-extremum walk).
    while(m_Vector[index(m_Max)] == 0 && m_Max != std::numeric_limits<T>::lowest())
    {
      m_Max = static_cast<T>(m_Max - 1);
    }
    while(m_Vector[index(m_Min)] == 0 && m_Min != std::numeric_limits<T>::max())
    {
      m_Min = static_cast<T>(m_Min + 1);
    }
  }
  void addBoundary()
  {
  }
  void removeBoundary()
  {
  }
  T getGradient() const
  {
    if(m_Count == 0)
    {
      return MorphologyGradientDifference(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max());
    }
    return MorphologyGradientDifference(m_Max, m_Min);
  }

private:
  static usize index(T p)
  {
    return static_cast<usize>(static_cast<int64>(p) - static_cast<int64>(std::numeric_limits<T>::lowest()));
  }

  std::vector<usize> m_Vector;
  usize m_Count = 0;
  T m_Max{};
  T m_Min{};
};

/**
 * @brief Selects the dense or map-based FUSED gradient histogram for @p T at compile time (see
 *        @ref k_UseVectorHistogram). The non-selected template is only named, never instantiated.
 */
template <class T>
using MorphGradientHistogram = std::conditional_t<k_UseVectorHistogram<T>, MorphGradientVectorHistogram<T>, MorphGradientMapHistogram<T>>;

/**
 * @brief Binary morphology accumulator: tracks only the count of foreground voxels in the sliding window
 *        (window size is always |SE|). Same interface as the grayscale @ref MorphHistogram (addPixel /
 *        removePixel / addBoundary / removeBoundary / getValue) so BinaryMorphDirect can slide it
 *        exactly like the histogram, at O(1)/step with a single integer -- lighter than the full
 * histogram.
 *
 * getValue applies the pinned binary formula against the compile-time op: Dilate emits fg when ANY window
 * voxel is foreground (fgCount > 0), Erode emits fg only when ALL are (fgCount == window size). Membership
 * is equality-based, so non-{fg,bg} labels count as background and the output is strictly {fg, bg}.
 *
 * @tparam T scalar voxel type. @tparam Dilate true -> "any foreground", false -> "all foreground".
 */
template <class T, bool Dilate>
class FgCountAccumulator
{
public:
  FgCountAccumulator(T foreground, T background, bool boundaryToForeground, usize windowSize)
  : m_Fg(foreground)
  , m_Bg(background)
  , m_BoundaryIsFg(boundaryToForeground)
  , m_Window(windowSize)
  {
  }
  void addPixel(T p)
  {
    if(p == m_Fg)
    {
      ++m_FgCount;
    }
  }
  void removePixel(T p)
  {
    // The moving window only ever removes a value it previously added, so m_FgCount stays non-negative and
    // the usize counter never underflows (mirrors MorphVectorHistogram's balanced add/remove invariant).
    if(p == m_Fg)
    {
      --m_FgCount;
    }
  }
  void addBoundary()
  {
    if(m_BoundaryIsFg)
    {
      ++m_FgCount;
    }
  }
  void removeBoundary()
  {
    // Balanced by a prior addBoundary (see removePixel), so m_FgCount stays non-negative -- no underflow.
    if(m_BoundaryIsFg)
    {
      --m_FgCount;
    }
  }
  T getValue() const
  {
    if constexpr(Dilate)
    {
      return (m_FgCount > 0) ? m_Fg : m_Bg;
    }
    else
    {
      return (m_FgCount == m_Window) ? m_Fg : m_Bg;
    }
  }

private:
  T m_Fg;
  T m_Bg;
  bool m_BoundaryIsFg;
  usize m_Window;
  usize m_FgCount = 0;
};

/**
 * @brief Per-axis "entering" (added) and "leaving" (removed) offset sets for a +1 moving-window step,
 *        shared by the grayscale @ref MorphDirect and the binary BinaryMorphDirect Direct paths.
 */
struct AxisAddRemoveSets
{
  std::array<std::vector<SEOffset>, 3> added;   // per axis a: offsets that ENTER the window on a +1 step along a
  std::array<std::vector<SEOffset>, 3> removed; // per axis a: offsets that LEAVE the window on a +1 step along a
};

/**
 * @brief Compute, per axis, the "entering" (added) and "leaving" (removed) offset sets for a +1 translation
 *        of the structuring-element window. When the center moves +e_a, the voxels that newly enter the
 *        window are {o + e_a : o in SE, o + e_a not in SE} (added, read at the OLD center) and those that
 *        leave are {o : o in SE, o - e_a not in SE} (removed, also read at the OLD center) -- exactly ITK's
 *        m_AddedOffsets / m_RemovedOffsets for a unit translation (only the +1 direction is needed because
 *        the traversal never steps backward). The two sets have equal size, so the window always holds
 *        exactly |SE| entries.
 *
 * PURE setup helper, behavior-identical to the inline computation it replaces and called ONCE before the
 * moving-window traversal (never inside any per-voxel hot loop), so both Direct paths share it without any
 * runtime branch being added to their inner loops. Grayscale @ref MorphDirect and binary BinaryMorphDirect
 * both consume the identical add/remove sets.
 */
inline AxisAddRemoveSets ComputeAxisAddRemoveSets(const StructuringElement& se)
{
  std::set<std::array<int32, 3>> onSet;
  for(const SEOffset& o : se.offsets)
  {
    onSet.insert({o.dx, o.dy, o.dz});
  }
  auto contains = [&onSet](int32 x, int32 y, int32 z) { return onSet.find({x, y, z}) != onSet.end(); };

  AxisAddRemoveSets result;
  constexpr int32 axisStep[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  for(int a = 0; a < 3; ++a)
  {
    for(const SEOffset& o : se.offsets)
    {
      const int32 qx = o.dx + axisStep[a][0];
      const int32 qy = o.dy + axisStep[a][1];
      const int32 qz = o.dz + axisStep[a][2];
      if(!contains(qx, qy, qz))
      {
        result.added[a].push_back(SEOffset{qx, qy, qz});
      }
      const int32 px = o.dx - axisStep[a][0];
      const int32 py = o.dy - axisStep[a][1];
      const int32 pz = o.dz - axisStep[a][2];
      if(!contains(px, py, pz))
      {
        result.removed[a].push_back(o);
      }
    }
  }
  return result;
}

/**
 * @brief Chooses the per-plane Y-block row span for @ref RunMorphologySlabWindow / @ref
 *        RunMorphologyGradientSlabWindow's flattened (localZ, yBlock) work-item decomposition.
 *
 * Partitioning solely over a batch's output Z planes gives ParallelDataAlgorithm only @p outputDepth
 * independent work items; on a batch whose plane count is a small multiple of the hardware thread count
 * (the common OOC shape, since the slab planner bounds a batch's resident footprint), most workers finish
 * their one or two planes long before the rest, leaving a visible load-imbalance tail. Splitting each
 * plane's rows into Y blocks multiplies the item count without changing what any item computes, at the
 * cost of one extra O(|SE|) row-start seed per item (the same arbitrary-row-start seed @ref
 * RunMorphologyPlaneWindow already performs for its Y-only partition).
 *
 * The heuristic targets @c k_ItemsPerWorkerTarget work items per hardware thread. If the plane count alone
 * already reaches that, splitting further would only add seeding cost with no scheduling benefit, so K=1
 * is returned as dimY itself -- a single Y block spanning the whole plane, i.e. one work item per plane,
 * identical in scope to the pre-existing per-plane partition. Otherwise each plane is divided into just
 * enough Y blocks to reach the target, never more finely than one row per block.
 *
 * @param outputDepth the batch's output Z depth (its plane count before splitting).
 * @param dimY the image's full Y extent.
 * @return the row span of one Y block; @c dimY itself when K=1.
 */
inline usize MorphologySlabWindowYBlockRows(usize outputDepth, usize dimY)
{
  if(outputDepth == 0 || dimY == 0)
  {
    return std::max<usize>(1, dimY);
  }
  const usize hardwareWorkers = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  constexpr usize k_ItemsPerWorkerTarget = 4;
  const usize desiredItems = hardwareWorkers * k_ItemsPerWorkerTarget;
  if(outputDepth >= desiredItems)
  {
    return dimY; // K=1: one Y block spanning the whole plane, matching the partition this replaces.
  }
  const usize desiredBlocksPerPlane = std::min(dimY, (desiredItems + outputDepth - 1) / outputDepth); // ceil(desiredItems / outputDepth), capped at one row per block
  return (dimY + desiredBlocksPerPlane - 1) / desiredBlocksPerPlane;                                  // ceil(dimY / desiredBlocksPerPlane)
}

struct MorphologyFlatFoldOffset
{
  int64 dx = 0;
  int64 dy = 0;
  int64 dz = 0;
  int64 flatDelta = 0;
};

inline std::vector<MorphologyFlatFoldOffset> MakeMorphologyFlatFoldOffsets(const StructuringElement& se, usize dimX, usize dimY)
{
  const int64 sliceValues = static_cast<int64>(dimX * dimY);
  std::vector<MorphologyFlatFoldOffset> flatOffsets;
  flatOffsets.reserve(se.offsets.size());
  for(const SEOffset& offset : se.offsets)
  {
    flatOffsets.push_back({offset.dx, offset.dy, offset.dz, offset.dz * sliceValues + offset.dy * static_cast<int64>(dimX) + offset.dx});
  }
  return flatOffsets;
}

/**
 * @brief Returns the local X interval with complete radius-X neighborhoods.
 *
 * @param boundDimX The bound interval width.
 * @param outputOriginX The output interval origin in the bound interval.
 * @param outputWidth The output interval width.
 * @param radiusX The X neighborhood radius.
 * @pre The output interval is inside the bound interval.
 * @return The local half-open interior interval. Returns @c {0, 0} when no local X position is interior.
 */
inline std::pair<usize, usize> MorphologyInteriorXRange(usize boundDimX, usize outputOriginX, usize outputWidth, usize radiusX) noexcept
{
  const usize interiorBegin = outputOriginX < radiusX ? radiusX - outputOriginX : 0;
  const usize interiorEndGlobal = boundDimX > radiusX ? boundDimX - radiusX : 0;
  if(outputOriginX >= interiorEndGlobal)
  {
    return {0, 0};
  }

  const usize interiorEnd = std::min(outputWidth, interiorEndGlobal - outputOriginX);
  return interiorBegin < interiorEnd ? std::pair<usize, usize>{interiorBegin, interiorEnd} : std::pair<usize, usize>{0, 0};
}

/**
 * @brief Folds one rectangular output block from a staged full-plane Z slab.
 *
 * The caller stages every in-bounds neighbor that the output block needs. Interior rows fold each
 * precomputed offset across contiguous X values, so the compiler can use SIMD extrema operations.
 * The border path retains the exact checked scalar fold against the padded or image bounds.
 *
 * @param stagedInput full X/Y planes starting at @p stagedZLo.
 * @param output local [Z][Y][X] output block.
 * @param boundDims complete image or padded bounds.
 * @param stagedZLo first global or padded Z plane in @p stagedInput.
 * @param outputDims local output block dimensions.
 * @param outputOrigin output block origin in @p boundDims.
 * @param se flat structuring element.
 * @param offsets precomputed slab-relative offsets.
 * @param shouldCancel stops work before later output writes.
 * @tparam T scalar voxel type.
 * @tparam Dilate true for max; false for min.
 */
template <class T, bool Dilate>
void RunMorphologyFlatFoldBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, const SizeVec3& outputDims, const SizeVec3& outputOrigin, const StructuringElement& se,
                                const std::vector<MorphologyFlatFoldOffset>& offsets, const std::atomic_bool& shouldCancel)
{
  const usize sliceValues = boundDims[0] * boundDims[1];
  const int64 boundX = static_cast<int64>(boundDims[0]);
  const int64 boundY = static_cast<int64>(boundDims[1]);
  const int64 boundZ = static_cast<int64>(boundDims[2]);
  const int64 radiusX = std::max<int64>(0, se.radius[0]);
  const int64 radiusY = std::max<int64>(0, se.radius[1]);
  const int64 radiusZ = std::max<int64>(0, se.radius[2]);
  const T identity = Dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
  const usize outputRows = outputDims[1] * outputDims[2];

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, outputRows);
  parallelAlgorithm.execute([&](const Range& range) {
    for(usize outputRow = range.min(); outputRow < range.max(); ++outputRow)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = outputRow / outputDims[1];
      const usize localY = outputRow % outputDims[1];
      const int64 xBegin = static_cast<int64>(outputOrigin[0]);
      const int64 y = static_cast<int64>(outputOrigin[1] + localY);
      const int64 z = static_cast<int64>(outputOrigin[2] + localZ);
      const usize stagedRowBase = (static_cast<usize>(z) - stagedZLo) * sliceValues + static_cast<usize>(y) * boundDims[0] + static_cast<usize>(xBegin);
      const bool interiorYZ = y >= radiusY && y + radiusY < boundY && z >= radiusZ && z + radiusZ < boundZ;
      T* outputRowPtr = output + outputRow * outputDims[0];

      auto foldChecked = [&](usize localX) {
        const int64 x = xBegin + static_cast<int64>(localX);
        const usize stagedBase = stagedRowBase + localX;
        T value = identity;
        for(const MorphologyFlatFoldOffset& offset : offsets)
        {
          const int64 neighborX = x + offset.dx;
          const int64 neighborY = y + offset.dy;
          const int64 neighborZ = z + offset.dz;
          if(neighborX >= 0 && neighborX < boundX && neighborY >= 0 && neighborY < boundY && neighborZ >= 0 && neighborZ < boundZ)
          {
            const T neighbor = stagedInput[static_cast<usize>(static_cast<int64>(stagedBase) + offset.flatDelta)];
            value = Dilate ? std::max(value, neighbor) : std::min(value, neighbor);
          }
        }
        outputRowPtr[localX] = value;
      };

      if(!interiorYZ)
      {
        for(usize localX = 0; localX < outputDims[0]; ++localX)
        {
          foldChecked(localX);
        }
        continue;
      }

      const auto [interiorXBegin, interiorXEnd] = MorphologyInteriorXRange(boundDims[0], outputOrigin[0], outputDims[0], static_cast<usize>(radiusX));
      if(interiorXBegin == interiorXEnd)
      {
        for(usize localX = 0; localX < outputDims[0]; ++localX)
        {
          foldChecked(localX);
        }
        continue;
      }

      for(usize localX = 0; localX < interiorXBegin; ++localX)
      {
        foldChecked(localX);
      }
      std::fill(outputRowPtr + interiorXBegin, outputRowPtr + interiorXEnd, identity);
      for(const MorphologyFlatFoldOffset& offset : offsets)
      {
        const T* neighborRowPtr = stagedInput + static_cast<usize>(static_cast<int64>(stagedRowBase + interiorXBegin) + offset.flatDelta);
        for(usize localX = interiorXBegin; localX < interiorXEnd; ++localX)
        {
          outputRowPtr[localX] = Dilate ? std::max(outputRowPtr[localX], *neighborRowPtr) : std::min(outputRowPtr[localX], *neighborRowPtr);
          ++neighborRowPtr;
        }
      }
      for(usize localX = interiorXEnd; localX < outputDims[0]; ++localX)
      {
        foldChecked(localX);
      }
    }
  });
}

/**
 * @brief Folds one block after creating slab-relative offsets.
 *
 * The wrapper creates the offsets once for this call. It delegates to the offset-taking implementation.
 *
 * @param stagedInput Full X/Y planes starting at @p stagedZLo.
 * @param output Local [Z][Y][X] output block.
 * @param boundDims Complete image or padded bounds.
 * @param stagedZLo First global or padded Z plane in @p stagedInput.
 * @param outputDims Local output block dimensions.
 * @param outputOrigin Output block origin in @p boundDims.
 * @param se Flat structuring element.
 * @param shouldCancel Stops work before later output writes.
 * @tparam T Scalar voxel type.
 * @tparam Dilate True for max; false for min.
 */
template <class T, bool Dilate>
void RunMorphologyFlatFoldBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, const SizeVec3& outputDims, const SizeVec3& outputOrigin, const StructuringElement& se,
                                const std::atomic_bool& shouldCancel)
{
  const std::vector<MorphologyFlatFoldOffset> offsets = MakeMorphologyFlatFoldOffsets(se, boundDims[0], boundDims[1]);
  RunMorphologyFlatFoldBlock<T, Dilate>(stagedInput, output, boundDims, stagedZLo, outputDims, outputOrigin, se, offsets, shouldCancel);
}

/**
 * @brief Calculates a gradient block from a staged full-plane Z slab.
 *
 * Interior rows use separate contiguous X loops for the maximum and minimum, so the compiler can use
 * SIMD extrema operations. A reusable thread-local O(row) buffer holds the minimum values. The border
 * path uses the same checked scalar fold as grayscale morphology. The gradient preserves the integer
 * wrap contract through @ref MorphologyGradientDifference.
 *
 * @param stagedInput full X/Y planes starting at @p stagedZLo.
 * @param output local [Z][Y][X] output block.
 * @param boundDims complete image or padded bounds.
 * @param stagedZLo first global or padded Z plane in @p stagedInput.
 * @param outputDims local output block dimensions.
 * @param outputOrigin output block origin in @p boundDims.
 * @param se flat structuring element.
 * @param offsets precomputed slab-relative offsets.
 * @param shouldCancel stops work before later output writes.
 * @tparam T scalar voxel type.
 */
template <class T>
void RunMorphologyGradientFlatFoldBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, const SizeVec3& outputDims, const SizeVec3& outputOrigin,
                                        const StructuringElement& se, const std::vector<MorphologyFlatFoldOffset>& offsets, const std::atomic_bool& shouldCancel)
{
  const usize sliceValues = boundDims[0] * boundDims[1];
  const int64 boundX = static_cast<int64>(boundDims[0]);
  const int64 boundY = static_cast<int64>(boundDims[1]);
  const int64 boundZ = static_cast<int64>(boundDims[2]);
  const int64 radiusX = std::max<int64>(0, se.radius[0]);
  const int64 radiusY = std::max<int64>(0, se.radius[1]);
  const int64 radiusZ = std::max<int64>(0, se.radius[2]);
  const usize outputRows = outputDims[1] * outputDims[2];

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, outputRows);
  parallelAlgorithm.execute([&](const Range& range) {
    static thread_local std::vector<T> minimumValues;
    minimumValues.resize(outputDims[0]);
    for(usize outputRow = range.min(); outputRow < range.max(); ++outputRow)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = outputRow / outputDims[1];
      const usize localY = outputRow % outputDims[1];
      const int64 xBegin = static_cast<int64>(outputOrigin[0]);
      const int64 y = static_cast<int64>(outputOrigin[1] + localY);
      const int64 z = static_cast<int64>(outputOrigin[2] + localZ);
      const usize stagedRowBase = (static_cast<usize>(z) - stagedZLo) * sliceValues + static_cast<usize>(y) * boundDims[0] + static_cast<usize>(xBegin);
      const bool interiorYZ = y >= radiusY && y + radiusY < boundY && z >= radiusZ && z + radiusZ < boundZ;
      T* outputRowPtr = output + outputRow * outputDims[0];
      T* minimumRowPtr = minimumValues.data();

      auto foldChecked = [&](usize localX) {
        const int64 x = xBegin + static_cast<int64>(localX);
        const usize stagedBase = stagedRowBase + localX;
        T maximum = std::numeric_limits<T>::lowest();
        T minimum = std::numeric_limits<T>::max();
        for(const MorphologyFlatFoldOffset& offset : offsets)
        {
          const int64 neighborX = x + offset.dx;
          const int64 neighborY = y + offset.dy;
          const int64 neighborZ = z + offset.dz;
          if(neighborX >= 0 && neighborX < boundX && neighborY >= 0 && neighborY < boundY && neighborZ >= 0 && neighborZ < boundZ)
          {
            const T neighbor = stagedInput[static_cast<usize>(static_cast<int64>(stagedBase) + offset.flatDelta)];
            maximum = std::max(maximum, neighbor);
            minimum = std::min(minimum, neighbor);
          }
        }
        outputRowPtr[localX] = MorphologyGradientDifference(maximum, minimum);
      };

      if(!interiorYZ)
      {
        for(usize localX = 0; localX < outputDims[0]; ++localX)
        {
          foldChecked(localX);
        }
        continue;
      }

      const auto [interiorXBegin, interiorXEnd] = MorphologyInteriorXRange(boundDims[0], outputOrigin[0], outputDims[0], static_cast<usize>(radiusX));
      if(interiorXBegin == interiorXEnd)
      {
        for(usize localX = 0; localX < outputDims[0]; ++localX)
        {
          foldChecked(localX);
        }
        continue;
      }

      for(usize localX = 0; localX < interiorXBegin; ++localX)
      {
        foldChecked(localX);
      }
      const usize interiorWidth = interiorXEnd - interiorXBegin;
      T* outputInteriorPtr = outputRowPtr + interiorXBegin;
      T* minimumInteriorPtr = minimumRowPtr + interiorXBegin;
      std::fill(outputInteriorPtr, outputInteriorPtr + interiorWidth, std::numeric_limits<T>::lowest());
      std::fill(minimumInteriorPtr, minimumInteriorPtr + interiorWidth, std::numeric_limits<T>::max());
      for(const MorphologyFlatFoldOffset& offset : offsets)
      {
        const T* neighborRowPtr = stagedInput + static_cast<usize>(static_cast<int64>(stagedRowBase + interiorXBegin) + offset.flatDelta);
        for(usize localX = interiorXBegin; localX < interiorXEnd; ++localX)
        {
          outputRowPtr[localX] = std::max(outputRowPtr[localX], *neighborRowPtr);
          ++neighborRowPtr;
        }
        neighborRowPtr = stagedInput + static_cast<usize>(static_cast<int64>(stagedRowBase + interiorXBegin) + offset.flatDelta);
        T* minimumPtr = minimumInteriorPtr;
        const T* const minimumEnd = minimumInteriorPtr + interiorWidth;
        for(; minimumPtr != minimumEnd; ++minimumPtr, ++neighborRowPtr)
        {
          *minimumPtr = std::min(*minimumPtr, *neighborRowPtr);
        }
      }
      T* minimumPtr = minimumInteriorPtr;
      T* const outputInteriorEnd = outputInteriorPtr + interiorWidth;
      for(T* outputPtr = outputInteriorPtr; outputPtr != outputInteriorEnd; ++outputPtr, ++minimumPtr)
      {
        *outputPtr = MorphologyGradientDifference(*outputPtr, *minimumPtr);
      }
      for(usize localX = interiorXEnd; localX < outputDims[0]; ++localX)
      {
        foldChecked(localX);
      }
    }
  });
}

/**
 * @brief Calculates one gradient block after creating slab-relative offsets.
 *
 * The wrapper creates the offsets once for this call. It delegates to the offset-taking implementation.
 *
 * @param stagedInput Full X/Y planes starting at @p stagedZLo.
 * @param output Local [Z][Y][X] output block.
 * @param boundDims Complete image or padded bounds.
 * @param stagedZLo First global or padded Z plane in @p stagedInput.
 * @param outputDims Local output block dimensions.
 * @param outputOrigin Output block origin in @p boundDims.
 * @param se Flat structuring element.
 * @param shouldCancel Stops work before later output writes.
 * @tparam T Scalar voxel type.
 */
template <class T>
void RunMorphologyGradientFlatFoldBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, const SizeVec3& outputDims, const SizeVec3& outputOrigin,
                                        const StructuringElement& se, const std::atomic_bool& shouldCancel)
{
  const std::vector<MorphologyFlatFoldOffset> offsets = MakeMorphologyFlatFoldOffsets(se, boundDims[0], boundDims[1]);
  RunMorphologyGradientFlatFoldBlock(stagedInput, output, boundDims, stagedZLo, outputDims, outputOrigin, se, offsets, shouldCancel);
}

/**
 * @brief Folds a binary morphology block from a staged full-plane Z slab.
 *
 * Dilate stops at the first foreground neighbor. Erode stops at the first background neighbor.
 *
 * @param stagedInput full X/Y planes starting at @p stagedZLo.
 * @param output local [Z][Y][X] output block.
 * @param boundDims complete image or padded bounds.
 * @param stagedZLo first global or padded Z plane in @p stagedInput.
 * @param outputDims local output block dimensions.
 * @param outputOrigin output block origin in @p boundDims.
 * @param se flat structuring element.
 * @param foreground foreground value.
 * @param background output background value.
 * @param boundaryToForeground maps out-of-bounds neighbors to foreground.
 * @param offsets precomputed slab-relative offsets.
 * @param shouldCancel stops work before later output writes.
 * @tparam T scalar voxel type.
 * @tparam Dilate true for any; false for all.
 */
template <class T, bool Dilate>
void RunBinaryMorphologyFlatFoldBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, const SizeVec3& outputDims, const SizeVec3& outputOrigin,
                                      const StructuringElement& se, T foreground, T background, bool boundaryToForeground, const std::vector<MorphologyFlatFoldOffset>& offsets,
                                      const std::atomic_bool& shouldCancel)
{
  const usize sliceValues = boundDims[0] * boundDims[1];
  const int64 boundX = static_cast<int64>(boundDims[0]);
  const int64 boundY = static_cast<int64>(boundDims[1]);
  const int64 boundZ = static_cast<int64>(boundDims[2]);
  const int64 radiusX = std::max<int64>(0, se.radius[0]);
  const int64 radiusY = std::max<int64>(0, se.radius[1]);
  const int64 radiusZ = std::max<int64>(0, se.radius[2]);
  const usize outputRows = outputDims[1] * outputDims[2];

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, outputRows);
  parallelAlgorithm.execute([&](const Range& range) {
    for(usize outputRow = range.min(); outputRow < range.max(); ++outputRow)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = outputRow / outputDims[1];
      const usize localY = outputRow % outputDims[1];
      const int64 xBegin = static_cast<int64>(outputOrigin[0]);
      const int64 y = static_cast<int64>(outputOrigin[1] + localY);
      const int64 z = static_cast<int64>(outputOrigin[2] + localZ);
      const usize stagedRowBase = (static_cast<usize>(z) - stagedZLo) * sliceValues + static_cast<usize>(y) * boundDims[0] + static_cast<usize>(xBegin);
      const bool interiorYZ = y >= radiusY && y + radiusY < boundY && z >= radiusZ && z + radiusZ < boundZ;
      T* outputRowPtr = output + outputRow * outputDims[0];
      for(usize localX = 0; localX < outputDims[0]; ++localX)
      {
        const int64 x = xBegin + static_cast<int64>(localX);
        const usize stagedBase = stagedRowBase + localX;
        bool result = !Dilate;
        if(interiorYZ && x >= radiusX && x + radiusX < boundX)
        {
          for(const MorphologyFlatFoldOffset& offset : offsets)
          {
            const bool isForeground = stagedInput[static_cast<usize>(static_cast<int64>(stagedBase) + offset.flatDelta)] == foreground;
            if constexpr(Dilate)
            {
              if(isForeground)
              {
                result = true;
                break;
              }
            }
            else if(!isForeground)
            {
              result = false;
              break;
            }
          }
        }
        else
        {
          for(const MorphologyFlatFoldOffset& offset : offsets)
          {
            const int64 neighborX = x + offset.dx;
            const int64 neighborY = y + offset.dy;
            const int64 neighborZ = z + offset.dz;
            const bool inBounds = neighborX >= 0 && neighborX < boundX && neighborY >= 0 && neighborY < boundY && neighborZ >= 0 && neighborZ < boundZ;
            const bool isForeground = inBounds ? stagedInput[static_cast<usize>(static_cast<int64>(stagedBase) + offset.flatDelta)] == foreground : boundaryToForeground;
            if constexpr(Dilate)
            {
              if(isForeground)
              {
                result = true;
                break;
              }
            }
            else if(!isForeground)
            {
              result = false;
              break;
            }
          }
        }
        outputRowPtr[localX] = result ? foreground : background;
      }
    }
  });
}

/**
 * @brief Folds one binary block after creating slab-relative offsets.
 *
 * The wrapper creates the offsets once for this call. It delegates to the offset-taking implementation.
 *
 * @param stagedInput Full X/Y planes starting at @p stagedZLo.
 * @param output Local [Z][Y][X] output block.
 * @param boundDims Complete image or padded bounds.
 * @param stagedZLo First global or padded Z plane in @p stagedInput.
 * @param outputDims Local output block dimensions.
 * @param outputOrigin Output block origin in @p boundDims.
 * @param se Flat structuring element.
 * @param foreground Foreground value.
 * @param background Output background value.
 * @param boundaryToForeground Maps out-of-bounds neighbors to foreground.
 * @param shouldCancel Stops work before later output writes.
 * @tparam T Scalar voxel type.
 * @tparam Dilate True for any; false for all.
 */
template <class T, bool Dilate>
void RunBinaryMorphologyFlatFoldBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, const SizeVec3& outputDims, const SizeVec3& outputOrigin,
                                      const StructuringElement& se, T foreground, T background, bool boundaryToForeground, const std::atomic_bool& shouldCancel)
{
  const std::vector<MorphologyFlatFoldOffset> offsets = MakeMorphologyFlatFoldOffsets(se, boundDims[0], boundDims[1]);
  RunBinaryMorphologyFlatFoldBlock<T, Dilate>(stagedInput, output, boundDims, stagedZLo, outputDims, outputOrigin, se, foreground, background, boundaryToForeground, offsets, shouldCancel);
}

using BinaryMorphWord = uint64;

/**
 * @brief Describes the contiguous 64-bit layout for one packed binary X/Y plane.
 *
 * Each word stores X bits in least-significant-bit first order. The last word in each row uses
 * @c validLastWordMask to clear bits outside the image width.
 */
struct PackedBinaryPlaneLayout
{
  usize dimX = 0;
  usize dimY = 0;
  usize wordsPerRow = 0;
  usize planeWords = 0;
  BinaryMorphWord validLastWordMask = 0;
};

/**
 * @brief Provides a logical-Z view of circular packed binary planes.
 *
 * The view maps logical Z planes in [@c zLo, @c zLo + @c depth) through @c head. The backing
 * words stay owned by the associated @ref PackedBinaryRing.
 * @struct PackedBinaryPlanesView
 */
struct PackedBinaryPlanesView
{
  const BinaryMorphWord* words = nullptr;
  PackedBinaryPlaneLayout layout;
  usize zLo = 0;
  usize depth = 0;
  usize capacity = 0;
  usize head = 0;
};

/**
 * @brief Owns a fixed-size circular ring of complete packed binary planes.
 *
 * Callers append contiguous logical Z planes. A full ring reuses its oldest plane. The caller
 * overwrites every word in the returned plane before the next view reads that plane.
 * @class PackedBinaryRing
 */
class PackedBinaryRing
{
public:
  /**
   * @brief Creates a circular packed-plane ring.
   * @param layout Packed layout for each ring plane.
   * @param capacity Maximum number of complete planes.
   */
  PackedBinaryRing(const PackedBinaryPlaneLayout& layout, usize capacity)
  : m_Layout(layout)
  , m_Capacity(capacity)
  {
    assert(capacity > 0 && "Packed binary ring capacity must be positive");
    usize wordCount = 0;
    const bool hasValidSize = TryMultiplyMorphologySize(capacity, layout.planeWords, wordCount);
    assert(hasValidSize && "Packed binary ring size overflows");
    if(hasValidSize)
    {
      m_Words.resize(wordCount);
    }
  }

  /**
   * @brief Appends a writable plane at the next logical Z position.
   * @param logicalZ Logical Z plane to append.
   * @return Writable storage for one complete packed plane.
   */
  BinaryMorphWord* appendPlane(usize logicalZ)
  {
    assert(m_Capacity > 0);
    assert(m_Depth == 0 || logicalZ == m_ZLo + m_Depth);
    if(m_Depth == 0)
    {
      m_ZLo = logicalZ;
    }
    usize slot = 0;
    if(m_Depth == m_Capacity)
    {
      slot = m_Head;
      m_Head = (m_Head + 1) % m_Capacity;
      ++m_ZLo;
    }
    else
    {
      slot = (m_Head + m_Depth) % m_Capacity;
      ++m_Depth;
    }
    return m_Words.data() + slot * m_Layout.planeWords;
  }

  /**
   * @brief Returns the current logical-Z view of the ring.
   * @return View of the retained packed planes.
   */
  PackedBinaryPlanesView view() const
  {
    return {m_Words.data(), m_Layout, m_ZLo, m_Depth, m_Capacity, m_Head};
  }

private:
  PackedBinaryPlaneLayout m_Layout;
  std::vector<BinaryMorphWord> m_Words;
  usize m_Capacity = 0;
  usize m_ZLo = 0;
  usize m_Depth = 0;
  usize m_Head = 0;
};

/**
 * @brief Creates a packed binary layout for full X/Y image planes.
 *
 * @param boundDims Complete image or padded bounds.
 * @return Layout for one packed X/Y plane.
 */
inline PackedBinaryPlaneLayout MakePackedBinaryPlaneLayout(const SizeVec3& boundDims)
{
  constexpr usize k_WordBits = std::numeric_limits<BinaryMorphWord>::digits;
  PackedBinaryPlaneLayout layout;
  layout.dimX = boundDims[0];
  layout.dimY = boundDims[1];
  layout.wordsPerRow = boundDims[0] / k_WordBits + (boundDims[0] % k_WordBits == 0 ? 0 : 1);
  if(!TryMultiplyMorphologySize(layout.wordsPerRow, boundDims[1], layout.planeWords))
  {
    assert(false && "Packed binary plane dimensions overflow");
    return layout;
  }
  const usize tailBits = boundDims[0] % k_WordBits;
  layout.validLastWordMask = tailBits == 0 ? std::numeric_limits<BinaryMorphWord>::max() : (BinaryMorphWord{1} << tailBits) - BinaryMorphWord{1};
  return layout;
}

/**
 * @brief Packs equality membership from staged raw full planes into 64-bit X-row words.
 *
 * Independent rows pack in parallel. The function reads only the staged raw buffer.
 *
 * @param stagedInput Staged full X/Y raw planes.
 * @param packedOutput Packed storage for @p stagedDepth planes.
 * @param layout Packed full-plane layout.
 * @param stagedDepth Number of staged planes.
 * @param foreground Value that maps to a set membership bit.
 * @param shouldCancel Stops packing before later rows.
 * @tparam T Scalar voxel type.
 * @return True when all rows were packed.
 */
template <class T>
bool PackBinaryMorphologyStagedPlanes(const T* stagedInput, BinaryMorphWord* packedOutput, const PackedBinaryPlaneLayout& layout, usize stagedDepth, T foreground, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_WordBits = std::numeric_limits<BinaryMorphWord>::digits;
  const usize rawPlaneValues = layout.dimX * layout.dimY;
  usize packedRows = 0;
  if(!TryMultiplyMorphologySize(stagedDepth, layout.dimY, packedRows))
  {
    assert(false && "Packed binary row count overflows");
    return false;
  }
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, packedRows);
  parallelAlgorithm.execute([&](const Range& range) {
    for(usize packedRowIndex = range.min(); packedRowIndex < range.max(); ++packedRowIndex)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = packedRowIndex / layout.dimY;
      const usize y = packedRowIndex % layout.dimY;
      const T* rawRow = stagedInput + localZ * rawPlaneValues + y * layout.dimX;
      BinaryMorphWord* packedRow = packedOutput + localZ * layout.planeWords + y * layout.wordsPerRow;
      for(usize wordIndex = 0; wordIndex < layout.wordsPerRow; ++wordIndex)
      {
        const usize xBegin = wordIndex * k_WordBits;
        const usize count = std::min(k_WordBits, layout.dimX - xBegin);
        BinaryMorphWord word = 0;
        for(usize bit = 0; bit < count; ++bit)
        {
          if(rawRow[xBegin + bit] == foreground)
          {
            word |= BinaryMorphWord{1} << bit;
          }
        }
        packedRow[wordIndex] = word & (wordIndex + 1 == layout.wordsPerRow ? layout.validLastWordMask : std::numeric_limits<BinaryMorphWord>::max());
      }
    }
  });
  return !shouldCancel;
}

/**
 * @brief Extracts 64 membership bits from one logical packed X/Y/Z row.
 *
 * The returned bit @c i maps to global X @p xStart plus @c i. Image-boundary bits use
 * @p boundaryToForeground. An in-bounds row must be present in the packed-plane view.
 *
 * @param view Logical-Z view of packed input planes.
 * @param boundDims Complete image or padded bounds.
 * @param xStart Global X coordinate for returned bit zero.
 * @param y Global Y coordinate.
 * @param z Global Z coordinate.
 * @param boundaryToForeground Maps out-of-bounds coordinates to set bits.
 * @return Packed membership bits.
 */
inline BinaryMorphWord ExtractPackedBinaryWord(const PackedBinaryPlanesView& view, const SizeVec3& boundDims, int64 xStart, int64 y, int64 z, bool boundaryToForeground);

/**
 * @brief Extracts a packed word from contiguous staged packed planes.
 *
 * @param packedInput Packed planes for global Z [@p stagedZLo, @p stagedZLo + @p stagedDepth).
 * @param layout Packed full-plane layout.
 * @param boundDims Complete image or padded bounds.
 * @param stagedZLo First global or padded packed plane.
 * @param stagedDepth Number of packed planes.
 * @param xStart Global X coordinate for returned bit zero.
 * @param y Global Y coordinate.
 * @param z Global Z coordinate.
 * @param boundaryToForeground Maps out-of-bounds coordinates to set bits.
 * @return Packed membership bits.
 */
inline BinaryMorphWord ExtractPackedBinaryWord(const BinaryMorphWord* packedInput, const PackedBinaryPlaneLayout& layout, const SizeVec3& boundDims, usize stagedZLo, usize stagedDepth, int64 xStart,
                                               int64 y, int64 z, bool boundaryToForeground)
{
  return ExtractPackedBinaryWord({packedInput, layout, stagedZLo, stagedDepth, stagedDepth, 0}, boundDims, xStart, y, z, boundaryToForeground);
}

/**
 * @brief Extracts a packed word from a logical packed-plane view.
 *
 * @param view Logical-Z view of packed input planes.
 * @param boundDims Complete image or padded bounds.
 * @param xStart Global X coordinate for returned bit zero.
 * @param y Global Y coordinate.
 * @param z Global Z coordinate.
 * @param boundaryToForeground Maps out-of-bounds coordinates to set bits.
 * @return Packed membership bits.
 */
inline BinaryMorphWord ExtractPackedBinaryWord(const PackedBinaryPlanesView& view, const SizeVec3& boundDims, int64 xStart, int64 y, int64 z, bool boundaryToForeground)
{
  constexpr usize k_WordBits = std::numeric_limits<BinaryMorphWord>::digits;
  const BinaryMorphWord boundaryWord = boundaryToForeground ? std::numeric_limits<BinaryMorphWord>::max() : BinaryMorphWord{0};
  if(y < 0 || y >= static_cast<int64>(boundDims[1]) || z < 0 || z >= static_cast<int64>(boundDims[2]))
  {
    return boundaryWord;
  }
  if(z < static_cast<int64>(view.zLo) || z >= static_cast<int64>(view.zLo + view.depth))
  {
    assert(false && "Packed binary input does not contain a required in-bounds Z plane");
    return boundaryWord;
  }

  const int64 xEnd = xStart + static_cast<int64>(k_WordBits);
  const int64 inBoundsBegin = std::max<int64>(0, xStart);
  const int64 inBoundsEnd = std::min<int64>(static_cast<int64>(boundDims[0]), xEnd);
  if(inBoundsBegin >= inBoundsEnd)
  {
    return boundaryWord;
  }

  const PackedBinaryPlaneLayout& layout = view.layout;
  const usize firstX = static_cast<usize>(inBoundsBegin);
  const usize sourceWordIndex = firstX / k_WordBits;
  const usize sourceShift = firstX % k_WordBits;
  const usize logicalPlane = static_cast<usize>(z) - view.zLo;
  const usize slot = (view.head + logicalPlane) % view.capacity;
  const BinaryMorphWord* packedRow = view.words + slot * layout.planeWords + static_cast<usize>(y) * layout.wordsPerRow;
  BinaryMorphWord sourceBits = packedRow[sourceWordIndex] >> sourceShift;
  if(sourceShift != 0 && sourceWordIndex + 1 < layout.wordsPerRow)
  {
    sourceBits |= packedRow[sourceWordIndex + 1] << (k_WordBits - sourceShift);
  }

  const usize inBoundsCount = static_cast<usize>(inBoundsEnd - inBoundsBegin);
  const BinaryMorphWord inBoundsMask = inBoundsCount == k_WordBits ? std::numeric_limits<BinaryMorphWord>::max() : (BinaryMorphWord{1} << inBoundsCount) - BinaryMorphWord{1};
  const usize destinationShift = static_cast<usize>(inBoundsBegin - xStart);
  const BinaryMorphWord destinationMask = inBoundsMask << destinationShift;
  return (boundaryWord & ~destinationMask) | ((sourceBits & inBoundsMask) << destinationShift);
}

/**
 * @brief Folds all structuring-element neighbors for one packed output word.
 *
 * @param view Logical-Z view of packed input planes.
 * @param boundDims Complete image or padded bounds.
 * @param xStart Global X coordinate for returned bit zero.
 * @param y Global Y coordinate.
 * @param z Global Z coordinate.
 * @param se Nonempty flat structuring element.
 * @param boundaryToForeground Maps out-of-bounds neighbors to set bits.
 * @tparam Dilate True for any; false for all.
 * @return Folded packed membership word.
 */
template <bool Dilate>
BinaryMorphWord FoldPackedBinaryWord(const PackedBinaryPlanesView& view, const SizeVec3& boundDims, int64 xStart, int64 y, int64 z, const StructuringElement& se, bool boundaryToForeground)
{
  BinaryMorphWord folded = Dilate ? BinaryMorphWord{0} : std::numeric_limits<BinaryMorphWord>::max();
  for(const SEOffset& offset : se.offsets)
  {
    const BinaryMorphWord neighbor = ExtractPackedBinaryWord(view, boundDims, xStart + offset.dx, y + offset.dy, z + offset.dz, boundaryToForeground);
    if constexpr(Dilate)
    {
      folded |= neighbor;
      if(folded == std::numeric_limits<BinaryMorphWord>::max())
      {
        break;
      }
    }
    else
    {
      folded &= neighbor;
      if(folded == 0)
      {
        break;
      }
    }
  }
  return folded;
}

/**
 * @brief Folds a binary morphology block from already-packed staged full planes.
 *
 * @param view Logical-Z view of packed input planes.
 * @param output Local [Z][Y][X] output block.
 * @param boundDims Complete image or padded bounds.
 * @param outputDims Local output block dimensions.
 * @param outputOrigin Output block origin in @p boundDims.
 * @param se Nonempty flat structuring element.
 * @param foreground Output foreground value.
 * @param background Output background value.
 * @param boundaryToForeground Maps out-of-bounds neighbors to foreground.
 * @param shouldCancel Stops work before later output rows.
 * @return True when all requested output rows complete.
 * @tparam T Scalar voxel type.
 * @tparam Dilate True for any; false for all.
 */
template <class T, bool Dilate>
bool RunPrepackedBinaryMorphologyBlock(const PackedBinaryPlanesView& view, T* output, const SizeVec3& boundDims, const SizeVec3& outputDims, const SizeVec3& outputOrigin, const StructuringElement& se,
                                       T foreground, T background, bool boundaryToForeground, const std::atomic_bool& shouldCancel)
{
  assert(!se.offsets.empty());
  if(shouldCancel)
  {
    return false;
  }
  constexpr usize k_WordBits = std::numeric_limits<BinaryMorphWord>::digits;
  const usize outputWordsPerRow = outputDims[0] / k_WordBits + (outputDims[0] % k_WordBits == 0 ? 0 : 1);
  const usize outputRows = outputDims[1] * outputDims[2];
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, outputRows);
  parallelAlgorithm.execute([&](const Range& range) {
    for(usize outputRow = range.min(); outputRow < range.max(); ++outputRow)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = outputRow / outputDims[1];
      const usize localY = outputRow % outputDims[1];
      const int64 y = static_cast<int64>(outputOrigin[1] + localY);
      const int64 z = static_cast<int64>(outputOrigin[2] + localZ);
      T* outputRowPtr = output + outputRow * outputDims[0];
      for(usize outputWord = 0; outputWord < outputWordsPerRow; ++outputWord)
      {
        const usize localX = outputWord * k_WordBits;
        const int64 xStart = static_cast<int64>(outputOrigin[0] + localX);
        BinaryMorphWord folded = FoldPackedBinaryWord<Dilate>(view, boundDims, xStart, y, z, se, boundaryToForeground);
        const usize validBits = std::min(k_WordBits, outputDims[0] - localX);
        const BinaryMorphWord validMask = validBits == k_WordBits ? std::numeric_limits<BinaryMorphWord>::max() : (BinaryMorphWord{1} << validBits) - BinaryMorphWord{1};
        folded &= validMask;
        for(usize bit = 0; bit < validBits; ++bit)
        {
          outputRowPtr[localX + bit] = ((folded >> bit) & BinaryMorphWord{1}) != 0 ? foreground : background;
        }
      }
    }
  });
  return !shouldCancel;
}

/**
 * @brief Folds a binary morphology block from contiguous packed staged planes.
 *
 * @param packedInput Packed planes for global Z [@p stagedZLo, @p stagedZLo + @p stagedDepth).
 * @param output Local [Z][Y][X] output block.
 * @param boundDims Complete image or padded bounds.
 * @param stagedZLo First global or padded packed plane.
 * @param stagedDepth Number of packed planes.
 * @param outputDims Local output block dimensions.
 * @param outputOrigin Output block origin in @p boundDims.
 * @param se Nonempty flat structuring element.
 * @param foreground Output foreground value.
 * @param background Output background value.
 * @param boundaryToForeground Maps out-of-bounds neighbors to foreground.
 * @param shouldCancel Stops work before later output rows.
 * @return True when all requested output rows complete.
 * @tparam T Scalar voxel type.
 * @tparam Dilate True for any; false for all.
 */
template <class T, bool Dilate>
bool RunPrepackedBinaryMorphologyBlock(const BinaryMorphWord* packedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, usize stagedDepth, const SizeVec3& outputDims,
                                       const SizeVec3& outputOrigin, const StructuringElement& se, T foreground, T background, bool boundaryToForeground, const std::atomic_bool& shouldCancel)
{
  const PackedBinaryPlaneLayout layout = MakePackedBinaryPlaneLayout(boundDims);
  return RunPrepackedBinaryMorphologyBlock<T, Dilate>({packedInput, layout, stagedZLo, stagedDepth, stagedDepth, 0}, output, boundDims, outputDims, outputOrigin, se, foreground, background,
                                                      boundaryToForeground, shouldCancel);
}

/**
 * @brief Folds one complete packed output plane from a packed input view.
 *
 * @param inputView Logical-Z view of packed input planes.
 * @param packedOutput Writable complete packed output plane.
 * @param boundDims Complete image or padded bounds.
 * @param outputOrigin Full-plane origin in @p boundDims.
 * @param se Nonempty flat structuring element.
 * @param boundaryToForeground Maps out-of-bounds neighbors to set bits.
 * @param shouldCancel Stops work before later output rows.
 * @return True when all requested output rows complete.
 * @tparam Dilate True for any; false for all.
 */
template <bool Dilate>
bool RunPrepackedBinaryMorphologyPlane(const PackedBinaryPlanesView& inputView, BinaryMorphWord* packedOutput, const SizeVec3& boundDims, const SizeVec3& outputOrigin, const StructuringElement& se,
                                       bool boundaryToForeground, const std::atomic_bool& shouldCancel)
{
  assert(!se.offsets.empty());
  if(shouldCancel)
  {
    return false;
  }
  const PackedBinaryPlaneLayout& layout = inputView.layout;
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, layout.dimY);
  parallelAlgorithm.execute([&](const Range& range) {
    for(usize localY = range.min(); localY < range.max(); ++localY)
    {
      if(shouldCancel)
      {
        return;
      }
      BinaryMorphWord* outputRow = packedOutput + localY * layout.wordsPerRow;
      const int64 y = static_cast<int64>(outputOrigin[1] + localY);
      const int64 z = static_cast<int64>(outputOrigin[2]);
      for(usize wordIndex = 0; wordIndex < layout.wordsPerRow; ++wordIndex)
      {
        const int64 xStart = static_cast<int64>(outputOrigin[0] + wordIndex * std::numeric_limits<BinaryMorphWord>::digits);
        const BinaryMorphWord word = FoldPackedBinaryWord<Dilate>(inputView, boundDims, xStart, y, z, se, boundaryToForeground);
        outputRow[wordIndex] = word & (wordIndex + 1 == layout.wordsPerRow ? layout.validLastWordMask : std::numeric_limits<BinaryMorphWord>::max());
      }
    }
  });
  return !shouldCancel;
}

/**
 * @brief Packs one padded plane from an optional original raw plane.
 *
 * @param sourcePlane Original raw plane, or null for a padded Z plane.
 * @param packedOutput Writable complete packed padded plane.
 * @param layout Packed padded-plane layout.
 * @param sourceDims Original image dimensions.
 * @param cropOffset Padded-plane offset for the original image.
 * @param foreground Source value that maps to a set bit.
 * @param shouldCancel Stops work before later source rows.
 * @return True when all requested source rows complete.
 * @tparam T Scalar voxel type.
 */
template <class T>
bool PackBinaryPaddedPlane(const T* sourcePlane, BinaryMorphWord* packedOutput, const PackedBinaryPlaneLayout& layout, const SizeVec3& sourceDims, const std::array<usize, 3>& cropOffset, T foreground,
                           const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return false;
  }
  std::fill_n(packedOutput, layout.planeWords, BinaryMorphWord{0});
  if(sourcePlane == nullptr)
  {
    return !shouldCancel;
  }
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, sourceDims[1]);
  parallelAlgorithm.execute([&](const Range& range) {
    for(usize y = range.min(); y < range.max(); ++y)
    {
      if(shouldCancel)
      {
        return;
      }
      BinaryMorphWord* packedRow = packedOutput + (cropOffset[1] + y) * layout.wordsPerRow;
      const T* sourceRow = sourcePlane + y * sourceDims[0];
      for(usize x = 0; x < sourceDims[0]; ++x)
      {
        if(sourceRow[x] == foreground)
        {
          const usize paddedX = cropOffset[0] + x;
          packedRow[paddedX / std::numeric_limits<BinaryMorphWord>::digits] |= BinaryMorphWord{1} << (paddedX % std::numeric_limits<BinaryMorphWord>::digits);
        }
      }
      packedRow[layout.wordsPerRow - 1] &= layout.validLastWordMask;
    }
  });
  return !shouldCancel;
}

/**
 * @brief Packs staged raw planes and folds a binary morphology block.
 *
 * @param stagedInput Full X/Y raw planes starting at @p stagedZLo.
 * @param output Local [Z][Y][X] output block.
 * @param boundDims Complete image or padded bounds.
 * @param stagedZLo First global or padded Z plane in @p stagedInput.
 * @param stagedDepth Number of staged planes.
 * @param outputDims Local output block dimensions.
 * @param outputOrigin Output block origin in @p boundDims.
 * @param se Nonempty flat structuring element.
 * @param foreground Foreground value.
 * @param background Output background value.
 * @param boundaryToForeground Maps out-of-bounds neighbors to foreground.
 * @param shouldCancel Stops work before later output writes.
 * @return True when packing and all output rows complete.
 * @tparam T Scalar voxel type.
 * @tparam Dilate True for any; false for all.
 */
template <class T, bool Dilate>
bool RunPackedBinaryMorphologyBlock(const T* stagedInput, T* output, const SizeVec3& boundDims, usize stagedZLo, usize stagedDepth, const SizeVec3& outputDims, const SizeVec3& outputOrigin,
                                    const StructuringElement& se, T foreground, T background, bool boundaryToForeground, const std::atomic_bool& shouldCancel)
{
  const PackedBinaryPlaneLayout layout = MakePackedBinaryPlaneLayout(boundDims);
  usize packedWordCount = 0;
  if(!TryMultiplyMorphologySize(layout.planeWords, stagedDepth, packedWordCount))
  {
    assert(false && "Packed binary staged-plane dimensions overflow");
    return false;
  }
  std::vector<BinaryMorphWord> packedInput(packedWordCount);
  if(!PackBinaryMorphologyStagedPlanes(stagedInput, packedInput.data(), layout, stagedDepth, foreground, shouldCancel))
  {
    return false;
  }
  return RunPrepackedBinaryMorphologyBlock<T, Dilate>(packedInput.data(), output, boundDims, stagedZLo, stagedDepth, outputDims, outputOrigin, se, foreground, background, boundaryToForeground,
                                                      shouldCancel);
}

/**
 * @brief Runs the moving-histogram traversal @ref MorphDirect uses over an out-of-core batch's staged
 *        Z-slab, so each step folds in only the entering/leaving neighbors (@c O(|added|+|removed|)) instead
 *        of gathering the full structuring-element neighborhood per voxel. Both approaches fold the same
 *        in-bounds neighborhood by max (Dilate) or min (Erode), so their output is bit-identical.
 *
 * The slab holds every global Z plane the structuring element's window can reach while producing the
 * batch's output range @c [zBegin, zBegin + outputDepth) -- the SE's full Z halo, clamped only where that
 * halo would cross a true image Z face (see @ref MakeMorphologySlabPlan). A halo plane is therefore always
 * real image data: every window lookup here is tested against the IMAGE's global extent (@p dimX, @p dimY,
 * @p dimZ), exactly as @ref MorphDirect tests against the whole in-core volume, so only a genuine
 * image-face neighbor (a global x, y, or z outside the volume) folds in the boundary extremum via
 * addBoundary/removeBoundary -- a slab Z edge that is not also an image face is always interior and is
 * read straight out of the slab via the zLo-relative @c flat lookup.
 *
 * @ref ParallelDataAlgorithm partitions a FLATTENED (localZ, yBlock) index space of @c outputDepth times
 * the block count returned by @ref MorphologySlabWindowYBlockRows, rather than Z planes alone, so a batch
 * with few output planes still exposes enough independent work items to keep every hardware thread busy.
 * Each work item decodes its own (localZ, yBegin, yEnd), seeds a fresh row-start window at
 * (0, yBegin, zBegin + localZ), and snakes X fastest, Y next across only its own row span, writing only
 * its own disjoint @p outSlab region. This forgoes the Z-push carry-over a single worker's contiguous Z
 * range used to exploit between consecutive planes; that is an accepted cost, since seeding is O(|SE|) per
 * item and items are numerous, and it cannot change the output -- a moving histogram's extremum depends
 * only on which neighbors are currently in the window, never on how that window was built up.
 *
 * @param slab read-only slab covering global Z in [zLo, zLo + slabDepth), laid out [z - zLo][y][x].
 * @param outSlab this batch's output slab (outputDepth * dimX * dimY values), laid out [z - zBegin][y][x].
 * @param dimX the image's full X extent.
 * @param dimY the image's full Y extent.
 * @param dimZ the image's full Z extent (the global bound every boundary test is measured against).
 * @param zLo the slab's first global Z plane.
 * @param zBegin the batch's first global output Z plane.
 * @param outputDepth the batch's output Z depth.
 * @param se the structuring element; @c se.offsets must be non-empty (callers route the degenerate SE
 *        through a pass-through copy instead of this traversal).
 * @param axisSets the SE's per-axis entering/leaving offset sets (see @ref ComputeAxisAddRemoveSets),
 *        computed once by the caller and shared across every batch.
 * @param shouldCancel checked once per work item (once per Y block, at least as often as the former
 *        once-per-plane density) so a long batch responds to cancellation promptly; the caller re-checks
 *        before committing any batch to the output store.
 * @tparam T scalar voxel type. @tparam Dilate true -> max fold, false -> min fold.
 */
template <class T, bool Dilate>
void RunMorphologySlabWindow(const T* slab, T* outSlab, usize dimX, usize dimY, usize dimZ, usize zLo, usize zBegin, usize outputDepth, const StructuringElement& se, const AxisAddRemoveSets& axisSets,
                             const std::atomic_bool& shouldCancel)
{
  using Hist = MorphHistogram<T, Dilate>;
  const usize sliceValues = dimX * dimY;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  const int64 rX = (se.radius[0] > 0) ? se.radius[0] : 0;
  const int64 rY = (se.radius[1] > 0) ? se.radius[1] : 0;
  const int64 rZ = (se.radius[2] > 0) ? se.radius[2] : 0;
  const T boundary = Dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
  const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
  const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;

  // Reads a GLOBAL (x,y,z) out of the slab; valid whenever (x,y,z) is in-bounds, since the slab always
  // spans every global Z the window can reach for this batch (see class docs).
  auto flat = [slab, sliceValues, dimX, zLo](int64 x, int64 y, int64 z) -> T { return slab[(static_cast<usize>(z) - zLo) * sliceValues + static_cast<usize>(y) * dimX + static_cast<usize>(x)]; };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  // Slide the histogram from center (cx,cy,cz) by +1 along axis a -- identical to MorphDirect's push,
  // reading through the slab-relative `flat` instead of a whole-volume buffer.
  auto push = [&](Hist& h, int64 cx, int64 cy, int64 cz, int a) {
    const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
    if(fast)
    {
      for(const SEOffset& q : added[a])
      {
        h.addPixel(flat(cx + q.dx, cy + q.dy, cz + q.dz));
      }
      for(const SEOffset& o : removed[a])
      {
        h.removePixel(flat(cx + o.dx, cy + o.dy, cz + o.dz));
      }
    }
    else
    {
      for(const SEOffset& q : added[a])
      {
        const int64 x = cx + q.dx;
        const int64 y = cy + q.dy;
        const int64 z = cz + q.dz;
        if(inBounds(x, y, z))
        {
          h.addPixel(flat(x, y, z));
        }
        else
        {
          h.addBoundary();
        }
      }
      for(const SEOffset& o : removed[a])
      {
        const int64 x = cx + o.dx;
        const int64 y = cy + o.dy;
        const int64 z = cz + o.dz;
        if(inBounds(x, y, z))
        {
          h.removePixel(flat(x, y, z));
        }
        else
        {
          h.removeBoundary();
        }
      }
    }
  };

  // Seed a row-start window at (0, rowY, z) -- a one-time O(|SE|) gather so each (localZ, yBlock) work item
  // below builds its OWN starting histogram independently of every other item, at an arbitrary row start
  // rather than only a plane's first row (mirrors RunMorphologyPlaneWindow's row-start seeding).
  auto seedRowHist = [&](Hist& h, int64 rowY, int64 z) {
    h.setBoundary(boundary);
    for(const SEOffset& o : se.offsets)
    {
      const int64 x = o.dx;
      const int64 y = rowY + o.dy;
      const int64 zz = z + o.dz;
      if(inBounds(x, y, zz))
      {
        h.addPixel(flat(x, y, zz));
      }
      else
      {
        h.addBoundary();
      }
    }
  };

  const usize blockRows = MorphologySlabWindowYBlockRows(outputDepth, dimY);
  const usize blockCount = blockRows == 0 ? 0 : (dimY + blockRows - 1) / blockRows;
  const usize itemCount = outputDepth * blockCount;

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, itemCount);
  parallelAlgorithm.execute([&](const Range& itemRange) {
    // Per-item scratch: histY starts each work item from a freshly value-initialized histogram before
    // seeding (seedRowHist only ADDS the window's entries; the histogram types keep their contents
    // across setBoundary, so seeding a carried-over histogram would fold the previous item's final
    // window into this one). histX is reset by assignment each row.
    Hist histY;
    Hist histX;

    for(usize item = itemRange.min(); item < itemRange.max(); ++item)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = item / blockCount;
      const usize blockIndex = item % blockCount;
      const usize yBegin = blockIndex * blockRows;
      const usize yEnd = std::min(dimY, yBegin + blockRows);
      const int64 z = static_cast<int64>(zBegin + localZ);

      histY = Hist{};
      seedRowHist(histY, static_cast<int64>(yBegin), z);
      for(usize y = yBegin; y < yEnd; ++y)
      {
        if(y > yBegin)
        {
          push(histY, 0, static_cast<int64>(y) - 1, z, 1);
        }
        histX = histY;
        for(int64 x = 0; x < nX; ++x)
        {
          outSlab[localZ * sliceValues + y * dimX + static_cast<usize>(x)] = histX.getValue();
          if(x < nX - 1)
          {
            push(histX, x, static_cast<int64>(y), z, 0);
          }
        }
      }
    }
  });
}

/**
 * @brief The BINARY analogue of @ref RunMorphologySlabWindow: runs @ref BinaryMorphDirect's moving fg-count
 *        traversal over an out-of-core batch's staged Z-slab. Used by @ref BinaryMorphScanline in place of a
 *        full per-voxel structuring-element gather, folding only the neighbors that enter/leave the window
 *        per step instead of testing every SE offset at every voxel. Both approaches fold the identical
 *        in-bounds neighborhood via any()/all() (see @ref detail::FgCountAccumulator), so their output is
 *        bit-identical.
 *
 * Shares @ref RunMorphologySlabWindow's flattened (localZ, yBlock) work-item partition (see
 * @ref MorphologySlabWindowYBlockRows) and per-item independence: each item seeds its OWN row-start window at
 * an arbitrary (0, yBegin, z) rather than carrying one across items (see that function's docs for why the
 * partition is flattened this way and why forgoing Z-push carry-over cannot change the output). Unlike the
 * grayscale histogram -- whose non-trivial vector/map state is intentionally reused across items via
 * @c setBoundary plus a fresh add loop, since reallocating it every item would be wasteful -- @ref
 * detail::FgCountAccumulator has no default constructor and no capacity worth preserving (it is a single
 * integer), so each item's seed simply CONSTRUCTS a fresh {fg, bg, boundaryToForeground, windowSize}
 * accumulator: bit-identical to a reset, at no meaningful extra cost. Kept SEPARATE from the grayscale slab
 * window so no binary branch is ever added to its hot loop.
 *
 * @param slab read-only slab covering global Z in [zLo, zLo + slabDepth), laid out [z - zLo][y][x].
 * @param outSlab this batch's output slab (outputDepth * dimX * dimY values), laid out [z - zBegin][y][x].
 * @param dimX/dimY/dimZ the image's full extent (the global bound every boundary test is measured against).
 * @param zLo the slab's first global Z plane.
 * @param zBegin the batch's first global output Z plane.
 * @param outputDepth the batch's output Z depth.
 * @param se the structuring element; @c se.offsets must be non-empty (callers route the degenerate SE
 *        through a pass-through copy instead of this traversal).
 * @param axisSets the SE's per-axis entering/leaving offset sets (see @ref ComputeAxisAddRemoveSets),
 *        computed once by the caller and shared across every batch.
 * @param fg/bg/boundaryToForeground the binary morphology parameters (see @ref detail::FgCountAccumulator).
 * @param shouldCancel checked once per work item, mirroring @ref RunMorphologySlabWindow.
 * @tparam T scalar voxel type. @tparam Dilate true -> any() fold, false -> all() fold.
 */
template <class T, bool Dilate>
void RunBinaryMorphologySlabWindow(const T* slab, T* outSlab, usize dimX, usize dimY, usize dimZ, usize zLo, usize zBegin, usize outputDepth, const StructuringElement& se,
                                   const AxisAddRemoveSets& axisSets, T fg, T bg, bool boundaryToForeground, const std::atomic_bool& shouldCancel)
{
  using Acc = FgCountAccumulator<T, Dilate>;
  const usize windowSize = se.offsets.size();
  const usize sliceValues = dimX * dimY;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  const int64 rX = (se.radius[0] > 0) ? se.radius[0] : 0;
  const int64 rY = (se.radius[1] > 0) ? se.radius[1] : 0;
  const int64 rZ = (se.radius[2] > 0) ? se.radius[2] : 0;
  const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
  const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;

  // Reads a GLOBAL (x,y,z) out of the slab; valid whenever (x,y,z) is in-bounds (see RunMorphologySlabWindow).
  auto flat = [slab, sliceValues, dimX, zLo](int64 x, int64 y, int64 z) -> T { return slab[(static_cast<usize>(z) - zLo) * sliceValues + static_cast<usize>(y) * dimX + static_cast<usize>(x)]; };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  // Slide the accumulator from center (cx,cy,cz) by +1 along axis a -- identical shape to
  // RunMorphologySlabWindow's push, folding in/out via addPixel/removePixel/addBoundary/removeBoundary instead
  // of a min/max histogram update.
  auto push = [&](Acc& acc, int64 cx, int64 cy, int64 cz, int a) {
    const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
    if(fast)
    {
      for(const SEOffset& q : added[a])
      {
        acc.addPixel(flat(cx + q.dx, cy + q.dy, cz + q.dz));
      }
      for(const SEOffset& o : removed[a])
      {
        acc.removePixel(flat(cx + o.dx, cy + o.dy, cz + o.dz));
      }
    }
    else
    {
      for(const SEOffset& q : added[a])
      {
        const int64 x = cx + q.dx;
        const int64 y = cy + q.dy;
        const int64 z = cz + q.dz;
        if(inBounds(x, y, z))
        {
          acc.addPixel(flat(x, y, z));
        }
        else
        {
          acc.addBoundary();
        }
      }
      for(const SEOffset& o : removed[a])
      {
        const int64 x = cx + o.dx;
        const int64 y = cy + o.dy;
        const int64 z = cz + o.dz;
        if(inBounds(x, y, z))
        {
          acc.removePixel(flat(x, y, z));
        }
        else
        {
          acc.removeBoundary();
        }
      }
    }
  };

  // Seeds a fresh row-start window at (0, rowY, z) -- a one-time O(|SE|) gather into a NEWLY-CONSTRUCTED
  // accumulator (see class docs for why this constructs rather than resets a shared instance).
  auto seedRowAcc = [&](int64 rowY, int64 z) {
    Acc acc(fg, bg, boundaryToForeground, windowSize);
    for(const SEOffset& o : se.offsets)
    {
      const int64 x = o.dx;
      const int64 y = rowY + o.dy;
      const int64 zz = z + o.dz;
      if(inBounds(x, y, zz))
      {
        acc.addPixel(flat(x, y, zz));
      }
      else
      {
        acc.addBoundary();
      }
    }
    return acc;
  };

  const usize blockRows = MorphologySlabWindowYBlockRows(outputDepth, dimY);
  const usize blockCount = blockRows == 0 ? 0 : (dimY + blockRows - 1) / blockRows;
  const usize itemCount = outputDepth * blockCount;

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, itemCount);
  parallelAlgorithm.execute([&](const Range& itemRange) {
    for(usize item = itemRange.min(); item < itemRange.max(); ++item)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = item / blockCount;
      const usize blockIndex = item % blockCount;
      const usize yBegin = blockIndex * blockRows;
      const usize yEnd = std::min(dimY, yBegin + blockRows);
      const int64 z = static_cast<int64>(zBegin + localZ);

      Acc accY = seedRowAcc(static_cast<int64>(yBegin), z);
      for(usize y = yBegin; y < yEnd; ++y)
      {
        if(y > yBegin)
        {
          push(accY, 0, static_cast<int64>(y) - 1, z, 1);
        }
        Acc accX = accY;
        for(int64 x = 0; x < nX; ++x)
        {
          outSlab[localZ * sliceValues + y * dimX + static_cast<usize>(x)] = accX.getValue();
          if(x < nX - 1)
          {
            push(accX, x, static_cast<int64>(y), z, 0);
          }
        }
      }
    }
  });
}

/**
 * @brief Runs the same moving-histogram traversal as @ref RunMorphologySlabWindow, but for a SINGLE output
 *        Z-plane sliced out of a resident Z-ring rather than a multi-plane out-of-core batch, and
 *        parallelized across Y instead of Z. Used by @ref ApplyMorphologyCompositePipeline for both of its
 *        morphology stages: each stage keeps its whole SE Z-halo resident in a ring, so a ring advance ever
 *        only produces exactly ONE new output plane -- @ref RunMorphologySlabWindow's Z-parallel partition
 *        would leave every worker but one idle for that shape of work. Splitting across Y instead lets every
 *        worker seed its OWN row-start histogram (one O(|SE|) gather) at (@p originOffset[0], @p
 *        originOffset[1] + its first row, the plane's fixed center Z) and then slide it down Y (@ref
 *        axisSets axis 1) across its own rows and across X (axis 0) within each row, mirroring @ref
 *        RunMorphologySlabWindow's per-worker independence and per-step add/remove cost.
 *
 * @p originOffset translates every output-local (ox, oy, 0) into the neighborhood-center coordinate read
 * from @p slab and tested against [0, @p boundDimX) x [0, @p boundDimY) x [0, @p boundDimZ). An all-zero
 * offset with @p outputDimX / @p outputDimY equal to the bound dims is an IDENTITY mapping: the output
 * plane IS the padded plane, and the padded volume's own faces are the bounds an out-of-image neighbor is
 * tested against. A non-zero offset with a smaller @p outputDimX / @p outputDimY is a CROP mapping: an
 * output plane the size of the original (unpadded) image is read out of the padded volume at that offset,
 * still bounds-tested against the FULL padded extent before the crop is applied. Both mappings fold by max
 * (Dilate) or min (Erode) and substitute the fold's extremum for an out-of-bounds neighbor via
 * addBoundary/removeBoundary, which can never displace a real neighbor -- equivalent to skipping it (see
 * @ref MorphOp).
 *
 * @param slab read-only Z-ring covering padded Z in [@p slabZLo, @p slabZLo + ringDepth), laid out
 *        [z - slabZLo][y][x] over a @p boundDimX * @p boundDimY plane stride -- the ring's OWN stride,
 *        shared by both @ref ApplyMorphologyCompositePipeline stages regardless of @p outputDimX / @p
 *        outputDimY.
 * @param outPlane this call's output plane (@p outputDimX * @p outputDimY values), laid out [oy][ox].
 * @param boundDimX/boundDimY/boundDimZ the padded volume's extent: what every neighbor is tested against.
 * @param slabZLo the ring's first resident padded Z plane.
 * @param outputDimX/outputDimY the output plane's extent (== the bound dims for the identity mapping; the
 *        original image's dims for the crop mapping).
 * @param originOffset added to every output-local (ox, oy, 0) to get the padded-space neighborhood center.
 * @param outputZ added to @p originOffset[2] to get the fixed padded-space center Z for the whole plane.
 * @param se the structuring element; @c se.offsets must be non-empty (callers route the degenerate SE
 *        through a pass-through copy instead of this traversal, exactly as @ref RunMorphologySlabWindow).
 * @param axisSets the SE's per-axis entering/leaving offset sets (see @ref ComputeAxisAddRemoveSets),
 *        computed once by the caller and shared across every plane.
 * @param shouldCancel checked once per output row so a wide plane responds to cancellation promptly.
 * @tparam T scalar voxel type. @tparam Dilate true -> max fold, false -> min fold.
 */
template <class T, bool Dilate>
void RunMorphologyPlaneWindow(const T* slab, T* outPlane, usize boundDimX, usize boundDimY, usize boundDimZ, usize slabZLo, usize outputDimX, usize outputDimY,
                              const std::array<usize, 3>& originOffset, usize outputZ, const StructuringElement& se, const AxisAddRemoveSets& axisSets, const std::atomic_bool& shouldCancel)
{
  using Hist = MorphHistogram<T, Dilate>;
  const usize boundSliceValues = boundDimX * boundDimY;
  const int64 nX = static_cast<int64>(boundDimX);
  const int64 nY = static_cast<int64>(boundDimY);
  const int64 nZ = static_cast<int64>(boundDimZ);
  const int64 rX = (se.radius[0] > 0) ? se.radius[0] : 0;
  const int64 rY = (se.radius[1] > 0) ? se.radius[1] : 0;
  const int64 rZ = (se.radius[2] > 0) ? se.radius[2] : 0;
  const T boundary = Dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
  const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
  const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;
  const int64 originX = static_cast<int64>(originOffset[0]);
  const int64 originY = static_cast<int64>(originOffset[1]);
  const int64 z = static_cast<int64>(originOffset[2] + outputZ); // fixed neighborhood-center Z for the whole plane

  // Reads a padded-space (x,y,z) out of the ring; valid whenever (x,y,z) is in-bounds, since the ring always
  // spans every padded Z this call can reach (see class docs).
  auto flat = [slab, boundSliceValues, boundDimX, slabZLo](int64 x, int64 y, int64 zc) -> T {
    return slab[(static_cast<usize>(zc) - slabZLo) * boundSliceValues + static_cast<usize>(y) * boundDimX + static_cast<usize>(x)];
  };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 zc) { return x >= 0 && x < nX && y >= 0 && y < nY && zc >= 0 && zc < nZ; };

  // Slide the histogram from center (cx,cy,cz) by +1 along axis a -- identical to RunMorphologySlabWindow's
  // push, reading through the ring-relative `flat` instead of a whole-slab buffer.
  auto push = [&](Hist& h, int64 cx, int64 cy, int64 cz, int a) {
    const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
    if(fast)
    {
      for(const SEOffset& q : added[a])
      {
        h.addPixel(flat(cx + q.dx, cy + q.dy, cz + q.dz));
      }
      for(const SEOffset& o : removed[a])
      {
        h.removePixel(flat(cx + o.dx, cy + o.dy, cz + o.dz));
      }
    }
    else
    {
      for(const SEOffset& q : added[a])
      {
        const int64 x = cx + q.dx;
        const int64 y = cy + q.dy;
        const int64 zz = cz + q.dz;
        if(inBounds(x, y, zz))
        {
          h.addPixel(flat(x, y, zz));
        }
        else
        {
          h.addBoundary();
        }
      }
      for(const SEOffset& o : removed[a])
      {
        const int64 x = cx + o.dx;
        const int64 y = cy + o.dy;
        const int64 zz = cz + o.dz;
        if(inBounds(x, y, zz))
        {
          h.removePixel(flat(x, y, zz));
        }
        else
        {
          h.removeBoundary();
        }
      }
    }
  };

  // Seeds a row-start window at (originX, rowY, z) -- a one-time O(|SE|) gather so each Y-worker below
  // builds its OWN starting histogram independently of every other worker (mirrors RunMorphologySlabWindow's
  // per-worker seedHist, but at an arbitrary row start instead of a plane start, since Z never changes here).
  auto seedHist = [&](Hist& h, int64 rowY) {
    h.setBoundary(boundary);
    for(const SEOffset& o : se.offsets)
    {
      const int64 x = originX + o.dx;
      const int64 y = rowY + o.dy;
      const int64 zz = z + o.dz;
      if(inBounds(x, y, zz))
      {
        h.addPixel(flat(x, y, zz));
      }
      else
      {
        h.addBoundary();
      }
    }
  };

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, outputDimY);
  parallelAlgorithm.execute([&](const Range& yRange) {
    const usize workerYBegin = yRange.min();
    const usize workerYEnd = yRange.max();
    if(workerYBegin >= workerYEnd)
    {
      return;
    }

    // Per-worker scratch: two histograms seeded at this worker's OWN row start (originX, originY +
    // workerYBegin, z), independent of every other worker. histX is reset by assignment each row, reusing
    // its capacity.
    Hist histY;
    seedHist(histY, originY + static_cast<int64>(workerYBegin));
    Hist histX;

    for(usize oy = workerYBegin; oy < workerYEnd; ++oy)
    {
      if(shouldCancel)
      {
        return;
      }
      const int64 y = originY + static_cast<int64>(oy);
      if(oy > workerYBegin)
      {
        push(histY, originX, y - 1, z, 1);
      }
      histX = histY;
      for(usize ox = 0; ox < outputDimX; ++ox)
      {
        const int64 x = originX + static_cast<int64>(ox);
        outPlane[oy * outputDimX + ox] = histX.getValue();
        if(ox + 1 < outputDimX)
        {
          push(histX, x, y, z, 0);
        }
      }
    }
  });
}

/**
 * @brief The BINARY analogue of @ref RunMorphologyPlaneWindow: runs @ref BinaryMorphDirect's moving fg-count
 *        traversal over a SINGLE output Z-plane sliced out of a resident Z-ring, parallelized across Y. Used
 *        by @ref ApplyBinaryMorphologyCompositePipeline for both of its stages, exactly as the grayscale ring
 *        uses @ref RunMorphologyPlaneWindow for its two stages -- see that function's docs for the ring-slice
 *        rationale and the IDENTITY vs CROP @p originOffset mapping, both of which apply unchanged here.
 *
 * Slides a @ref detail::FgCountAccumulator instead of a min/max histogram: each worker constructs and seeds
 * ONE row-start accumulator at its own first row (a one-time O(|SE|) gather), then pushes it down Y and
 * across X via the same entering/leaving @p axisSets @ref RunMorphologyPlaneWindow uses, folding only the
 * neighbors that enter/leave the window per step. A neighbor is foreground iff it is in-bounds and equals
 * @p fg, or out-of-bounds and @p boundaryToForeground -- bit-identical to @ref BinaryMorphDirect and
 * @ref BinaryMorphScanline's boundary rule. Kept SEPARATE from the grayscale plane window so no binary branch
 * is ever added to its hot loop.
 *
 * @param fg/bg/boundaryToForeground the binary morphology parameters (see @ref detail::FgCountAccumulator).
 * @tparam T scalar voxel type. @tparam Dilate true -> any() fold, false -> all() fold.
 */
template <class T, bool Dilate>
void RunBinaryMorphologyPlaneWindow(const T* slab, T* outPlane, usize boundDimX, usize boundDimY, usize boundDimZ, usize slabZLo, usize outputDimX, usize outputDimY,
                                    const std::array<usize, 3>& originOffset, usize outputZ, const StructuringElement& se, const AxisAddRemoveSets& axisSets, T fg, T bg, bool boundaryToForeground,
                                    const std::atomic_bool& shouldCancel)
{
  using Acc = FgCountAccumulator<T, Dilate>;
  const usize windowSize = se.offsets.size();
  const usize boundSliceValues = boundDimX * boundDimY;
  const int64 nX = static_cast<int64>(boundDimX);
  const int64 nY = static_cast<int64>(boundDimY);
  const int64 nZ = static_cast<int64>(boundDimZ);
  const int64 rX = (se.radius[0] > 0) ? se.radius[0] : 0;
  const int64 rY = (se.radius[1] > 0) ? se.radius[1] : 0;
  const int64 rZ = (se.radius[2] > 0) ? se.radius[2] : 0;
  const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
  const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;
  const int64 originX = static_cast<int64>(originOffset[0]);
  const int64 originY = static_cast<int64>(originOffset[1]);
  const int64 z = static_cast<int64>(originOffset[2] + outputZ); // fixed neighborhood-center Z for the whole plane

  // Reads a padded-space (x,y,z) out of the ring (see RunMorphologyPlaneWindow).
  auto flat = [slab, boundSliceValues, boundDimX, slabZLo](int64 x, int64 y, int64 zc) -> T {
    return slab[(static_cast<usize>(zc) - slabZLo) * boundSliceValues + static_cast<usize>(y) * boundDimX + static_cast<usize>(x)];
  };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 zc) { return x >= 0 && x < nX && y >= 0 && y < nY && zc >= 0 && zc < nZ; };

  // Slide the accumulator from center (cx,cy,cz) by +1 along axis a -- identical shape to
  // RunMorphologyPlaneWindow's push, reading through the ring-relative `flat`.
  auto push = [&](Acc& acc, int64 cx, int64 cy, int64 cz, int a) {
    const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
    if(fast)
    {
      for(const SEOffset& q : added[a])
      {
        acc.addPixel(flat(cx + q.dx, cy + q.dy, cz + q.dz));
      }
      for(const SEOffset& o : removed[a])
      {
        acc.removePixel(flat(cx + o.dx, cy + o.dy, cz + o.dz));
      }
    }
    else
    {
      for(const SEOffset& q : added[a])
      {
        const int64 x = cx + q.dx;
        const int64 y = cy + q.dy;
        const int64 zz = cz + q.dz;
        if(inBounds(x, y, zz))
        {
          acc.addPixel(flat(x, y, zz));
        }
        else
        {
          acc.addBoundary();
        }
      }
      for(const SEOffset& o : removed[a])
      {
        const int64 x = cx + o.dx;
        const int64 y = cy + o.dy;
        const int64 zz = cz + o.dz;
        if(inBounds(x, y, zz))
        {
          acc.removePixel(flat(x, y, zz));
        }
        else
        {
          acc.removeBoundary();
        }
      }
    }
  };

  // Seeds a row-start window at (originX, rowY, z) into an ALREADY-CONSTRUCTED accumulator -- unlike the
  // grayscale seedHist, @ref detail::FgCountAccumulator has no default constructor, so the caller constructs
  // a fresh {fg, bg, boundaryToForeground, windowSize} accumulator once per worker and this only performs the
  // O(|SE|) gather into it (mirrors RunMorphologyPlaneWindow's row-start seeding).
  auto seedAcc = [&](Acc& acc, int64 rowY) {
    for(const SEOffset& o : se.offsets)
    {
      const int64 x = originX + o.dx;
      const int64 y = rowY + o.dy;
      const int64 zz = z + o.dz;
      if(inBounds(x, y, zz))
      {
        acc.addPixel(flat(x, y, zz));
      }
      else
      {
        acc.addBoundary();
      }
    }
  };

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, outputDimY);
  parallelAlgorithm.execute([&](const Range& yRange) {
    const usize workerYBegin = yRange.min();
    const usize workerYEnd = yRange.max();
    if(workerYBegin >= workerYEnd)
    {
      return;
    }

    // Per-worker scratch: two accumulators seeded at this worker's OWN row start (originX, originY +
    // workerYBegin, z), independent of every other worker. accX is reset by assignment each row.
    Acc accY(fg, bg, boundaryToForeground, windowSize);
    seedAcc(accY, originY + static_cast<int64>(workerYBegin));
    Acc accX(fg, bg, boundaryToForeground, windowSize);

    for(usize oy = workerYBegin; oy < workerYEnd; ++oy)
    {
      if(shouldCancel)
      {
        return;
      }
      const int64 y = originY + static_cast<int64>(oy);
      if(oy > workerYBegin)
      {
        push(accY, originX, y - 1, z, 1);
      }
      accX = accY;
      for(usize ox = 0; ox < outputDimX; ++ox)
      {
        const int64 x = originX + static_cast<int64>(ox);
        outPlane[oy * outputDimX + ox] = accX.getValue();
        if(ox + 1 < outputDimX)
        {
          push(accX, x, y, z, 0);
        }
      }
    }
  });
}

/**
 * @brief The FUSED-gradient analogue of @ref RunMorphologySlabWindow: runs @ref MorphGradientDirect's
 *        moving-window traversal over an out-of-core batch's staged Z-slab, sliding ONE @ref
 *        MorphGradientHistogram that tracks both the window max and min so every voxel's @c max - min is
 *        produced in a single pass (replacing a per-voxel gather that folded both extrema separately).
 *
 * Boundary, slab-coverage, and work-item-decomposition semantics are identical to @ref
 * RunMorphologySlabWindow (see its docs for the flattened (localZ, yBlock) partition and the K heuristic
 * in @ref MorphologySlabWindowYBlockRows); the only differences are the histogram type and that @ref
 * MorphGradientHistogram's addBoundary/removeBoundary are no-ops (an out-of-image neighbor can win neither
 * extreme), so unlike the single-extremum traversal there is no boundary value to seed.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
void RunMorphologyGradientSlabWindow(const T* slab, T* outSlab, usize dimX, usize dimY, usize dimZ, usize zLo, usize zBegin, usize outputDepth, const StructuringElement& se,
                                     const AxisAddRemoveSets& axisSets, const std::atomic_bool& shouldCancel)
{
  using Hist = MorphGradientHistogram<T>;
  const usize sliceValues = dimX * dimY;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  const int64 rX = (se.radius[0] > 0) ? se.radius[0] : 0;
  const int64 rY = (se.radius[1] > 0) ? se.radius[1] : 0;
  const int64 rZ = (se.radius[2] > 0) ? se.radius[2] : 0;
  const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
  const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;

  auto flat = [slab, sliceValues, dimX, zLo](int64 x, int64 y, int64 z) -> T { return slab[(static_cast<usize>(z) - zLo) * sliceValues + static_cast<usize>(y) * dimX + static_cast<usize>(x)]; };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  auto push = [&](Hist& h, int64 cx, int64 cy, int64 cz, int a) {
    const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
    if(fast)
    {
      for(const SEOffset& q : added[a])
      {
        h.addPixel(flat(cx + q.dx, cy + q.dy, cz + q.dz));
      }
      for(const SEOffset& o : removed[a])
      {
        h.removePixel(flat(cx + o.dx, cy + o.dy, cz + o.dz));
      }
    }
    else
    {
      for(const SEOffset& q : added[a])
      {
        const int64 x = cx + q.dx;
        const int64 y = cy + q.dy;
        const int64 z = cz + q.dz;
        if(inBounds(x, y, z))
        {
          h.addPixel(flat(x, y, z));
        }
        else
        {
          h.addBoundary();
        }
      }
      for(const SEOffset& o : removed[a])
      {
        const int64 x = cx + o.dx;
        const int64 y = cy + o.dy;
        const int64 z = cz + o.dz;
        if(inBounds(x, y, z))
        {
          h.removePixel(flat(x, y, z));
        }
        else
        {
          h.removeBoundary();
        }
      }
    }
  };

  // Seed a row-start window at (0, rowY, z) (mirrors RunMorphologySlabWindow's seedRowHist; no boundary
  // value to set since addBoundary/removeBoundary are no-ops for the fused gradient).
  auto seedRowHist = [&](Hist& h, int64 rowY, int64 z) {
    for(const SEOffset& o : se.offsets)
    {
      const int64 x = o.dx;
      const int64 y = rowY + o.dy;
      const int64 zz = z + o.dz;
      if(inBounds(x, y, zz))
      {
        h.addPixel(flat(x, y, zz));
      }
      else
      {
        h.addBoundary();
      }
    }
  };

  const usize blockRows = MorphologySlabWindowYBlockRows(outputDepth, dimY);
  const usize blockCount = blockRows == 0 ? 0 : (dimY + blockRows - 1) / blockRows;
  const usize itemCount = outputDepth * blockCount;

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, itemCount);
  parallelAlgorithm.execute([&](const Range& itemRange) {
    // Per-item scratch: histY starts each work item from a freshly value-initialized histogram before
    // seeding (seedRowHist only ADDS the window's entries; the histogram types keep their contents
    // across setBoundary, so seeding a carried-over histogram would fold the previous item's final
    // window into this one). histX is reset by assignment each row.
    Hist histY;
    Hist histX;

    for(usize item = itemRange.min(); item < itemRange.max(); ++item)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localZ = item / blockCount;
      const usize blockIndex = item % blockCount;
      const usize yBegin = blockIndex * blockRows;
      const usize yEnd = std::min(dimY, yBegin + blockRows);
      const int64 z = static_cast<int64>(zBegin + localZ);

      histY = Hist{};
      seedRowHist(histY, static_cast<int64>(yBegin), z);
      for(usize y = yBegin; y < yEnd; ++y)
      {
        if(y > yBegin)
        {
          push(histY, 0, static_cast<int64>(y) - 1, z, 1);
        }
        histX = histY;
        for(int64 x = 0; x < nX; ++x)
        {
          outSlab[localZ * sliceValues + y * dimX + static_cast<usize>(x)] = histX.getGradient();
          if(x < nX - 1)
          {
            push(histX, x, static_cast<int64>(y), z, 0);
          }
        }
      }
    }
  });
}

} // namespace detail

/**
 * @brief Out-of-core-safe grayscale morphology (Scanline path).
 *
 * For true 3D, each batch bulk-reads a clamped Z slab with its complete halo. Moderate multi-plane
 * 8-bit neighborhoods use @ref detail::RunMorphologyFlatFoldBlock, which folds contiguous interior X
 * rows with SIMD extrema operations and keeps checked scalar borders. Other cases use
 * @ref detail::RunMorphologySlabWindow. Both routes test global image bounds and preserve the extrema
 * boundary rule. For true 2D, @ref detail::ExecuteBoundedMorphology2D gathers neighborhoods from a bounded buffer.
 *
 * The algorithm keeps store I/O serial. Only staged-buffer work runs in parallel. The 3D planner targets
 * about 16 MiB. One output plane and its complete halo remain resident, so wide planes can exceed the target.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class MorphScanline
{
public:
  MorphScanline(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, const std::atomic_bool& shouldCancel,
                const IFilter::MessageHandler& messageHandler, usize targetBufferBytes = detail::k_MorphologySlabTargetBytes)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_Op(op)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_TargetBufferBytes(targetBufferBytes)
  {
  }

  ~MorphScanline() = default;

  MorphScanline(const MorphScanline&) = delete;
  MorphScanline(MorphScanline&&) noexcept = delete;
  MorphScanline& operator=(const MorphScanline&) = delete;
  MorphScanline& operator=(MorphScanline&&) noexcept = delete;

  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    // The SE's z-offsets span [-rz, rz]; a negative radius means an empty axis, so clamp to 0 for the
    // slab bounds (the offset list is empty in that degenerate case anyway).
    const usize rz = (m_SE.radius[2] > 0) ? static_cast<usize>(m_SE.radius[2]) : 0;
    const bool dilate = (m_Op == MorphOp::Dilate);
    detail::MorphologySlabPlan plan;
    if(Result<> validation = detail::MakeMorphologySlabPlan(m_In, m_Out, m_Dims, rz, m_TargetBufferBytes, plan); validation.invalid())
    {
      return validation;
    }
    if(plan.volumeValues == 0)
    {
      return {};
    }
    const usize sliceValues = plan.sliceValues;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying morphology filter: {:.1f}%");
    auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    if(dimZ == 1)
    {
      Result<> result =
          detail::ExecuteBoundedMorphology2D<T>(m_In, m_Out, m_Dims, m_SE, m_ShouldCancel, m_TargetBufferBytes,
                                                [&](const T* input, T* output, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
                                                  return detail::Morphology2DBlockBody<T>{.input = input,
                                                                                          .output = output,
                                                                                          .offsets = m_SE.offsets.data(),
                                                                                          .numOffsets = m_SE.offsets.size(),
                                                                                          .dimX = dimX,
                                                                                          .dimY = dimY,
                                                                                          .inputXBegin = inputXBegin,
                                                                                          .inputYBegin = inputYBegin,
                                                                                          .inputWidth = inputWidth,
                                                                                          .outputXBegin = outputXBegin,
                                                                                          .outputYBegin = outputYBegin,
                                                                                          .outputWidth = outputWidth,
                                                                                          .dilate = dilate};
                                                });
      if(result.valid() && !m_ShouldCancel)
      {
        progressMessenger.sendProgressMessage(1);
      }
      return result;
    }

    auto outSlab = std::make_unique<T[]>(plan.outputBatchDepth * sliceValues);

    // Degenerate structuring element (empty offset list, only reachable for a radius-0 Annulus): there
    // are no neighbors to reduce, so pass the input through unchanged rather than emitting the fold
    // identity (lowest()/max()) into every voxel. Bounded: one output batch, reused across Z.
    if(m_SE.offsets.empty())
    {
      for(usize zBegin = 0; zBegin < dimZ; zBegin += plan.outputBatchDepth)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize outputDepth = std::min(plan.outputBatchDepth, dimZ - zBegin);
        const usize outputValues = outputDepth * sliceValues;
        if(Result<> r = m_In.copyIntoBuffer(zBegin * sliceValues, nonstd::span<T>(outSlab.get(), outputValues)); r.invalid())
        {
          return r;
        }
        if(m_ShouldCancel)
        {
          return {};
        }
        if(Result<> r = m_Out.copyFromBuffer(zBegin * sliceValues, nonstd::span<const T>(outSlab.get(), outputValues)); r.invalid())
        {
          return r;
        }
        progressMessenger.sendProgressMessage(outputDepth);
      }
      return {};
    }

    // The flat fold uses only its precomputed offsets. The moving-window fallback needs axis sets.
    const bool useFlatFold = detail::k_UseVectorHistogram<T> && detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE);
    const std::vector<detail::MorphologyFlatFoldOffset> flatOffsets = useFlatFold ? detail::MakeMorphologyFlatFoldOffsets(m_SE, dimX, dimY) : std::vector<detail::MorphologyFlatFoldOffset>{};
    std::optional<detail::AxisAddRemoveSets> axisSets;
    if(!useFlatFold)
    {
      axisSets = detail::ComputeAxisAddRemoveSets(m_SE);
    }

    // Allocate one input halo slab at the maximum batch depth and reuse it. The 16 MiB target covers
    // input plus output buffers whenever at least one plane fits; unusually wide images retain the
    // required one-plane O(slice) minimum. The bound never scales with the full volume depth.
    auto slab = std::make_unique<T[]>(plan.maxInputDepth * sliceValues);
    usize loadedZLo = 0;
    usize loadedDepth = 0;

    for(usize zBegin = 0; zBegin < dimZ; zBegin += plan.outputBatchDepth)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize outputDepth = std::min(plan.outputBatchDepth, dimZ - zBegin);
      const usize zEnd = zBegin + outputDepth;
      const usize zLo = (zBegin >= rz) ? (zBegin - rz) : 0;
      const usize zHi = (zEnd - 1) + std::min(rz, dimZ - zEnd);
      const usize slabDepth = zHi - zLo + 1;
      if(loadedDepth == 0)
      {
        if(Result<> r = m_In.copyIntoBuffer(zLo * sliceValues, nonstd::span<T>(slab.get(), slabDepth * sliceValues)); r.invalid())
        {
          return r;
        }
      }
      else
      {
        const usize loadedZHi = loadedZLo + loadedDepth - 1;
        const usize retainedZLo = std::max(zLo, loadedZLo);
        const usize retainedZHi = std::min(zHi, loadedZHi);
        const usize retainedDepth = retainedZLo <= retainedZHi ? retainedZHi - retainedZLo + 1 : 0;
        if(retainedDepth > 0)
        {
          std::memmove(slab.get(), slab.get() + (retainedZLo - loadedZLo) * sliceValues, retainedDepth * sliceValues * sizeof(T));
        }
        const usize readZBegin = retainedDepth == 0 ? zLo : retainedZHi + 1;
        if(readZBegin <= zHi)
        {
          const usize readDepth = zHi - readZBegin + 1;
          if(Result<> r = m_In.copyIntoBuffer(readZBegin * sliceValues, nonstd::span<T>(slab.get() + retainedDepth * sliceValues, readDepth * sliceValues)); r.invalid())
          {
            return r;
          }
        }
      }
      loadedZLo = zLo;
      loadedDepth = slabDepth;

      if(useFlatFold)
      {
        if(dilate)
        {
          detail::RunMorphologyFlatFoldBlock<T, true>(slab.get(), outSlab.get(), m_Dims, zLo, SizeVec3{dimX, dimY, outputDepth}, SizeVec3{0, 0, zBegin}, m_SE, flatOffsets, m_ShouldCancel);
        }
        else
        {
          detail::RunMorphologyFlatFoldBlock<T, false>(slab.get(), outSlab.get(), m_Dims, zLo, SizeVec3{dimX, dimY, outputDepth}, SizeVec3{0, 0, zBegin}, m_SE, flatOffsets, m_ShouldCancel);
        }
      }
      else
      {
        if(dilate)
        {
          detail::RunMorphologySlabWindow<T, true>(slab.get(), outSlab.get(), dimX, dimY, dimZ, zLo, zBegin, outputDepth, m_SE, *axisSets, m_ShouldCancel);
        }
        else
        {
          detail::RunMorphologySlabWindow<T, false>(slab.get(), outSlab.get(), dimX, dimY, dimZ, zLo, zBegin, outputDepth, m_SE, *axisSets, m_ShouldCancel);
        }
      }

      if(m_ShouldCancel)
      {
        return {};
      }
      if(Result<> r = m_Out.copyFromBuffer(zBegin * sliceValues, nonstd::span<const T>(outSlab.get(), outputDepth * sliceValues)); r.invalid())
      {
        return r;
      }
      progressMessenger.sendProgressMessage(outputDepth);
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  MorphOp m_Op;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_TargetBufferBytes;
};

/**
 * @brief Resident-buffer adaptive grayscale morphology (Direct path): the FAST engine.
 *
 * Moderate kernels on multi-plane 8-bit images use a parallel flat-offset fold across contiguous interior
 * X rows with SIMD extrema operations. Checked scalar folds preserve exact borders. Larger neighborhoods,
 * true 2D, and wider value types use the moving-histogram implementation lifted from ITK 5.4.4
 * (itkMovingHistogramImageFilter[.hxx],
 * itkMovingHistogramImageFilterBase.hxx, itkMorphologyHistogram.h, itkMovingHistogramMorphologyImageFilter
 * and the Dilate/Erode subclasses). The whole array is pulled into a flat buffer once for free random access;
 * then a histogram of the structuring-element neighborhood is *slid*
 * across the volume, folding in only the voxels that ENTER and folding out only the voxels that LEAVE as
 * the center steps by one. That reduces the per-voxel cost from O(|SE|) (Scanline) to O(perimeter).
 *
 * Traversal. ITK computes, per axis, the "added" and "removed" offset sets for a +1 translation
 * (@c m_AddedOffsets / @c m_RemovedOffsets) and reuses one histogram across whole lines via its
 * HistVec/Steps bookkeeping. This implementation specializes that to a FIXED fastest-axis (X) traversal
 * for cache locality and verifiability: three nested moving histograms mirror the three loop levels --
 * @c histZ holds the (0,0,z) plane-start window (stepped +Z per plane), @c histY the (0,y,z) line-start
 * window (reset from @c histZ, stepped +Y per line), and @c histX the working window (reset from @c histY,
 * stepped +X across the line). This is exactly ITK's line-transition reuse (no rebuild after the single
 * origin fill) without ITK's per-thread best-axis heuristic; results are identical either way.
 *
 * Boundary. Out-of-image SE neighbors are folded in as the type extremum via AddBoundary/RemoveBoundary
 * (Dilate -> lowest(), Erode -> max()), bit-identical to Scanline skipping OOB neighbors (see @ref MorphOp
 * and @ref MovingHistogramMorphologyImageFilter's SetBoundary). The @ref detail::MorphHistogram alias
 * picks a dense bin-count histogram for narrow ints and a std::map for wide/float types.
 *
 * Its required constructor arguments match @ref MorphScanline so both drop into @ref DispatchAlgorithm.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class MorphDirect
{
public:
  MorphDirect(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, const std::atomic_bool& shouldCancel,
              const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_Op(op)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  ~MorphDirect() = default;

  MorphDirect(const MorphDirect&) = delete;
  MorphDirect(MorphDirect&&) noexcept = delete;
  MorphDirect& operator=(const MorphDirect&) = delete;
  MorphDirect& operator=(MorphDirect&&) noexcept = delete;

  Result<> operator()()
  {
    // Dispatch the runtime op to a compile-time bool so the histogram's comparator/extremum are fixed at
    // compile time (matching ITK's distinct Dilate/Erode filter classes).
    return (m_Op == MorphOp::Dilate) ? runImpl<true>() : runImpl<false>();
  }

private:
  template <bool Dilate>
  Result<> runImpl()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    const usize sliceValues = dimX * dimY;
    const usize vol = sliceValues * dimZ;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying morphology filter: {:.1f}%");

    // Degenerate structuring element (empty offset list, only reachable for a radius-0 Annulus): pass the
    // input through unchanged, byte-for-byte identical to MorphScanline so the two paths still agree. This
    // rare edge case stays serial; its output-plane scratch lives here so the parallel fast path below never
    // allocates it.
    if(m_SE.offsets.empty())
    {
      auto outPlane = std::make_unique<T[]>(sliceValues);
      for(usize z = 0; z < dimZ; ++z)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        if(Result<> r = m_In.copyIntoBuffer(z * sliceValues, nonstd::span<T>(outPlane.get(), sliceValues)); r.invalid())
        {
          return r;
        }
        if(Result<> r = m_Out.copyFromBuffer(z * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
        {
          return r;
        }
      }
      return {};
    }

    // Pull the whole array into a flat local buffer once for free random SE access. This is a memcpy in-core
    // and one bulk read when the working-memory wrapper selects this route for a chunked array.
    auto data = std::make_unique<T[]>(vol);
    if(Result<> r = m_In.copyIntoBuffer(0, nonstd::span<T>(data.get(), vol)); r.invalid())
    {
      return r;
    }
    const T* in = data.get();

    if constexpr(detail::k_UseVectorHistogram<T>)
    {
      if(detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE))
      {
        auto output = std::make_unique<T[]>(vol);
        detail::RunMorphologyFlatFoldBlock<T, Dilate>(in, output.get(), m_Dims, 0, m_Dims, SizeVec3{0, 0, 0}, m_SE, m_ShouldCancel);
        if(m_ShouldCancel)
        {
          return {};
        }
        return m_Out.copyFromBuffer(0, nonstd::span<const T>(output.get(), vol));
      }
    }

    // Build, per axis, the "entering" (added) and "leaving" (removed) offset sets for a +1 step (shared with
    // BinaryMorphDirect; see detail::ComputeAxisAddRemoveSets). Bound as const references so the push slide
    // and its captures below are unchanged from the prior inline computation.
    const detail::AxisAddRemoveSets axisSets = detail::ComputeAxisAddRemoveSets(m_SE);
    const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
    const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;

    // Boundary extremum folded in for out-of-image neighbors (see class docs / MorphOp).
    const T boundary = Dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();

    using Hist = detail::MorphHistogram<T, Dilate>;

    const int64 nX = static_cast<int64>(dimX);
    const int64 nY = static_cast<int64>(dimY);
    const int64 nZ = static_cast<int64>(dimZ);
    // Per-axis radius (clamped >= 0). Any added/removed offset reaches at most r_a + 1 along axis a, so a
    // center whose (r_a + 1)-padded neighborhood is fully in bounds needs no per-voxel checks.
    const int64 rX = (m_SE.radius[0] > 0) ? m_SE.radius[0] : 0;
    const int64 rY = (m_SE.radius[1] > 0) ? m_SE.radius[1] : 0;
    const int64 rZ = (m_SE.radius[2] > 0) ? m_SE.radius[2] : 0;

    // PRECONDITION for the unchecked fast path below: every SE offset lies within its per-axis radius box,
    // i.e. offsets subseteq [-radius, radius]^3. radius and offsets are independent public members of
    // StructuringElement, so assert their agreement once here (MakeStructuringElement always honors it).
#ifndef NDEBUG
    for(const SEOffset& o : m_SE.offsets)
    {
      assert(o.dx >= -rX && o.dx <= rX && o.dy >= -rY && o.dy <= rY && o.dz >= -rZ && o.dz <= rZ && "SE offset outside its radius box; fast-path bounds assumption violated");
    }
#endif

    auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
    auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

    // Slide the histogram from center (cx,cy,cz) by +1 along axis a: fold in the entering voxels, fold out
    // the leaving ones. The fast path (whole padded neighborhood in bounds) skips per-voxel checks; the slow
    // path substitutes the boundary extremum for OOB neighbors (== Scanline skipping them).
    auto push = [&](Hist& h, int64 cx, int64 cy, int64 cz, int a) {
      const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
      if(fast)
      {
        for(const SEOffset& q : added[a])
        {
          h.addPixel(in[flat(cx + q.dx, cy + q.dy, cz + q.dz)]);
        }
        for(const SEOffset& o : removed[a])
        {
          h.removePixel(in[flat(cx + o.dx, cy + o.dy, cz + o.dz)]);
        }
      }
      else
      {
        for(const SEOffset& q : added[a])
        {
          const int64 x = cx + q.dx;
          const int64 y = cy + q.dy;
          const int64 z = cz + q.dz;
          if(inBounds(x, y, z))
          {
            h.addPixel(in[flat(x, y, z)]);
          }
          else
          {
            h.addBoundary();
          }
        }
        for(const SEOffset& o : removed[a])
        {
          const int64 x = cx + o.dx;
          const int64 y = cy + o.dy;
          const int64 z = cz + o.dz;
          if(inBounds(x, y, z))
          {
            h.removePixel(in[flat(x, y, z)]);
          }
          else
          {
            h.removeBoundary();
          }
        }
      }
    };

    // Seed a plane-start window at corner (0,0,zStart) -- a one-time O(|SE|) init identical to the former
    // origin seed but shifted along Z, so each parallel Z-slab worker below can build its OWN starting
    // histogram independently instead of carrying one across slabs.
    auto seedHist = [&](Hist& h, int64 zStart) {
      h.setBoundary(boundary);
      for(const SEOffset& o : m_SE.offsets)
      {
        const int64 x = o.dx;
        const int64 y = o.dy;
        const int64 z = zStart + o.dz;
        if(inBounds(x, y, z))
        {
          h.addPixel(in[flat(x, y, z)]);
        }
        else
        {
          h.addBoundary();
        }
      }
    };

    // Parallelize the moving-histogram traversal across contiguous Z-slabs (mirrors MorphScanline's
    // ParallelDataAlgorithm use and ITK's DynamicThreadedGenerateData). Every output voxel is an
    // order-independent min/max reduction over the READ-ONLY input `in`, and each worker owns its output
    // plane plus its three nested histograms and writes only its own disjoint Z-planes, so threading cannot
    // change any value. Direct is chosen only for in-core stores; requireStoresInMemory keeps parallelization
    // on for them (and defensively serializes were a store ever out-of-core).
    std::atomic<bool> hasError{false};
    std::mutex resultMutex;
    Result<> firstError;

    auto processZRange = [&](const Range& zRange) {
      const usize zBegin = zRange.min();
      const usize zEnd = zRange.max();
      if(zBegin >= zEnd)
      {
        return;
      }
      // Per-worker scratch: its own output plane, its own ProgressMessenger (the messenger is not shared-
      // thread-safe, but the progress COUNTER it feeds is atomic), and its own three nested histograms.
      auto outPlane = std::make_unique<T[]>(sliceValues);
      auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

      // Seed this slab's plane-start window at its OWN origin corner (0,0,zBegin), independent of every other
      // slab. histY (line start) and histX (working window) are declared ONCE per worker and reset by
      // assignment below, reusing their capacity instead of reallocating a histogram every line/plane --
      // byte-identical to copy-constructing them each iteration.
      Hist histZ;
      seedHist(histZ, static_cast<int64>(zBegin));
      Hist histY;
      Hist histX;

      for(usize zc = zBegin; zc < zEnd; ++zc)
      {
        if(m_ShouldCancel)
        {
          return;
        }
        const int64 z = static_cast<int64>(zc);
        if(zc > zBegin)
        {
          push(histZ, 0, 0, z - 1, 2);
        }
        histY = histZ;
        for(int64 y = 0; y < nY; ++y)
        {
          if(y > 0)
          {
            push(histY, 0, y - 1, z, 1);
          }
          histX = histY;
          for(int64 x = 0; x < nX; ++x)
          {
            outPlane[static_cast<usize>(y * nX + x)] = histX.getValue();
            if(x < nX - 1)
            {
              push(histX, x, y, z, 0);
            }
          }
        }
        if(Result<> r = m_Out.copyFromBuffer(zc * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
        {
          const std::lock_guard<std::mutex> lock(resultMutex);
          if(!hasError)
          {
            firstError = std::move(r);
            hasError = true;
          }
          return;
        }
        progressMessenger.sendProgressMessage(1);
      }
    };

    ParallelDataAlgorithm parallelAlgorithm;
    IParallelAlgorithm::AlgorithmStores algStores{&m_In, &m_Out};
    parallelAlgorithm.requireStoresInMemory(algStores);
    parallelAlgorithm.setRange(0, dimZ);
    parallelAlgorithm.execute(processZRange);

    if(hasError)
    {
      return firstError;
    }
    return {};
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  MorphOp m_Op;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

template <class T>
class MorphWorkingMemory
{
public:
  MorphWorkingMemory(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, const std::atomic_bool& shouldCancel,
                     const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_Op(op)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    auto allocationResult = detail::ReserveMorphologyResidentWorkingMemory<T>(m_Dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if constexpr(detail::k_UseVectorHistogram<T>)
    {
      if(detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE) && allocation.holdsCompleteState())
      {
        try
        {
          return MorphDirect<T>{m_In, m_Out, m_Dims, m_SE, m_Op, m_ShouldCancel, m_MessageHandler}();
        } catch(const std::bad_alloc&)
        {
          // Reuse the complete reservation for the bounded scanline fallback.
        }
      }
    }
    if(allocation.reservation.sizeBytes() > 0 && allocation.reservation.sizeBytes() <= std::numeric_limits<usize>::max())
    {
      detail::MorphologySlabPlan plan;
      const usize radiusZ = m_SE.radius[2] > 0 ? static_cast<usize>(m_SE.radius[2]) : 0;
      if(Result<> result = detail::MakeMorphologySlabPlan(m_In, m_Out, m_Dims, radiusZ, static_cast<usize>(allocation.reservation.sizeBytes()), plan); result.invalid())
      {
        return result;
      }
      usize residentValues = 0;
      usize residentBytes = 0;
      if(plan.volumeValues > 0 && plan.maxInputDepth <= std::numeric_limits<usize>::max() - plan.outputBatchDepth)
      {
        const usize residentPlaneCount = plan.maxInputDepth + plan.outputBatchDepth;
        if(detail::TryMultiplyMorphologySize(residentPlaneCount, plan.sliceValues, residentValues) && detail::TryMultiplyMorphologySize(residentValues, sizeof(T), residentBytes))
        {
          allocation.reservation.shrinkTo(static_cast<uint64>(residentBytes));
        }
      }
      return MorphScanline<T>{m_In, m_Out, m_Dims, m_SE, m_Op, m_ShouldCancel, m_MessageHandler, static_cast<usize>(allocation.reservation.sizeBytes())}();
    }
    return MorphScanline<T>{m_In, m_Out, m_Dims, m_SE, m_Op, m_ShouldCancel, m_MessageHandler}();
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  MorphOp m_Op;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Public entry point for the grayscale morphology engine.
 *
 * Dispatches on storage type via @ref DispatchAlgorithm. In-core arrays use @ref MorphDirect. Chunked arrays
 * use a complete resident 8-bit state only when the shared working-memory budget grants it; otherwise they use
 * @ref MorphScanline. Both routes produce bit-identical output.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
Result<> ApplyMorphology(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, const IDataArray& inArray,
                         const IDataArray& outArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  return DispatchAlgorithm<MorphDirect<T>, MorphWorkingMemory<T>>({&inArray, &outArray}, in, out, dims, se, op, shouldCancel, messageHandler);
}

template <class T>
Result<usize> CalculateMorphologyCompositePipelineWorkingMemoryBytes(const SizeVec3& dims, const StructuringElement& se, bool safeBorder)
{
  const std::array<usize, 3> cropOffset = safeBorder ? detail::MorphologySafeBorderPadRadius(se, dims) : std::array<usize, 3>{0, 0, 0};
  SizeVec3 paddedDims = dims;
  if(safeBorder)
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(cropOffset[axis] > (std::numeric_limits<usize>::max() - dims[axis]) / 2)
      {
        return MakeErrorResult<usize>(
            -8479, fmt::format("SafeBorder morphology-composite padded dimensions overflow. Dimensions: {}; axis: {}; radius: {}.", StringUtilities::formatDimensions3D(dims), axis, cropOffset[axis]));
      }
      paddedDims[axis] = dims[axis] + 2 * cropOffset[axis];
    }
  }
  const usize radiusZ = detail::MorphologyCompositePipelineRadiusZ(se, dims);
  if(radiusZ > (std::numeric_limits<usize>::max() - 1) / 2)
  {
    return MakeErrorResult<usize>(
        -8479, fmt::format("SafeBorder morphology-composite pipeline Z radius {} overflows its rolling-ring depth for dimensions {}.", radiusZ, StringUtilities::formatDimensions3D(dims)));
  }
  const usize ringDepth = 2 * radiusZ + 1;
  usize paddedSliceValues = 0;
  usize outputSliceValues = 0;
  usize ringValues = 0;
  usize requiredValues = 0;
  usize requiredBytes = 0;
  if(!detail::TryMultiplyMorphologySize(paddedDims[0], paddedDims[1], paddedSliceValues) || !detail::TryMultiplyMorphologySize(dims[0], dims[1], outputSliceValues) ||
     !detail::TryMultiplyMorphologySize(ringDepth, paddedSliceValues, ringValues) || ringValues > std::numeric_limits<usize>::max() / 2 ||
     !detail::TryMultiplyMorphologySize(ringValues, usize{2}, requiredValues) || requiredValues > std::numeric_limits<usize>::max() - outputSliceValues ||
     !detail::TryMultiplyMorphologySize(requiredValues + outputSliceValues, sizeof(T), requiredBytes))
  {
    return MakeErrorResult<usize>(
        -8479, fmt::format("SafeBorder morphology-composite pipeline working-memory dimensions overflow. Dimensions: {}; padded slice values: {}; output slice values: {}; ring depth: {}.",
                           StringUtilities::formatDimensions3D(dims), paddedSliceValues, outputSliceValues, ringDepth));
  }
  return {requiredBytes};
}

template <class T>
Result<> ApplyMorphologyCompositePipeline(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, const SizeVec3& dims, const StructuringElement& se, MorphOp firstOp, MorphOp secondOp,
                                          bool safeBorder, detail::MorphologyCompositeOutputMode outputMode, const std::atomic_bool& shouldCancel, usize grantedWorkingMemoryBytes)
{
  if(shouldCancel)
  {
    return {};
  }
  auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<T>(dims, se, safeBorder);
  if(requiredBytesResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(requiredBytesResult));
  }
  const usize requiredBytes = requiredBytesResult.value();
  const std::array<usize, 3> cropOffset = safeBorder ? detail::MorphologySafeBorderPadRadius(se, dims) : std::array<usize, 3>{0, 0, 0};
  SizeVec3 paddedDims = dims;
  if(safeBorder)
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      paddedDims[axis] = dims[axis] + 2 * cropOffset[axis];
    }
  }
  const usize radiusZ = detail::MorphologyCompositePipelineRadiusZ(se, dims);
  const usize ringDepth = 2 * radiusZ + 1;
  usize paddedSliceValues = 0;
  usize outputSliceValues = 0;
  usize ringValues = 0;
  if(!detail::TryMultiplyMorphologySize(paddedDims[0], paddedDims[1], paddedSliceValues) || !detail::TryMultiplyMorphologySize(dims[0], dims[1], outputSliceValues) ||
     !detail::TryMultiplyMorphologySize(ringDepth, paddedSliceValues, ringValues))
  {
    return MakeErrorResult(-8479, fmt::format("SafeBorder morphology-composite pipeline slab dimensions overflow. Dimensions: {}; padded dimensions: {}; ring depth: {}.",
                                              StringUtilities::formatDimensions3D(dims), StringUtilities::formatDimensions3D(paddedDims), ringDepth));
  }
  if(grantedWorkingMemoryBytes < requiredBytes)
  {
    return MakeErrorResult(-8480, fmt::format("SafeBorder morphology-composite pipeline requires {} bytes but only {} bytes are reserved. Dimensions: {}; padded dimensions: {}; Z radius: {}.",
                                              requiredBytes, grantedWorkingMemoryBytes, StringUtilities::formatDimensions3D(dims), StringUtilities::formatDimensions3D(paddedDims), radiusZ));
  }

  auto inputRing = std::make_unique<T[]>(ringValues);
  auto firstRing = std::make_unique<T[]>(ringValues);

  // Read-ahead / write-behind batching for this ring's per-plane store I/O (see appendInputPlane and
  // writeFinalPlane below). Each unbatched plane transfer moves only one output slice -- often close to
  // 1 MiB -- and spans just one or two chunks, below the OOC backend's parallel resident-chunk scatter gate
  // (four or more chunks, at least 1 MiB), so every such small transfer is served serially. Batching several
  // consecutive planes into one bulk copyIntoBuffer/copyFromBuffer call amortizes the fixed per-call cost and
  // gives the backend enough chunks to scatter in parallel.
  //
  // This is a SEPARATE, best-effort reservation from the ring/output-plane budget already validated above:
  // CalculateMorphologyCompositePipelineWorkingMemoryBytes, and the exact byte count callers reserve against
  // it, describe only the hard ring requirement, so requesting extra staging headroom here can never make an
  // otherwise-satisfiable call fail. A caller with no spare budget -- or one that has already exhausted the
  // process-wide working-memory ceiling -- simply gets 0 bytes back here, which collapses the staging depth
  // to one plane: today's unbatched per-plane transfer pattern.
  usize stagingPlanes = detail::k_CompositeStagingMinPlanes;
  usize stagingPlaneBytes = 0;
  CacheMemoryBudgetManager::WorkingMemoryReservation stagingReservation;
  if(detail::TryMultiplyMorphologySize(outputSliceValues, sizeof(T), stagingPlaneBytes) && stagingPlaneBytes > 0)
  {
    usize preferredSideBytes = 0;
    usize preferredTotalBytes = 0;
    if(detail::TryMultiplyMorphologySize(stagingPlaneBytes, detail::k_CompositeStagingPreferredPlanes, preferredSideBytes) &&
       detail::TryMultiplyMorphologySize(preferredSideBytes, usize{2}, preferredTotalBytes))
    {
      // One read-ahead buffer and one write-behind buffer, each sized for the preferred plane count.
      stagingReservation = ReserveWorkingMemory(preferredTotalBytes, preferredTotalBytes);
      const usize grantedPlanes = static_cast<usize>(stagingReservation.sizeBytes()) / (2 * stagingPlaneBytes);
      stagingPlanes = std::clamp(grantedPlanes, detail::k_CompositeStagingMinPlanes, detail::k_CompositeStagingPreferredPlanes);
      stagingReservation.shrinkTo(static_cast<uint64>(stagingPlanes * 2 * stagingPlaneBytes));
    }
  }
  // Never stage more planes than the volume has -- avoids over-allocating for small or thin volumes.
  stagingPlanes = std::min(stagingPlanes, std::max<usize>(dims[2], usize{1}));

  usize stagingBufferElements = 0;
  if(!detail::TryMultiplyMorphologySize(stagingPlanes, outputSliceValues, stagingBufferElements))
  {
    return MakeErrorResult(-8792, fmt::format("SafeBorder morphology-composite pipeline staging buffer size overflows. Dimensions: {}; staging planes: {}; output slice values: {}.",
                                              StringUtilities::formatDimensions3D(dims), stagingPlanes, outputSliceValues));
  }

  auto inputStagingBuffer = std::make_unique<T[]>(stagingBufferElements);
  usize inputStagingBase = 0;     // originalZ of the first plane currently resident in inputStagingBuffer
  usize inputStagingCount = 0;    // number of valid planes currently resident
  usize inputStagingConsumed = 0; // planes already scattered into the ring from the current load

  auto outputStagingBuffer = std::make_unique<T[]>(stagingBufferElements);
  usize outputStagingBase = 0;  // outputZ of the first plane pending flush
  usize outputStagingCount = 0; // number of valid planes pending flush

  const T padValue = firstOp == MorphOp::Erode ? std::numeric_limits<T>::max() : std::numeric_limits<T>::lowest();
  const bool firstDilate = firstOp == MorphOp::Dilate;
  const bool secondDilate = secondOp == MorphOp::Dilate;
  // The flat fold uses only its precomputed offsets. The moving-window fallback needs axis sets.
  const bool useFlatFold = detail::k_UseVectorHistogram<T> && detail::ShouldUseMorphologyResidentFlatFold(paddedDims, se);
  const std::vector<detail::MorphologyFlatFoldOffset> flatOffsets =
      useFlatFold ? detail::MakeMorphologyFlatFoldOffsets(se, paddedDims[0], paddedDims[1]) : std::vector<detail::MorphologyFlatFoldOffset>{};
  std::optional<detail::AxisAddRemoveSets> axisSets;
  if(!useFlatFold)
  {
    axisSets = detail::ComputeAxisAddRemoveSets(se);
  }
  // The first stage's output plane IS the padded plane (no crop), so its neighborhood center needs no
  // translation from output-local (ox, oy) -- unlike the second stage, which reads through cropOffset.
  const std::array<usize, 3> identityOrigin{0, 0, 0};
  usize inputZLo = 0;
  usize inputDepth = 0;
  usize firstZLo = 0;
  usize firstDepth = 0;

  // Flushes any output planes pending in outputStagingBuffer with one bulk copyFromBuffer call. Called both
  // when the write-behind buffer fills and at every early-return/completion point below, so a canceled or
  // completed run never leaves already-computed planes stranded in memory instead of reaching the store.
  const auto flushOutputStaging = [&]() -> Result<> {
    if(outputStagingCount == 0)
    {
      return {};
    }
    const usize flushedPlanes = outputStagingCount;
    outputStagingCount = 0;
    return output.copyFromBuffer(outputStagingBase * outputSliceValues, nonstd::span<const T>(outputStagingBuffer.get(), flushedPlanes * outputSliceValues));
  };

  const auto appendInputPlane = [&](usize paddedZ) -> Result<> {
    if(inputDepth == ringDepth)
    {
      std::memmove(inputRing.get(), inputRing.get() + paddedSliceValues, (ringDepth - 1) * paddedSliceValues * sizeof(T));
      ++inputZLo;
      --inputDepth;
    }
    T* plane = inputRing.get() + inputDepth * paddedSliceValues;
    std::fill_n(plane, paddedSliceValues, padValue);
    if(paddedZ >= cropOffset[2] && paddedZ < cropOffset[2] + dims[2])
    {
      const usize originalZ = paddedZ - cropOffset[2];
      if(inputStagingConsumed >= inputStagingCount)
      {
        // paddedZ advances exactly one plane per call, and the crop-offset skip region only ever precedes
        // this contiguous run of in-range planes, so originalZ always continues immediately where the
        // previous load left off: a fresh batch never needs to re-read or skip a plane.
        assert(originalZ == inputStagingBase + inputStagingCount && "input staging batch must continue contiguously from the previous load");
        const usize batchPlanes = std::min(stagingPlanes, dims[2] - originalZ);
        if(Result<> result = input.copyIntoBuffer(originalZ * outputSliceValues, nonstd::span<T>(inputStagingBuffer.get(), batchPlanes * outputSliceValues)); result.invalid())
        {
          return result;
        }
        inputStagingBase = originalZ;
        inputStagingCount = batchPlanes;
        inputStagingConsumed = 0;
      }
      const T* sourcePlane = inputStagingBuffer.get() + inputStagingConsumed * outputSliceValues;
      ++inputStagingConsumed;
      for(usize y = 0; y < dims[1]; ++y)
      {
        std::copy_n(sourcePlane + y * dims[0], dims[0], plane + (cropOffset[1] + y) * paddedDims[0] + cropOffset[0]);
      }
    }
    if(inputDepth == 0)
    {
      inputZLo = paddedZ;
    }
    ++inputDepth;
    return {};
  };

  const auto appendFirstPlane = [&](usize paddedZ) -> Result<> {
    if(firstDepth == ringDepth)
    {
      std::memmove(firstRing.get(), firstRing.get() + paddedSliceValues, (ringDepth - 1) * paddedSliceValues * sizeof(T));
      ++firstZLo;
      --firstDepth;
    }
    T* plane = firstRing.get() + firstDepth * paddedSliceValues;
    if(se.offsets.empty())
    {
      std::copy_n(inputRing.get() + (paddedZ - inputZLo) * paddedSliceValues, paddedSliceValues, plane);
    }
    else
    {
      if(useFlatFold)
      {
        if(firstDilate)
        {
          detail::RunMorphologyFlatFoldBlock<T, true>(inputRing.get(), plane, paddedDims, inputZLo, SizeVec3{paddedDims[0], paddedDims[1], 1}, SizeVec3{0, 0, paddedZ}, se, flatOffsets, shouldCancel);
        }
        else
        {
          detail::RunMorphologyFlatFoldBlock<T, false>(inputRing.get(), plane, paddedDims, inputZLo, SizeVec3{paddedDims[0], paddedDims[1], 1}, SizeVec3{0, 0, paddedZ}, se, flatOffsets, shouldCancel);
        }
      }
      else
      {
        if(firstDilate)
        {
          detail::RunMorphologyPlaneWindow<T, true>(inputRing.get(), plane, paddedDims[0], paddedDims[1], paddedDims[2], inputZLo, paddedDims[0], paddedDims[1], identityOrigin, paddedZ, se, *axisSets,
                                                    shouldCancel);
        }
        else
        {
          detail::RunMorphologyPlaneWindow<T, false>(inputRing.get(), plane, paddedDims[0], paddedDims[1], paddedDims[2], inputZLo, paddedDims[0], paddedDims[1], identityOrigin, paddedZ, se,
                                                     *axisSets, shouldCancel);
        }
      }
    }
    if(firstDepth == 0)
    {
      firstZLo = paddedZ;
    }
    ++firstDepth;
    return {};
  };

  const auto writeFinalPlane = [&](usize paddedFirstZ) -> Result<> {
    if(paddedFirstZ < radiusZ + cropOffset[2])
    {
      return {};
    }
    const usize outputZ = paddedFirstZ - radiusZ - cropOffset[2];
    if(outputZ >= dims[2])
    {
      return {};
    }
    const usize sourceCenterZ = paddedFirstZ - radiusZ;
    if(outputMode != detail::MorphologyCompositeOutputMode::Morphology && (sourceCenterZ < inputZLo || sourceCenterZ >= inputZLo + inputDepth))
    {
      return MakeErrorResult(
          -8481, fmt::format("SafeBorder morphology-composite pipeline lost the original crop plane before subtraction. Dimensions: {}; first plane: {}; source center: {}; input range: [{}, {}].",
                             StringUtilities::formatDimensions3D(dims), paddedFirstZ, sourceCenterZ, inputZLo, inputZLo + inputDepth));
    }
    // Completed output planes accumulate here and flush to the store in one bulk copyFromBuffer call every
    // stagingPlanes planes (or whatever remains at the end); see flushOutputStaging above. outputZ is always
    // contiguous with whatever is already pending, since firstZ (and therefore outputZ) strictly increases by
    // one across every call this lambda ever receives -- see the caller loops below.
    if(outputStagingCount == 0)
    {
      outputStagingBase = outputZ;
    }
    T* outputPlane = outputStagingBuffer.get() + outputStagingCount * outputSliceValues;
    if(se.offsets.empty())
    {
      const T* sourcePlane = firstRing.get() + (sourceCenterZ - firstZLo) * paddedSliceValues;
      for(usize y = 0; y < dims[1]; ++y)
      {
        std::copy_n(sourcePlane + (cropOffset[1] + y) * paddedDims[0] + cropOffset[0], dims[0], outputPlane + y * dims[0]);
      }
    }
    else
    {
      if(useFlatFold)
      {
        const SizeVec3 outputOrigin{cropOffset[0], cropOffset[1], cropOffset[2] + outputZ};
        if(secondDilate)
        {
          detail::RunMorphologyFlatFoldBlock<T, true>(firstRing.get(), outputPlane, paddedDims, firstZLo, SizeVec3{dims[0], dims[1], 1}, outputOrigin, se, flatOffsets, shouldCancel);
        }
        else
        {
          detail::RunMorphologyFlatFoldBlock<T, false>(firstRing.get(), outputPlane, paddedDims, firstZLo, SizeVec3{dims[0], dims[1], 1}, outputOrigin, se, flatOffsets, shouldCancel);
        }
      }
      else
      {
        if(secondDilate)
        {
          detail::RunMorphologyPlaneWindow<T, true>(firstRing.get(), outputPlane, paddedDims[0], paddedDims[1], paddedDims[2], firstZLo, dims[0], dims[1], cropOffset, outputZ, se, *axisSets,
                                                    shouldCancel);
        }
        else
        {
          detail::RunMorphologyPlaneWindow<T, false>(firstRing.get(), outputPlane, paddedDims[0], paddedDims[1], paddedDims[2], firstZLo, dims[0], dims[1], cropOffset, outputZ, se, *axisSets,
                                                     shouldCancel);
        }
      }
    }
    if(outputMode != detail::MorphologyCompositeOutputMode::Morphology)
    {
      const T* originalPlane = inputRing.get() + (sourceCenterZ - inputZLo) * paddedSliceValues;
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, dims[1]);
      parallelAlgorithm.execute([&](const Range& yRange) {
        if(shouldCancel)
        {
          return;
        }
        detail::SubtractMorphologyPlaneRows(originalPlane, outputPlane, paddedDims[0], dims[0], cropOffset[0], cropOffset[1], outputMode, yRange.min(), yRange.max());
      });
      if(shouldCancel)
      {
        return {};
      }
    }
    ++outputStagingCount;
    if(outputStagingCount == stagingPlanes)
    {
      return flushOutputStaging();
    }
    return {};
  };

  const usize paddedZCount = paddedDims[2];
  for(usize paddedZ = 0; paddedZ < paddedZCount; ++paddedZ)
  {
    if(shouldCancel)
    {
      // Persist whatever output planes already finished before giving up the rest of this run.
      return flushOutputStaging();
    }
    if(Result<> result = appendInputPlane(paddedZ); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return flushOutputStaging();
    }
    if(paddedZ >= radiusZ)
    {
      const usize firstZ = paddedZ - radiusZ;
      if(Result<> result = appendFirstPlane(firstZ); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return flushOutputStaging();
      }
      if(Result<> result = writeFinalPlane(firstZ); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return flushOutputStaging();
      }
    }
  }
  const usize firstTailBegin = paddedZCount > radiusZ ? paddedZCount - radiusZ : 0;
  for(usize firstZ = firstTailBegin; firstZ < paddedZCount; ++firstZ)
  {
    if(shouldCancel)
    {
      // Persist whatever output planes already finished before giving up the rest of this run.
      return flushOutputStaging();
    }
    if(Result<> result = appendFirstPlane(firstZ); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return flushOutputStaging();
    }
    if(Result<> result = writeFinalPlane(firstZ); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return flushOutputStaging();
    }
  }
  if(!safeBorder)
  {
    if(radiusZ > std::numeric_limits<usize>::max() - paddedZCount)
    {
      return MakeErrorResult(-8481, fmt::format("SafeBorder=false morphology-composite virtual tail index overflows. Padded Z count: {}; Z radius: {}; dimensions: {}.", paddedZCount, radiusZ,
                                                StringUtilities::formatDimensions3D(dims)));
    }
    const usize virtualFirstZEnd = paddedZCount + radiusZ;
    for(usize virtualFirstZ = paddedZCount; virtualFirstZ < virtualFirstZEnd; ++virtualFirstZ)
    {
      if(shouldCancel)
      {
        // Persist whatever output planes already finished before giving up the rest of this run.
        return flushOutputStaging();
      }
      if(Result<> result = writeFinalPlane(virtualFirstZ); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return flushOutputStaging();
      }
    }
  }
  return flushOutputStaging();
}

/**
 * @brief Out-of-core-safe FUSED morphological-GRADIENT (Scanline path).
 *
 * For true 3D, each batch bulk-reads a clamped Z slab with its complete halo. Moderate multi-plane
 * 8-bit neighborhoods use @ref detail::RunMorphologyGradientFlatFoldBlock, which folds the extrema in
 * separate contiguous interior X loops with SIMD operations and keeps checked scalar borders. Other cases use
 * @ref detail::RunMorphologyGradientSlabWindow. Both routes use global bounds and write the same gradient.
 * For true 2D, @ref detail::ExecuteBoundedMorphology2D gathers each neighborhood from a bounded buffer.
 *
 * The algorithm keeps store I/O serial. Only staged-buffer work runs in parallel. The 3D planner targets
 * about 16 MiB. One output plane and its complete halo remain resident, so wide planes can exceed the target.
 * Its required constructor arguments match @ref
 * MorphGradientDirect for @ref DispatchAlgorithm.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class MorphGradientScanline
{
public:
  MorphGradientScanline(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const std::atomic_bool& shouldCancel,
                        const IFilter::MessageHandler& messageHandler, usize targetBufferBytes = detail::k_MorphologySlabTargetBytes)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_TargetBufferBytes(targetBufferBytes)
  {
  }

  ~MorphGradientScanline() = default;

  MorphGradientScanline(const MorphGradientScanline&) = delete;
  MorphGradientScanline(MorphGradientScanline&&) noexcept = delete;
  MorphGradientScanline& operator=(const MorphGradientScanline&) = delete;
  MorphGradientScanline& operator=(MorphGradientScanline&&) noexcept = delete;

  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    // The SE's z-offsets span [-rz, rz]; a negative radius means an empty axis, so clamp to 0 for the slab
    // bounds (the offset list is empty in that degenerate case anyway).
    const usize rz = (m_SE.radius[2] > 0) ? static_cast<usize>(m_SE.radius[2]) : 0;
    detail::MorphologySlabPlan plan;
    if(Result<> validation = detail::MakeMorphologySlabPlan(m_In, m_Out, m_Dims, rz, m_TargetBufferBytes, plan); validation.invalid())
    {
      return validation;
    }
    if(plan.volumeValues == 0)
    {
      return {};
    }
    const usize sliceValues = plan.sliceValues;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying morphology filter: {:.1f}%");
    auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    if(dimZ == 1)
    {
      if(m_SE.offsets.empty())
      {
        const usize targetBytes = m_TargetBufferBytes == detail::k_MorphologySlabTargetBytes ? 64ULL * 1024ULL * 1024ULL : m_TargetBufferBytes;
        const usize targetValues = targetBytes / sizeof(T);
        if(targetValues == 0)
        {
          return MakeErrorResult(-8655, "The bounded 2-D morphology target cannot hold one value.");
        }

        const usize chunkValues = std::min(plan.volumeValues, targetValues);
        auto outputBuffer = std::make_unique<T[]>(chunkValues);
        std::fill_n(outputBuffer.get(), chunkValues, static_cast<T>(0));
        for(usize start = 0; start < plan.volumeValues; start += chunkValues)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const usize count = std::min(chunkValues, plan.volumeValues - start);
          if(Result<> result = m_Out.copyFromBuffer(start, nonstd::span<const T>(outputBuffer.get(), count)); result.invalid())
          {
            return result;
          }
        }
        progressMessenger.sendProgressMessage(1);
        return {};
      }

      Result<> result =
          detail::ExecuteBoundedMorphology2D<T>(m_In, m_Out, m_Dims, m_SE, m_ShouldCancel, m_TargetBufferBytes,
                                                [&](const T* input, T* output, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
                                                  return detail::MorphGradient2DBlockBody<T>{.input = input,
                                                                                             .output = output,
                                                                                             .offsets = m_SE.offsets.data(),
                                                                                             .numOffsets = m_SE.offsets.size(),
                                                                                             .dimX = dimX,
                                                                                             .dimY = dimY,
                                                                                             .inputXBegin = inputXBegin,
                                                                                             .inputYBegin = inputYBegin,
                                                                                             .inputWidth = inputWidth,
                                                                                             .outputXBegin = outputXBegin,
                                                                                             .outputYBegin = outputYBegin,
                                                                                             .outputWidth = outputWidth};
                                                });
      if(result.valid() && !m_ShouldCancel)
      {
        progressMessenger.sendProgressMessage(1);
      }
      return result;
    }

    auto outSlab = std::make_unique<T[]>(plan.outputBatchDepth * sliceValues);

    // Degenerate structuring element (empty offset list, only reachable for a radius-0 Annulus): the gradient
    // is dilate(passthrough) - erode(passthrough) == in - in == 0 at every voxel, so emit zeros -- byte-for-byte
    // identical to the old dilate/erode passthrough + SubtractStores. Bounded: one output batch, reused across Z.
    if(m_SE.offsets.empty())
    {
      std::fill(outSlab.get(), outSlab.get() + plan.outputBatchDepth * sliceValues, static_cast<T>(0));
      for(usize zBegin = 0; zBegin < dimZ; zBegin += plan.outputBatchDepth)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize outputDepth = std::min(plan.outputBatchDepth, dimZ - zBegin);
        if(Result<> r = m_Out.copyFromBuffer(zBegin * sliceValues, nonstd::span<const T>(outSlab.get(), outputDepth * sliceValues)); r.invalid())
        {
          return r;
        }
        progressMessenger.sendProgressMessage(outputDepth);
      }
      return {};
    }

    // The flat fold uses only its precomputed offsets. The moving-window fallback needs axis sets.
    const bool useFlatFold = detail::k_UseVectorHistogram<T> && detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE);
    const std::vector<detail::MorphologyFlatFoldOffset> flatOffsets = useFlatFold ? detail::MakeMorphologyFlatFoldOffsets(m_SE, dimX, dimY) : std::vector<detail::MorphologyFlatFoldOffset>{};
    std::optional<detail::AxisAddRemoveSets> axisSets;
    if(!useFlatFold)
    {
      axisSets = detail::ComputeAxisAddRemoveSets(m_SE);
    }

    // Reuse one bounded input halo slab across all output batches (see MorphScanline).
    auto slab = std::make_unique<T[]>(plan.maxInputDepth * sliceValues);
    usize loadedZLo = 0;
    usize loadedDepth = 0;

    for(usize zBegin = 0; zBegin < dimZ; zBegin += plan.outputBatchDepth)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize outputDepth = std::min(plan.outputBatchDepth, dimZ - zBegin);
      const usize zEnd = zBegin + outputDepth;
      const usize zLo = (zBegin >= rz) ? (zBegin - rz) : 0;
      const usize zHi = (zEnd - 1) + std::min(rz, dimZ - zEnd);
      const usize slabDepth = zHi - zLo + 1;
      if(loadedDepth == 0)
      {
        if(Result<> r = m_In.copyIntoBuffer(zLo * sliceValues, nonstd::span<T>(slab.get(), slabDepth * sliceValues)); r.invalid())
        {
          return r;
        }
      }
      else
      {
        const usize loadedZHi = loadedZLo + loadedDepth - 1;
        const usize retainedZLo = std::max(zLo, loadedZLo);
        const usize retainedZHi = std::min(zHi, loadedZHi);
        const usize retainedDepth = retainedZLo <= retainedZHi ? retainedZHi - retainedZLo + 1 : 0;
        if(retainedDepth > 0)
        {
          std::memmove(slab.get(), slab.get() + (retainedZLo - loadedZLo) * sliceValues, retainedDepth * sliceValues * sizeof(T));
        }
        const usize readZBegin = retainedDepth == 0 ? zLo : retainedZHi + 1;
        if(readZBegin <= zHi)
        {
          const usize readDepth = zHi - readZBegin + 1;
          if(Result<> r = m_In.copyIntoBuffer(readZBegin * sliceValues, nonstd::span<T>(slab.get() + retainedDepth * sliceValues, readDepth * sliceValues)); r.invalid())
          {
            return r;
          }
        }
      }
      loadedZLo = zLo;
      loadedDepth = slabDepth;

      if(useFlatFold)
      {
        detail::RunMorphologyGradientFlatFoldBlock<T>(slab.get(), outSlab.get(), m_Dims, zLo, SizeVec3{dimX, dimY, outputDepth}, SizeVec3{0, 0, zBegin}, m_SE, flatOffsets, m_ShouldCancel);
      }
      else
      {
        detail::RunMorphologyGradientSlabWindow<T>(slab.get(), outSlab.get(), dimX, dimY, dimZ, zLo, zBegin, outputDepth, m_SE, *axisSets, m_ShouldCancel);
      }

      if(m_ShouldCancel)
      {
        return {};
      }
      if(Result<> r = m_Out.copyFromBuffer(zBegin * sliceValues, nonstd::span<const T>(outSlab.get(), outputDepth * sliceValues)); r.invalid())
      {
        return r;
      }
      progressMessenger.sendProgressMessage(outputDepth);
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_TargetBufferBytes;
};

/**
 * @brief Resident-buffer adaptive FUSED morphological-GRADIENT (Direct path): the FAST engine.
 *
 * The both-extremes analogue of @ref MorphDirect. Moderate kernels on multi-plane 8-bit images use separate
 * parallel flat-offset folds for each extrema across contiguous interior X rows with SIMD operations. A
 * reusable thread-local O(row) buffer holds the minimum values, and checked scalar folds preserve borders.
 * Other inputs reuse that class's moving-window machinery -- the
 * whole array is pulled into a flat buffer once, the per-axis
 * "entering"/"leaving" offset sets are built once via @ref detail::ComputeAxisAddRemoveSets (shared with
 * @ref MorphDirect), and three nested windows (histZ/histY/histX) snake across the volume folding in only the
 * voxels that ENTER and out only those that LEAVE -- but it slides a @ref detail::MorphGradientHistogram, which
 * tracks BOTH the window max and min, so @ref detail::MorphGradientMapHistogram::getGradient returns
 * @c max - min directly. This fuses the former dilate-pass + erode-pass + SubtractStores into ONE moving pass.
 *
 * Boundary. An out-of-image SE neighbor wins neither extreme (@c lowest() on the max side, @c max() on the min
 * side), so the fused accumulator's addBoundary/removeBoundary are no-ops; a voxel with NO in-bounds neighbor
 * therefore yields @c static_cast<T>(lowest() - max()), bit-identical to the 2-pass dilate(lowest()) -
 * erode(max()) (see @ref MorphOp and @ref detail::MorphGradientMapHistogram). The empty-SE (radius-0 Annulus)
 * case emits 0 everywhere (== in - in), matching @ref MorphGradientScanline so the two paths agree.
 *
 * Its required constructor arguments match @ref MorphGradientScanline so both drop into @ref DispatchAlgorithm.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class MorphGradientDirect
{
public:
  MorphGradientDirect(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const std::atomic_bool& shouldCancel,
                      const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  ~MorphGradientDirect() = default;

  MorphGradientDirect(const MorphGradientDirect&) = delete;
  MorphGradientDirect(MorphGradientDirect&&) noexcept = delete;
  MorphGradientDirect& operator=(const MorphGradientDirect&) = delete;
  MorphGradientDirect& operator=(MorphGradientDirect&&) noexcept = delete;

  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    const usize sliceValues = dimX * dimY;
    const usize vol = sliceValues * dimZ;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying morphology filter: {:.1f}%");

    // Degenerate structuring element (empty offset list, only reachable for a radius-0 Annulus): the gradient
    // is in - in == 0 at every voxel, byte-for-byte identical to MorphGradientScanline. This rare edge case
    // stays serial; its output-plane scratch lives here so the parallel fast path below never allocates it.
    if(m_SE.offsets.empty())
    {
      auto outPlane = std::make_unique<T[]>(sliceValues);
      std::fill(outPlane.get(), outPlane.get() + sliceValues, static_cast<T>(0));
      for(usize z = 0; z < dimZ; ++z)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        if(Result<> r = m_Out.copyFromBuffer(z * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
        {
          return r;
        }
      }
      return {};
    }

    // Pull the whole array into a flat local buffer once for free random SE access. This is a memcpy in-core
    // and one bulk read when the working-memory wrapper selects this route for a chunked array.
    auto data = std::make_unique<T[]>(vol);
    if(Result<> r = m_In.copyIntoBuffer(0, nonstd::span<T>(data.get(), vol)); r.invalid())
    {
      return r;
    }
    const T* in = data.get();

    if constexpr(detail::k_UseVectorHistogram<T>)
    {
      if(detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE))
      {
        auto output = std::make_unique<T[]>(vol);
        detail::RunMorphologyGradientFlatFoldBlock<T>(in, output.get(), m_Dims, 0, m_Dims, SizeVec3{0, 0, 0}, m_SE, m_ShouldCancel);
        if(m_ShouldCancel)
        {
          return {};
        }
        return m_Out.copyFromBuffer(0, nonstd::span<const T>(output.get(), vol));
      }
    }

    // Per-axis "entering"/"leaving" offset sets for a +1 step, shared with MorphDirect (see
    // detail::ComputeAxisAddRemoveSets). The two sets have equal size, so the window always holds |SE| entries.
    const detail::AxisAddRemoveSets axisSets = detail::ComputeAxisAddRemoveSets(m_SE);
    const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
    const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;

    using Hist = detail::MorphGradientHistogram<T>;

    const int64 nX = static_cast<int64>(dimX);
    const int64 nY = static_cast<int64>(dimY);
    const int64 nZ = static_cast<int64>(dimZ);
    // Per-axis radius (clamped >= 0). Any added/removed offset reaches at most r_a + 1 along axis a, so a center
    // whose (r_a + 1)-padded neighborhood is fully in bounds needs no per-voxel checks.
    const int64 rX = (m_SE.radius[0] > 0) ? m_SE.radius[0] : 0;
    const int64 rY = (m_SE.radius[1] > 0) ? m_SE.radius[1] : 0;
    const int64 rZ = (m_SE.radius[2] > 0) ? m_SE.radius[2] : 0;

    // PRECONDITION for the unchecked fast path below: every SE offset lies within its per-axis radius box, i.e.
    // offsets subseteq [-radius, radius]^3 (see MorphDirect; MakeStructuringElement always honors it).
#ifndef NDEBUG
    for(const SEOffset& o : m_SE.offsets)
    {
      assert(o.dx >= -rX && o.dx <= rX && o.dy >= -rY && o.dy <= rY && o.dz >= -rZ && o.dz <= rZ && "SE offset outside its radius box; fast-path bounds assumption violated");
    }
#endif

    auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
    auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

    // Slide the fused histogram from center (cx,cy,cz) by +1 along axis a: fold in the entering voxels, fold out
    // the leaving ones. The fast path (whole padded neighborhood in bounds) skips per-voxel checks; the slow
    // path drops OOB neighbors (addBoundary/removeBoundary are no-ops for the gradient, since a boundary neighbor
    // wins neither extreme -- == MorphGradientScanline skipping them).
    auto push = [&](Hist& h, int64 cx, int64 cy, int64 cz, int a) {
      const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
      if(fast)
      {
        for(const SEOffset& q : added[a])
        {
          h.addPixel(in[flat(cx + q.dx, cy + q.dy, cz + q.dz)]);
        }
        for(const SEOffset& o : removed[a])
        {
          h.removePixel(in[flat(cx + o.dx, cy + o.dy, cz + o.dz)]);
        }
      }
      else
      {
        for(const SEOffset& q : added[a])
        {
          const int64 x = cx + q.dx;
          const int64 y = cy + q.dy;
          const int64 z = cz + q.dz;
          if(inBounds(x, y, z))
          {
            h.addPixel(in[flat(x, y, z)]);
          }
          else
          {
            h.addBoundary();
          }
        }
        for(const SEOffset& o : removed[a])
        {
          const int64 x = cx + o.dx;
          const int64 y = cy + o.dy;
          const int64 z = cz + o.dz;
          if(inBounds(x, y, z))
          {
            h.removePixel(in[flat(x, y, z)]);
          }
          else
          {
            h.removeBoundary();
          }
        }
      }
    };

    // Seed a plane-start window at corner (0,0,zStart) -- a one-time O(|SE|) init, so each parallel Z-slab worker
    // below builds its OWN starting histogram independently instead of carrying one across slabs.
    auto seedHist = [&](Hist& h, int64 zStart) {
      for(const SEOffset& o : m_SE.offsets)
      {
        const int64 x = o.dx;
        const int64 y = o.dy;
        const int64 z = zStart + o.dz;
        if(inBounds(x, y, z))
        {
          h.addPixel(in[flat(x, y, z)]);
        }
        else
        {
          h.addBoundary();
        }
      }
    };

    // Parallelize the moving-histogram traversal across contiguous Z-slabs (mirrors MorphDirect). Every output
    // voxel is an order-independent max/min reduction over the READ-ONLY input `in`, and each worker owns its
    // output plane plus its three nested histograms and writes only its own disjoint Z-planes, so threading
    // cannot change any value. Direct is chosen only for in-core stores; requireStoresInMemory keeps
    // parallelization on for them (and defensively serializes were a store ever out-of-core).
    std::atomic<bool> hasError{false};
    std::mutex resultMutex;
    Result<> firstError;

    auto processZRange = [&](const Range& zRange) {
      const usize zBegin = zRange.min();
      const usize zEnd = zRange.max();
      if(zBegin >= zEnd)
      {
        return;
      }
      // Per-worker scratch: its own output plane, its own ProgressMessenger (the messenger is not shared-thread-
      // safe, but the progress COUNTER it feeds is atomic), and its own three nested histograms.
      auto outPlane = std::make_unique<T[]>(sliceValues);
      auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

      // Seed this slab's plane-start window at its OWN origin corner (0,0,zBegin), independent of every other
      // slab. histY (line start) and histX (working window) are declared ONCE per worker and reset by assignment
      // below, reusing their capacity instead of reallocating a histogram every line/plane.
      Hist histZ;
      seedHist(histZ, static_cast<int64>(zBegin));
      Hist histY;
      Hist histX;

      for(usize zc = zBegin; zc < zEnd; ++zc)
      {
        if(m_ShouldCancel)
        {
          return;
        }
        const int64 z = static_cast<int64>(zc);
        if(zc > zBegin)
        {
          push(histZ, 0, 0, z - 1, 2);
        }
        histY = histZ;
        for(int64 y = 0; y < nY; ++y)
        {
          if(y > 0)
          {
            push(histY, 0, y - 1, z, 1);
          }
          histX = histY;
          for(int64 x = 0; x < nX; ++x)
          {
            outPlane[static_cast<usize>(y * nX + x)] = histX.getGradient();
            if(x < nX - 1)
            {
              push(histX, x, y, z, 0);
            }
          }
        }
        if(Result<> r = m_Out.copyFromBuffer(zc * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
        {
          const std::lock_guard<std::mutex> lock(resultMutex);
          if(!hasError)
          {
            firstError = std::move(r);
            hasError = true;
          }
          return;
        }
        progressMessenger.sendProgressMessage(1);
      }
    };

    ParallelDataAlgorithm parallelAlgorithm;
    IParallelAlgorithm::AlgorithmStores algStores{&m_In, &m_Out};
    parallelAlgorithm.requireStoresInMemory(algStores);
    parallelAlgorithm.setRange(0, dimZ);
    parallelAlgorithm.execute(processZRange);

    if(hasError)
    {
      return firstError;
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

template <class T>
class MorphGradientWorkingMemory
{
public:
  MorphGradientWorkingMemory(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    auto allocationResult = detail::ReserveMorphologyResidentWorkingMemory<T>(m_Dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if constexpr(detail::k_UseVectorHistogram<T>)
    {
      if(detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE) && allocation.holdsCompleteState())
      {
        try
        {
          return MorphGradientDirect<T>{m_In, m_Out, m_Dims, m_SE, m_ShouldCancel, m_MessageHandler}();
        } catch(const std::bad_alloc&)
        {
          // Reuse the complete reservation for the bounded scanline fallback.
        }
      }
    }
    if(allocation.reservation.sizeBytes() > 0 && allocation.reservation.sizeBytes() <= std::numeric_limits<usize>::max())
    {
      detail::MorphologySlabPlan plan;
      const usize radiusZ = m_SE.radius[2] > 0 ? static_cast<usize>(m_SE.radius[2]) : 0;
      if(Result<> result = detail::MakeMorphologySlabPlan(m_In, m_Out, m_Dims, radiusZ, static_cast<usize>(allocation.reservation.sizeBytes()), plan); result.invalid())
      {
        return result;
      }
      usize residentValues = 0;
      usize residentBytes = 0;
      if(plan.volumeValues > 0 && plan.maxInputDepth <= std::numeric_limits<usize>::max() - plan.outputBatchDepth)
      {
        const usize residentPlaneCount = plan.maxInputDepth + plan.outputBatchDepth;
        if(detail::TryMultiplyMorphologySize(residentPlaneCount, plan.sliceValues, residentValues) && detail::TryMultiplyMorphologySize(residentValues, sizeof(T), residentBytes))
        {
          allocation.reservation.shrinkTo(static_cast<uint64>(residentBytes));
        }
      }
      return MorphGradientScanline<T>{m_In, m_Out, m_Dims, m_SE, m_ShouldCancel, m_MessageHandler, static_cast<usize>(allocation.reservation.sizeBytes())}();
    }
    return MorphGradientScanline<T>{m_In, m_Out, m_Dims, m_SE, m_ShouldCancel, m_MessageHandler}();
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Public entry point for the FUSED grayscale morphological-gradient engine.
 *
 * Dispatches on storage type via @ref DispatchAlgorithm. In-core arrays use @ref MorphGradientDirect. Chunked
 * arrays use a complete resident 8-bit state only when the shared working-memory budget grants it; otherwise
 * they use @ref MorphGradientScanline. Both routes compute @c max(window) - min(window) per voxel in a SINGLE
 * pass and produce bit-identical output -- to each other, and to the former dilate + erode + SubtractStores
 * three-pass sequence.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
Result<> ApplyMorphologyGradient(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const IDataArray& inArray, const IDataArray& outArray,
                                 const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  return DispatchAlgorithm<MorphGradientDirect<T>, MorphGradientWorkingMemory<T>>({&inArray, &outArray}, in, out, dims, se, shouldCancel, messageHandler);
}

/**
 * @brief Out-of-core-safe BINARY morphology (Scanline path).
 *
 * For true 3D, each output batch bulk-reads one clamped Z slab with the batch's complete shared halo.
 * Moderate 8-bit neighborhoods pack each staged slab once and use
 * @ref detail::RunPrepackedBinaryMorphologyBlock. Other cases route by op:
 *  - Dilate slides @ref detail::RunBinaryMorphologySlabWindow's moving fg-count window across the staged
 *    slab -- the SAME window @ref BinaryMorphDirect slides across an in-core volume -- folding in only the
 *    neighbors entering/leaving the window per step rather than testing every structuring-element offset at
 *    every voxel.
 *  - Erode instead gathers each voxel's full neighborhood directly via @ref detail::BinaryMorphPlaneBody's
 *    all() predicate, which SHORT-CIRCUITS at the first non-foreground neighbor. On typical sparse-foreground
 *    data that early exit ends most Erode scans almost immediately, beating the sliding window's unconditional
 *    per-step add/remove work; Dilate's any() short-circuit is comparatively rare to trigger early on the same
 *    data (foreground is sparse, so most windows scan to the end before finding one), so its sliding window
 *    has no equally cheap early-exit to lose and the window still wins there. Both routings are measured
 *    decisions, not a stylistic split.
 *
 * All routes fold the identical global-bounds neighborhood via any() (Dilate) or all() (Erode), and an
 * out-of-bounds SE neighbor is foreground iff @p boundaryToForeground (see @ref detail::FgCountAccumulator /
 * @ref detail::BinaryMorphPlaneBody), so their output is bit-identical. Membership is equality-based, so an
 * arbitrary-label input is handled correctly and the output is strictly @c {fg, bg}. For true 2D, @ref
 * detail::ExecuteBoundedMorphology2D still gathers each voxel's neighborhood directly from a bounded row/tile
 * buffer via @ref detail::BinaryMorphology2DBlockBody.
 *
 * The algorithm keeps store I/O serial. Only staged-buffer work runs in parallel. The 3D planner targets
 * about 16 MiB. One output plane and its complete halo remain resident, so wide planes can exceed the target.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class BinaryMorphScanline
{
public:
  BinaryMorphScanline(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, T foreground, T background, bool boundaryToForeground,
                      const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize targetBufferBytes = detail::k_MorphologySlabTargetBytes)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_Op(op)
  , m_Fg(foreground)
  , m_Bg(background)
  , m_BoundaryToForeground(boundaryToForeground)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_TargetBufferBytes(targetBufferBytes)
  {
  }

  ~BinaryMorphScanline() = default;

  BinaryMorphScanline(const BinaryMorphScanline&) = delete;
  BinaryMorphScanline(BinaryMorphScanline&&) noexcept = delete;
  BinaryMorphScanline& operator=(const BinaryMorphScanline&) = delete;
  BinaryMorphScanline& operator=(BinaryMorphScanline&&) noexcept = delete;

  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    // The SE's z-offsets span [-rz, rz]; a negative radius means an empty axis, so clamp to 0 for the
    // slab bounds (the offset list is empty in that degenerate case anyway).
    const usize rz = (m_SE.radius[2] > 0) ? static_cast<usize>(m_SE.radius[2]) : 0;
    const bool dilate = (m_Op == MorphOp::Dilate);
    detail::MorphologySlabPlan plan;
    if(Result<> validation = detail::MakeMorphologySlabPlan(m_In, m_Out, m_Dims, rz, m_TargetBufferBytes, plan); validation.invalid())
    {
      return validation;
    }
    if(plan.volumeValues == 0)
    {
      return {};
    }
    const usize sliceValues = plan.sliceValues;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying morphology filter: {:.1f}%");
    auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    if(dimZ == 1)
    {
      Result<> result =
          detail::ExecuteBoundedMorphology2D<T>(m_In, m_Out, m_Dims, m_SE, m_ShouldCancel, m_TargetBufferBytes,
                                                [&](const T* input, T* output, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
                                                  return detail::BinaryMorphology2DBlockBody<T>{.input = input,
                                                                                                .output = output,
                                                                                                .offsets = m_SE.offsets.data(),
                                                                                                .numOffsets = m_SE.offsets.size(),
                                                                                                .dimX = dimX,
                                                                                                .dimY = dimY,
                                                                                                .inputXBegin = inputXBegin,
                                                                                                .inputYBegin = inputYBegin,
                                                                                                .inputWidth = inputWidth,
                                                                                                .outputXBegin = outputXBegin,
                                                                                                .outputYBegin = outputYBegin,
                                                                                                .outputWidth = outputWidth,
                                                                                                .dilate = dilate,
                                                                                                .foreground = m_Fg,
                                                                                                .background = m_Bg,
                                                                                                .boundaryToForeground = m_BoundaryToForeground};
                                                });
      if(result.valid() && !m_ShouldCancel)
      {
        progressMessenger.sendProgressMessage(1);
      }
      return result;
    }

    auto outSlab = std::make_unique<T[]>(plan.outputBatchDepth * sliceValues);

    // Degenerate structuring element (empty offset list, only reachable for a radius-0 Annulus): there are
    // no neighbors to count, so pass the input through unchanged, mirroring MorphScanline's degenerate-SE
    // handling. Bounded: one output batch, reused across Z.
    if(m_SE.offsets.empty())
    {
      for(usize zBegin = 0; zBegin < dimZ; zBegin += plan.outputBatchDepth)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize outputDepth = std::min(plan.outputBatchDepth, dimZ - zBegin);
        const usize outputValues = outputDepth * sliceValues;
        if(Result<> r = m_In.copyIntoBuffer(zBegin * sliceValues, nonstd::span<T>(outSlab.get(), outputValues)); r.invalid())
        {
          return r;
        }
        if(m_ShouldCancel)
        {
          return {};
        }
        if(Result<> r = m_Out.copyFromBuffer(zBegin * sliceValues, nonstd::span<const T>(outSlab.get(), outputValues)); r.invalid())
        {
          return r;
        }
        progressMessenger.sendProgressMessage(outputDepth);
      }
      return {};
    }

    // The packed fold operates on one bit per staged voxel. The moving-window fallback needs axis sets.
    const bool useFlatFold = detail::k_UseVectorHistogram<T> && detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE);
    std::optional<detail::AxisAddRemoveSets> axisSets;
    if(!useFlatFold)
    {
      axisSets = detail::ComputeAxisAddRemoveSets(m_SE);
    }

    // Reuse one bounded input halo slab across all output batches (see MorphScanline).
    auto slab = std::make_unique<T[]>(plan.maxInputDepth * sliceValues);
    detail::PackedBinaryPlaneLayout packedLayout;
    std::vector<detail::BinaryMorphWord> packedSlab;
    if(useFlatFold)
    {
      packedLayout = detail::MakePackedBinaryPlaneLayout(m_Dims);
      usize packedSlabWords = 0;
      if(!detail::TryMultiplyMorphologySize(packedLayout.planeWords, plan.maxInputDepth, packedSlabWords))
      {
        return MakeErrorResult(-8660, fmt::format("Packed binary morphology requires {} staged planes with image dimensions ({}). This overflows the addressable word count.", plan.maxInputDepth,
                                                  StringUtilities::formatDimensions3D(m_Dims)));
      }
      packedSlab.resize(packedSlabWords);
    }
    usize loadedZLo = 0;
    usize loadedDepth = 0;

    for(usize zBegin = 0; zBegin < dimZ; zBegin += plan.outputBatchDepth)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize outputDepth = std::min(plan.outputBatchDepth, dimZ - zBegin);
      const usize zEnd = zBegin + outputDepth;
      const usize zLo = (zBegin >= rz) ? (zBegin - rz) : 0;
      const usize zHi = (zEnd - 1) + std::min(rz, dimZ - zEnd);
      const usize slabDepth = zHi - zLo + 1;
      if(loadedDepth == 0)
      {
        if(Result<> r = m_In.copyIntoBuffer(zLo * sliceValues, nonstd::span<T>(slab.get(), slabDepth * sliceValues)); r.invalid())
        {
          return r;
        }
      }
      else
      {
        const usize loadedZHi = loadedZLo + loadedDepth - 1;
        const usize retainedZLo = std::max(zLo, loadedZLo);
        const usize retainedZHi = std::min(zHi, loadedZHi);
        const usize retainedDepth = retainedZLo <= retainedZHi ? retainedZHi - retainedZLo + 1 : 0;
        if(retainedDepth > 0)
        {
          std::memmove(slab.get(), slab.get() + (retainedZLo - loadedZLo) * sliceValues, retainedDepth * sliceValues * sizeof(T));
        }
        const usize readZBegin = retainedDepth == 0 ? zLo : retainedZHi + 1;
        if(readZBegin <= zHi)
        {
          const usize readDepth = zHi - readZBegin + 1;
          if(Result<> r = m_In.copyIntoBuffer(readZBegin * sliceValues, nonstd::span<T>(slab.get() + retainedDepth * sliceValues, readDepth * sliceValues)); r.invalid())
          {
            return r;
          }
        }
      }
      loadedZLo = zLo;
      loadedDepth = slabDepth;

      if(useFlatFold)
      {
        if(!detail::PackBinaryMorphologyStagedPlanes(slab.get(), packedSlab.data(), packedLayout, slabDepth, m_Fg, m_ShouldCancel))
        {
          return {};
        }
        if(dilate)
        {
          detail::RunPrepackedBinaryMorphologyBlock<T, true>(packedSlab.data(), outSlab.get(), m_Dims, zLo, slabDepth, SizeVec3{dimX, dimY, outputDepth}, SizeVec3{0, 0, zBegin}, m_SE, m_Fg, m_Bg,
                                                             m_BoundaryToForeground, m_ShouldCancel);
        }
        else
        {
          detail::RunPrepackedBinaryMorphologyBlock<T, false>(packedSlab.data(), outSlab.get(), m_Dims, zLo, slabDepth, SizeVec3{dimX, dimY, outputDepth}, SizeVec3{0, 0, zBegin}, m_SE, m_Fg, m_Bg,
                                                              m_BoundaryToForeground, m_ShouldCancel);
        }
      }
      else
      {
        // Dilate uses the moving fg-count window. Erode gathers because its all() fold commonly
        // stops at the first background value.
        if(dilate)
        {
          detail::RunBinaryMorphologySlabWindow<T, true>(slab.get(), outSlab.get(), dimX, dimY, dimZ, zLo, zBegin, outputDepth, m_SE, *axisSets, m_Fg, m_Bg, m_BoundaryToForeground, m_ShouldCancel);
        }
        else
        {
          ParallelDataAlgorithm parallelAlgorithm;
          parallelAlgorithm.setRange(0, outputDepth * sliceValues);
          parallelAlgorithm.execute(detail::BinaryMorphPlaneBody<T>{.slab = slab.get(),
                                                                    .outSlab = outSlab.get(),
                                                                    .offsets = m_SE.offsets.data(),
                                                                    .numOffsets = m_SE.offsets.size(),
                                                                    .dimX = dimX,
                                                                    .dimY = dimY,
                                                                    .dimZ = dimZ,
                                                                    .zLo = zLo,
                                                                    .zBegin = zBegin,
                                                                    .dilate = dilate,
                                                                    .fg = m_Fg,
                                                                    .bg = m_Bg,
                                                                    .boundaryToForeground = m_BoundaryToForeground});
        }
      }

      if(m_ShouldCancel)
      {
        return {};
      }
      if(Result<> r = m_Out.copyFromBuffer(zBegin * sliceValues, nonstd::span<const T>(outSlab.get(), outputDepth * sliceValues)); r.invalid())
      {
        return r;
      }
      progressMessenger.sendProgressMessage(outputDepth);
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  MorphOp m_Op;
  T m_Fg;
  T m_Bg;
  bool m_BoundaryToForeground;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_TargetBufferBytes;
};

/**
 * @brief Resident-buffer adaptive BINARY morphology (Direct path): the FAST binary engine.
 *
 * The binary analogue of @ref MorphDirect. Moderate kernels on multi-plane 8-bit images pack equality
 * membership into portable 64-bit X-row words. The parallel fold uses any/all word operations.
 * Other inputs use that class's moving-window machinery. The whole array is pulled into a flat buffer.
 * The per-axis "entering"/"leaving" offset sets are built once via @ref detail::ComputeAxisAddRemoveSets
 * (shared with @ref MorphDirect), and
 * three nested windows (histZ/histY/histX, here accZ/accY/accX) snake across the volume folding in only the
 * voxels that ENTER and out only those that LEAVE -- but it slides a @ref detail::FgCountAccumulator (a single
 * foreground counter) instead of a min/max histogram. That drops the per-step cost to O(perimeter) integer
 * increments, and the accumulator's @ref detail::FgCountAccumulator::getValue applies the pinned binary
 * formula: Dilate emits fg when ANY window voxel is foreground (count > 0), Erode only when ALL are
 * (count == |SE|). Membership is equality-based, so arbitrary non-{fg,bg} labels count as background and the
 * output is strictly @c {fg, bg}.
 *
 * Boundary. Out-of-image SE neighbors are folded via addBoundary/removeBoundary, which count as foreground
 * iff @p boundaryToForeground -- bit-identical to @ref BinaryMorphScanline treating an OOB neighbor as fg iff
 * boundaryToForeground. The empty-SE (radius-0 Annulus) case passes the input through unchanged, matching
 * @ref BinaryMorphScanline so the two agree on that degenerate case.
 *
 * Its required constructor arguments match @ref BinaryMorphScanline so both drop into @ref DispatchAlgorithm.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class BinaryMorphDirect
{
public:
  BinaryMorphDirect(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, T foreground, T background, bool boundaryToForeground,
                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_Op(op)
  , m_Fg(foreground)
  , m_Bg(background)
  , m_BoundaryToForeground(boundaryToForeground)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  ~BinaryMorphDirect() = default;

  BinaryMorphDirect(const BinaryMorphDirect&) = delete;
  BinaryMorphDirect(BinaryMorphDirect&&) noexcept = delete;
  BinaryMorphDirect& operator=(const BinaryMorphDirect&) = delete;
  BinaryMorphDirect& operator=(BinaryMorphDirect&&) noexcept = delete;

  Result<> operator()()
  {
    // Dispatch the runtime op to a compile-time bool so the accumulator's any/all formula is fixed at compile
    // time (matching ITK's distinct Dilate/Erode filter classes).
    return (m_Op == MorphOp::Dilate) ? runImpl<true>() : runImpl<false>();
  }

private:
  template <bool Dilate>
  Result<> runImpl()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    const usize sliceValues = dimX * dimY;
    const usize vol = sliceValues * dimZ;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying morphology filter: {:.1f}%");

    // Degenerate structuring element (empty offset list, only reachable for a radius-0 Annulus): pass the
    // input through unchanged, byte-for-byte identical to BinaryMorphScanline so the two paths still agree.
    // This rare edge case stays serial; its output-plane scratch lives here so the parallel fast path below
    // never allocates it.
    if(m_SE.offsets.empty())
    {
      auto outPlane = std::make_unique<T[]>(sliceValues);
      for(usize z = 0; z < dimZ; ++z)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        if(Result<> r = m_In.copyIntoBuffer(z * sliceValues, nonstd::span<T>(outPlane.get(), sliceValues)); r.invalid())
        {
          return r;
        }
        if(Result<> r = m_Out.copyFromBuffer(z * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
        {
          return r;
        }
      }
      return {};
    }

    // Pull the whole array into a flat local buffer once for free random SE access. This is a memcpy in-core
    // and one bulk read when the working-memory wrapper selects this route for a chunked array.
    auto data = std::make_unique<T[]>(vol);
    if(Result<> r = m_In.copyIntoBuffer(0, nonstd::span<T>(data.get(), vol)); r.invalid())
    {
      return r;
    }
    const T* in = data.get();

    if constexpr(detail::k_UseVectorHistogram<T>)
    {
      if(detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE))
      {
        const detail::PackedBinaryPlaneLayout packedLayout = detail::MakePackedBinaryPlaneLayout(m_Dims);
        usize packedWordCount = 0;
        if(!detail::TryMultiplyMorphologySize(packedLayout.planeWords, dimZ, packedWordCount))
        {
          return MakeErrorResult(-8661, fmt::format("Packed binary morphology image dimensions ({}) overflow the addressable word count.", StringUtilities::formatDimensions3D(m_Dims)));
        }
        std::vector<detail::BinaryMorphWord> packedInput(packedWordCount);
        if(!detail::PackBinaryMorphologyStagedPlanes(data.get(), packedInput.data(), packedLayout, dimZ, m_Fg, m_ShouldCancel))
        {
          return {};
        }
        // Release the full raw input before output allocation. The packed input uses one bit per voxel,
        // so the resident fast path stays below its former two-raw-volume scratch peak.
        data.reset();
        auto output = std::make_unique<T[]>(vol);
        detail::RunPrepackedBinaryMorphologyBlock<T, Dilate>(packedInput.data(), output.get(), m_Dims, 0, dimZ, m_Dims, SizeVec3{0, 0, 0}, m_SE, m_Fg, m_Bg, m_BoundaryToForeground, m_ShouldCancel);
        if(m_ShouldCancel)
        {
          return {};
        }
        return m_Out.copyFromBuffer(0, nonstd::span<const T>(output.get(), vol));
      }
    }

    // Per-axis "entering"/"leaving" offset sets for a +1 step, shared with MorphDirect (see
    // detail::ComputeAxisAddRemoveSets). The two sets have equal size, so the window always holds |SE| entries.
    const detail::AxisAddRemoveSets axisSets = detail::ComputeAxisAddRemoveSets(m_SE);
    const std::array<std::vector<SEOffset>, 3>& added = axisSets.added;
    const std::array<std::vector<SEOffset>, 3>& removed = axisSets.removed;

    using Acc = detail::FgCountAccumulator<T, Dilate>;
    const usize windowSize = m_SE.offsets.size();

    const int64 nX = static_cast<int64>(dimX);
    const int64 nY = static_cast<int64>(dimY);
    const int64 nZ = static_cast<int64>(dimZ);
    // Per-axis radius (clamped >= 0). Any added/removed offset reaches at most r_a + 1 along axis a, so a
    // center whose (r_a + 1)-padded neighborhood is fully in bounds needs no per-voxel checks.
    const int64 rX = (m_SE.radius[0] > 0) ? m_SE.radius[0] : 0;
    const int64 rY = (m_SE.radius[1] > 0) ? m_SE.radius[1] : 0;
    const int64 rZ = (m_SE.radius[2] > 0) ? m_SE.radius[2] : 0;

    // PRECONDITION for the unchecked fast path below: every SE offset lies within its per-axis radius box,
    // i.e. offsets subseteq [-radius, radius]^3 (see MorphDirect; MakeStructuringElement always honors it).
#ifndef NDEBUG
    for(const SEOffset& o : m_SE.offsets)
    {
      assert(o.dx >= -rX && o.dx <= rX && o.dy >= -rY && o.dy <= rY && o.dz >= -rZ && o.dz <= rZ && "SE offset outside its radius box; fast-path bounds assumption violated");
    }
#endif

    auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
    auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

    // Slide the accumulator from center (cx,cy,cz) by +1 along axis a: fold in the entering voxels, fold out
    // the leaving ones. The fast path (whole padded neighborhood in bounds) skips per-voxel checks; the slow
    // path treats an OOB neighbor as foreground iff boundaryToForeground via addBoundary/removeBoundary
    // (== BinaryMorphScanline's boundary rule).
    auto push = [&](Acc& h, int64 cx, int64 cy, int64 cz, int a) {
      const bool fast = (cx - (rX + 1) >= 0) && (cx + (rX + 1) < nX) && (cy - (rY + 1) >= 0) && (cy + (rY + 1) < nY) && (cz - (rZ + 1) >= 0) && (cz + (rZ + 1) < nZ);
      if(fast)
      {
        for(const SEOffset& q : added[a])
        {
          h.addPixel(in[flat(cx + q.dx, cy + q.dy, cz + q.dz)]);
        }
        for(const SEOffset& o : removed[a])
        {
          h.removePixel(in[flat(cx + o.dx, cy + o.dy, cz + o.dz)]);
        }
      }
      else
      {
        for(const SEOffset& q : added[a])
        {
          const int64 x = cx + q.dx;
          const int64 y = cy + q.dy;
          const int64 z = cz + q.dz;
          if(inBounds(x, y, z))
          {
            h.addPixel(in[flat(x, y, z)]);
          }
          else
          {
            h.addBoundary();
          }
        }
        for(const SEOffset& o : removed[a])
        {
          const int64 x = cx + o.dx;
          const int64 y = cy + o.dy;
          const int64 z = cz + o.dz;
          if(inBounds(x, y, z))
          {
            h.removePixel(in[flat(x, y, z)]);
          }
          else
          {
            h.removeBoundary();
          }
        }
      }
    };

    // Seed a plane-start window at corner (0,0,zStart) -- a one-time O(|SE|) init identical to the former
    // origin seed but shifted along Z, so each parallel Z-slab worker below can build its OWN starting
    // accumulator independently instead of carrying one across slabs.
    auto seedAcc = [&](Acc& h, int64 zStart) {
      for(const SEOffset& o : m_SE.offsets)
      {
        const int64 x = o.dx;
        const int64 y = o.dy;
        const int64 z = zStart + o.dz;
        if(inBounds(x, y, z))
        {
          h.addPixel(in[flat(x, y, z)]);
        }
        else
        {
          h.addBoundary();
        }
      }
    };

    // Parallelize the moving fg-count traversal across contiguous Z-slabs (mirrors MorphScanline's
    // ParallelDataAlgorithm use and ITK's DynamicThreadedGenerateData). Every output voxel is an
    // order-independent any/all predicate over the READ-ONLY input `in`, and each worker owns its output
    // plane plus its three nested accumulators and writes only its own disjoint Z-planes, so threading cannot
    // change any value. Direct is chosen only for in-core stores; requireStoresInMemory keeps parallelization
    // on for them (and defensively serializes were a store ever out-of-core).
    std::atomic<bool> hasError{false};
    std::mutex resultMutex;
    Result<> firstError;

    auto processZRange = [&](const Range& zRange) {
      const usize zBegin = zRange.min();
      const usize zEnd = zRange.max();
      if(zBegin >= zEnd)
      {
        return;
      }
      // Per-worker scratch: its own output plane, its own ProgressMessenger (the messenger is not shared-
      // thread-safe, but the progress COUNTER it feeds is atomic), and its own three nested accumulators.
      auto outPlane = std::make_unique<T[]>(sliceValues);
      auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

      // Seed this slab's plane-start window at its OWN origin corner (0,0,zBegin), independent of every other
      // slab. accY (line start) and accX (working window) are declared ONCE per worker and reset by
      // assignment below, mirroring MorphDirect's per-worker histogram reuse (the accumulator is a single
      // integer, so this is byte-identical to copy-constructing them each iteration).
      Acc accZ(m_Fg, m_Bg, m_BoundaryToForeground, windowSize);
      seedAcc(accZ, static_cast<int64>(zBegin));
      Acc accY(m_Fg, m_Bg, m_BoundaryToForeground, windowSize);
      Acc accX(m_Fg, m_Bg, m_BoundaryToForeground, windowSize);

      for(usize zc = zBegin; zc < zEnd; ++zc)
      {
        if(m_ShouldCancel)
        {
          return;
        }
        const int64 z = static_cast<int64>(zc);
        if(zc > zBegin)
        {
          push(accZ, 0, 0, z - 1, 2);
        }
        accY = accZ;
        for(int64 y = 0; y < nY; ++y)
        {
          if(y > 0)
          {
            push(accY, 0, y - 1, z, 1);
          }
          accX = accY;
          for(int64 x = 0; x < nX; ++x)
          {
            outPlane[static_cast<usize>(y * nX + x)] = accX.getValue();
            if(x < nX - 1)
            {
              push(accX, x, y, z, 0);
            }
          }
        }
        if(Result<> r = m_Out.copyFromBuffer(zc * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
        {
          const std::lock_guard<std::mutex> lock(resultMutex);
          if(!hasError)
          {
            firstError = std::move(r);
            hasError = true;
          }
          return;
        }
        progressMessenger.sendProgressMessage(1);
      }
    };

    ParallelDataAlgorithm parallelAlgorithm;
    IParallelAlgorithm::AlgorithmStores algStores{&m_In, &m_Out};
    parallelAlgorithm.requireStoresInMemory(algStores);
    parallelAlgorithm.setRange(0, dimZ);
    parallelAlgorithm.execute(processZRange);

    if(hasError)
    {
      return firstError;
    }
    return {};
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  MorphOp m_Op;
  T m_Fg;
  T m_Bg;
  bool m_BoundaryToForeground;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

template <class T>
class BinaryMorphWorkingMemory
{
public:
  BinaryMorphWorkingMemory(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, T foreground, T background,
                           bool boundaryToForeground, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_Op(op)
  , m_Fg(foreground)
  , m_Bg(background)
  , m_BoundaryToForeground(boundaryToForeground)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    if constexpr(detail::k_UseVectorHistogram<T>)
    {
      if(detail::ShouldUseMorphologyResidentFlatFold(m_Dims, m_SE))
      {
        auto allocationResult = detail::ReserveMorphologyResidentWorkingMemory<T>(m_Dims);
        if(allocationResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(allocationResult));
        }
        auto allocation = std::move(allocationResult.value());
        if(allocation.holdsCompleteState())
        {
          try
          {
            return BinaryMorphDirect<T>{m_In, m_Out, m_Dims, m_SE, m_Op, m_Fg, m_Bg, m_BoundaryToForeground, m_ShouldCancel, m_MessageHandler}();
          } catch(const std::bad_alloc&)
          {
            // Release the reservation; the scanline fallback overwrites the complete output.
          }
        }
      }
    }
    return BinaryMorphScanline<T>{m_In, m_Out, m_Dims, m_SE, m_Op, m_Fg, m_Bg, m_BoundaryToForeground, m_ShouldCancel, m_MessageHandler}();
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  MorphOp m_Op;
  T m_Fg;
  T m_Bg;
  bool m_BoundaryToForeground;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Public entry point for the binary morphology engine.
 *
 * Dispatches on storage type via @ref DispatchAlgorithm. In-core arrays use @ref BinaryMorphDirect. Chunked
 * arrays use a complete resident 8-bit state only when the shared working-memory budget grants it; otherwise
 * they use @ref BinaryMorphScanline. Both routes produce bit-identical output.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
Result<> ApplyBinaryMorphology(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, MorphOp op, T foreground, T background,
                               bool boundaryToForeground, const IDataArray& inArray, const IDataArray& outArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  return DispatchAlgorithm<BinaryMorphDirect<T>, BinaryMorphWorkingMemory<T>>({&inArray, &outArray}, in, out, dims, se, op, foreground, background, boundaryToForeground, shouldCancel, messageHandler);
}

/**
 * @brief Computes the conservative bounded working-memory requirement for @ref ApplyBinaryMorphologyCompositePipeline.
 *
 * The reservation covers two raw rolling Z-rings and one output plane. Moderate multi-plane 8-bit
 * runs use smaller packed membership rings. The requirement never scales with the volume depth.
 *
 * @param dims Image dimensions.
 * @param se Flat structuring element.
 * @param safeBorder True to include the SafeBorder pad.
 * @tparam T Scalar voxel type.
 * @return Required conservative working-memory bytes.
 */
template <class T>
Result<usize> CalculateBinaryMorphologyCompositePipelineWorkingMemoryBytes(const SizeVec3& dims, const StructuringElement& se, bool safeBorder)
{
  const std::array<usize, 3> cropOffset = safeBorder ? detail::MorphologySafeBorderPadRadius(se, dims) : std::array<usize, 3>{0, 0, 0};
  SizeVec3 paddedDims = dims;
  if(safeBorder)
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(cropOffset[axis] > (std::numeric_limits<usize>::max() - dims[axis]) / 2)
      {
        return MakeErrorResult<usize>(-8482, fmt::format("SafeBorder binary morphology-composite padded dimensions overflow. Dimensions: {}; axis: {}; radius: {}.",
                                                         StringUtilities::formatDimensions3D(dims), axis, cropOffset[axis]));
      }
      paddedDims[axis] = dims[axis] + 2 * cropOffset[axis];
    }
  }
  const usize radiusZ = detail::MorphologyCompositePipelineRadiusZ(se, dims);
  if(radiusZ > (std::numeric_limits<usize>::max() - 1) / 2)
  {
    return MakeErrorResult<usize>(
        -8482, fmt::format("SafeBorder binary morphology-composite pipeline Z radius {} overflows its rolling-ring depth for dimensions {}.", radiusZ, StringUtilities::formatDimensions3D(dims)));
  }
  const usize ringDepth = 2 * radiusZ + 1;
  usize paddedSliceValues = 0;
  usize outputSliceValues = 0;
  usize ringValues = 0;
  usize requiredValues = 0;
  usize requiredBytes = 0;
  if(!detail::TryMultiplyMorphologySize(paddedDims[0], paddedDims[1], paddedSliceValues) || !detail::TryMultiplyMorphologySize(dims[0], dims[1], outputSliceValues) ||
     !detail::TryMultiplyMorphologySize(ringDepth, paddedSliceValues, ringValues) || ringValues > std::numeric_limits<usize>::max() / 2 ||
     !detail::TryMultiplyMorphologySize(ringValues, usize{2}, requiredValues) || requiredValues > std::numeric_limits<usize>::max() - outputSliceValues ||
     !detail::TryMultiplyMorphologySize(requiredValues + outputSliceValues, sizeof(T), requiredBytes))
  {
    return MakeErrorResult<usize>(
        -8482, fmt::format("SafeBorder binary morphology-composite pipeline working-memory dimensions overflow. Dimensions: {}; padded slice values: {}; output slice values: {}; ring depth: {}.",
                           StringUtilities::formatDimensions3D(dims), paddedSliceValues, outputSliceValues, ringDepth));
  }
  return {requiredBytes};
}

/**
 * @brief Runs an out-of-core-safe binary Opening or Closing with bounded rolling rings.
 *
 * Moderate multi-plane 8-bit runs retain packed membership rings. Each input plane is packed
 * once. The first stage writes packed words. The second stage writes only the final raw plane.
 * Other runs retain raw rings. They use existing fallback folds. No path materializes a full
 * intermediate array.
 *
 * This pipeline has no binary top-hat output mode. It does not retain an original plane for
 * subtraction. The caller-supplied @p background fills the pad. Closing uses ITK's derived
 * internal background. Opening calls this function with @p safeBorder false.
 *
 * @param input Input data store.
 * @param output Output data store.
 * @param dims Image dimensions.
 * @param se Flat structuring element.
 * @param firstOp First morphology operation.
 * @param secondOp Second morphology operation.
 * @param safeBorder True to include the SafeBorder pad.
 * @param foreground Source value that maps to foreground membership.
 * @param background Output background value and SafeBorder pad value.
 * @param shouldCancel Stops work before later output writes.
 * @param grantedWorkingMemoryBytes Reserved conservative working-memory bytes.
 * @tparam T Scalar voxel type.
 * @return An error result for invalid memory dimensions or store transfers.
 */
template <class T>
Result<> ApplyBinaryMorphologyCompositePipeline(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, const SizeVec3& dims, const StructuringElement& se, MorphOp firstOp, MorphOp secondOp,
                                                bool safeBorder, T foreground, T background, const std::atomic_bool& shouldCancel, usize grantedWorkingMemoryBytes)
{
  if(shouldCancel)
  {
    return {};
  }
  auto requiredBytesResult = CalculateBinaryMorphologyCompositePipelineWorkingMemoryBytes<T>(dims, se, safeBorder);
  if(requiredBytesResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(requiredBytesResult));
  }
  const usize requiredBytes = requiredBytesResult.value();
  const std::array<usize, 3> cropOffset = safeBorder ? detail::MorphologySafeBorderPadRadius(se, dims) : std::array<usize, 3>{0, 0, 0};
  SizeVec3 paddedDims = dims;
  if(safeBorder)
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      paddedDims[axis] = dims[axis] + 2 * cropOffset[axis];
    }
  }
  const usize radiusZ = detail::MorphologyCompositePipelineRadiusZ(se, dims);
  const usize ringDepth = 2 * radiusZ + 1;
  usize paddedSliceValues = 0;
  usize outputSliceValues = 0;
  usize ringValues = 0;
  if(!detail::TryMultiplyMorphologySize(paddedDims[0], paddedDims[1], paddedSliceValues) || !detail::TryMultiplyMorphologySize(dims[0], dims[1], outputSliceValues) ||
     !detail::TryMultiplyMorphologySize(ringDepth, paddedSliceValues, ringValues))
  {
    return MakeErrorResult(-8482, fmt::format("SafeBorder binary morphology-composite pipeline slab dimensions overflow. Dimensions: {}; padded dimensions: {}; ring depth: {}.",
                                              StringUtilities::formatDimensions3D(dims), StringUtilities::formatDimensions3D(paddedDims), ringDepth));
  }
  if(grantedWorkingMemoryBytes < requiredBytes)
  {
    return MakeErrorResult(-8483, fmt::format("SafeBorder binary morphology-composite pipeline requires {} bytes but only {} bytes are reserved. Dimensions: {}; padded dimensions: {}; Z radius: {}.",
                                              requiredBytes, grantedWorkingMemoryBytes, StringUtilities::formatDimensions3D(dims), StringUtilities::formatDimensions3D(paddedDims), radiusZ));
  }

  const bool useFlatFold = detail::k_UseVectorHistogram<T> && detail::ShouldUseMorphologyResidentFlatFold(paddedDims, se);
  const bool usePackedComposite = useFlatFold && !se.offsets.empty();
  const detail::PackedBinaryPlaneLayout packedLayout = detail::MakePackedBinaryPlaneLayout(paddedDims);
  std::unique_ptr<T[]> inputRing;
  std::unique_ptr<T[]> firstRing;
  std::optional<detail::PackedBinaryRing> packedInputRing;
  std::optional<detail::PackedBinaryRing> packedFirstRing;
  if(usePackedComposite)
  {
    packedInputRing.emplace(packedLayout, ringDepth);
    packedFirstRing.emplace(packedLayout, ringDepth);
  }
  else
  {
    inputRing = std::make_unique<T[]>(ringValues);
    firstRing = std::make_unique<T[]>(ringValues);
  }

  // Read-ahead / write-behind batching for this ring's per-plane store I/O -- identical rationale and bounds
  // to the grayscale ring (see ApplyMorphologyCompositePipeline): a single unbatched plane transfer is too
  // small to trigger the OOC backend's parallel resident-chunk scatter, so several consecutive planes are
  // batched into one bulk copyIntoBuffer/copyFromBuffer call. Best-effort: a failed or short reservation just
  // collapses staging to one plane (today's unbatched transfer), never fails the call.
  usize stagingPlanes = detail::k_CompositeStagingMinPlanes;
  usize stagingPlaneBytes = 0;
  CacheMemoryBudgetManager::WorkingMemoryReservation stagingReservation;
  if(detail::TryMultiplyMorphologySize(outputSliceValues, sizeof(T), stagingPlaneBytes) && stagingPlaneBytes > 0)
  {
    usize preferredSideBytes = 0;
    usize preferredTotalBytes = 0;
    if(detail::TryMultiplyMorphologySize(stagingPlaneBytes, detail::k_CompositeStagingPreferredPlanes, preferredSideBytes) &&
       detail::TryMultiplyMorphologySize(preferredSideBytes, usize{2}, preferredTotalBytes))
    {
      // One read-ahead buffer and one write-behind buffer, each sized for the preferred plane count.
      stagingReservation = ReserveWorkingMemory(preferredTotalBytes, preferredTotalBytes);
      const usize grantedPlanes = static_cast<usize>(stagingReservation.sizeBytes()) / (2 * stagingPlaneBytes);
      stagingPlanes = std::clamp(grantedPlanes, detail::k_CompositeStagingMinPlanes, detail::k_CompositeStagingPreferredPlanes);
      stagingReservation.shrinkTo(static_cast<uint64>(stagingPlanes * 2 * stagingPlaneBytes));
    }
  }
  // Never stage more planes than the volume has -- avoids over-allocating for small or thin volumes.
  stagingPlanes = std::min(stagingPlanes, std::max<usize>(dims[2], usize{1}));

  usize stagingBufferElements = 0;
  if(!detail::TryMultiplyMorphologySize(stagingPlanes, outputSliceValues, stagingBufferElements))
  {
    return MakeErrorResult(-8484, fmt::format("SafeBorder binary morphology-composite pipeline staging buffer size overflows. Dimensions: {}; staging planes: {}; output slice values: {}.",
                                              StringUtilities::formatDimensions3D(dims), stagingPlanes, outputSliceValues));
  }

  auto inputStagingBuffer = std::make_unique<T[]>(stagingBufferElements);
  usize inputStagingBase = 0;     // originalZ of the first plane currently resident in inputStagingBuffer
  usize inputStagingCount = 0;    // number of valid planes currently resident
  usize inputStagingConsumed = 0; // planes already scattered into the ring from the current load

  auto outputStagingBuffer = std::make_unique<T[]>(stagingBufferElements);
  usize outputStagingBase = 0;  // outputZ of the first plane pending flush
  usize outputStagingCount = 0; // number of valid planes pending flush

  const bool firstDilate = firstOp == MorphOp::Dilate;
  const bool secondDilate = secondOp == MorphOp::Dilate;
  const bool firstBoundaryToForeground = firstOp == MorphOp::Erode;
  const bool secondBoundaryToForeground = secondOp == MorphOp::Erode;
  // The flat fold uses only its precomputed offsets. The moving-window fallback needs axis sets.
  const std::vector<detail::MorphologyFlatFoldOffset> flatOffsets =
      !usePackedComposite && useFlatFold ? detail::MakeMorphologyFlatFoldOffsets(se, paddedDims[0], paddedDims[1]) : std::vector<detail::MorphologyFlatFoldOffset>{};
  std::optional<detail::AxisAddRemoveSets> axisSets;
  if(!useFlatFold)
  {
    axisSets = detail::ComputeAxisAddRemoveSets(se);
  }
  // The first stage's output plane IS the padded plane (no crop), so its neighborhood center needs no
  // translation from output-local (ox, oy) -- unlike the second stage, which reads through cropOffset.
  const std::array<usize, 3> identityOrigin{0, 0, 0};
  usize inputZLo = 0;
  usize inputDepth = 0;
  usize firstZLo = 0;
  usize firstDepth = 0;

  // Flushes any output planes pending in outputStagingBuffer with one bulk copyFromBuffer call. Called both
  // when the write-behind buffer fills and at every early-return/completion point below, so a canceled or
  // completed run never leaves already-computed planes stranded in memory instead of reaching the store.
  const auto flushOutputStaging = [&]() -> Result<> {
    if(outputStagingCount == 0)
    {
      return {};
    }
    const usize flushedPlanes = outputStagingCount;
    outputStagingCount = 0;
    return output.copyFromBuffer(outputStagingBase * outputSliceValues, nonstd::span<const T>(outputStagingBuffer.get(), flushedPlanes * outputSliceValues));
  };

  const auto appendInputPlane = [&](usize paddedZ) -> Result<> {
    T* rawPlane = nullptr;
    if(!usePackedComposite && inputDepth == ringDepth)
    {
      std::memmove(inputRing.get(), inputRing.get() + paddedSliceValues, (ringDepth - 1) * paddedSliceValues * sizeof(T));
      ++inputZLo;
      --inputDepth;
    }
    if(!usePackedComposite)
    {
      rawPlane = inputRing.get() + inputDepth * paddedSliceValues;
      std::fill_n(rawPlane, paddedSliceValues, background);
    }
    const T* sourcePlane = nullptr;
    if(paddedZ >= cropOffset[2] && paddedZ < cropOffset[2] + dims[2])
    {
      const usize originalZ = paddedZ - cropOffset[2];
      if(inputStagingConsumed >= inputStagingCount)
      {
        // paddedZ advances exactly one plane per call, and the crop-offset skip region only ever precedes
        // this contiguous run of in-range planes, so originalZ always continues immediately where the
        // previous load left off: a fresh batch never needs to re-read or skip a plane.
        assert(originalZ == inputStagingBase + inputStagingCount && "input staging batch must continue contiguously from the previous load");
        const usize batchPlanes = std::min(stagingPlanes, dims[2] - originalZ);
        if(Result<> result = input.copyIntoBuffer(originalZ * outputSliceValues, nonstd::span<T>(inputStagingBuffer.get(), batchPlanes * outputSliceValues)); result.invalid())
        {
          return result;
        }
        inputStagingBase = originalZ;
        inputStagingCount = batchPlanes;
        inputStagingConsumed = 0;
      }
      sourcePlane = inputStagingBuffer.get() + inputStagingConsumed * outputSliceValues;
      ++inputStagingConsumed;
      if(!usePackedComposite)
      {
        for(usize y = 0; y < dims[1]; ++y)
        {
          std::copy_n(sourcePlane + y * dims[0], dims[0], rawPlane + (cropOffset[1] + y) * paddedDims[0] + cropOffset[0]);
        }
      }
    }
    if(usePackedComposite)
    {
      detail::BinaryMorphWord* packedPlane = packedInputRing->appendPlane(paddedZ);
      if(!detail::PackBinaryPaddedPlane(sourcePlane, packedPlane, packedLayout, dims, cropOffset, foreground, shouldCancel))
      {
        return {};
      }
    }
    else
    {
      if(inputDepth == 0)
      {
        inputZLo = paddedZ;
      }
      ++inputDepth;
    }
    return {};
  };

  const auto appendFirstPlane = [&](usize paddedZ) -> Result<> {
    if(usePackedComposite)
    {
      detail::BinaryMorphWord* packedPlane = packedFirstRing->appendPlane(paddedZ);
      const detail::PackedBinaryPlanesView inputView = packedInputRing->view();
      if(firstDilate)
      {
        if(!detail::RunPrepackedBinaryMorphologyPlane<true>(inputView, packedPlane, paddedDims, SizeVec3{0, 0, paddedZ}, se, firstBoundaryToForeground, shouldCancel))
        {
          return {};
        }
      }
      else
      {
        if(!detail::RunPrepackedBinaryMorphologyPlane<false>(inputView, packedPlane, paddedDims, SizeVec3{0, 0, paddedZ}, se, firstBoundaryToForeground, shouldCancel))
        {
          return {};
        }
      }
      return {};
    }
    if(firstDepth == ringDepth)
    {
      std::memmove(firstRing.get(), firstRing.get() + paddedSliceValues, (ringDepth - 1) * paddedSliceValues * sizeof(T));
      ++firstZLo;
      --firstDepth;
    }
    T* plane = firstRing.get() + firstDepth * paddedSliceValues;
    if(se.offsets.empty())
    {
      std::copy_n(inputRing.get() + (paddedZ - inputZLo) * paddedSliceValues, paddedSliceValues, plane);
    }
    else
    {
      if(useFlatFold)
      {
        if(firstDilate)
        {
          detail::RunBinaryMorphologyFlatFoldBlock<T, true>(inputRing.get(), plane, paddedDims, inputZLo, SizeVec3{paddedDims[0], paddedDims[1], 1}, SizeVec3{0, 0, paddedZ}, se, foreground,
                                                            background, firstBoundaryToForeground, flatOffsets, shouldCancel);
        }
        else
        {
          detail::RunBinaryMorphologyFlatFoldBlock<T, false>(inputRing.get(), plane, paddedDims, inputZLo, SizeVec3{paddedDims[0], paddedDims[1], 1}, SizeVec3{0, 0, paddedZ}, se, foreground,
                                                             background, firstBoundaryToForeground, flatOffsets, shouldCancel);
        }
      }
      else
      {
        if(firstDilate)
        {
          detail::RunBinaryMorphologyPlaneWindow<T, true>(inputRing.get(), plane, paddedDims[0], paddedDims[1], paddedDims[2], inputZLo, paddedDims[0], paddedDims[1], identityOrigin, paddedZ, se,
                                                          *axisSets, foreground, background, firstBoundaryToForeground, shouldCancel);
        }
        else
        {
          detail::RunBinaryMorphologyPlaneWindow<T, false>(inputRing.get(), plane, paddedDims[0], paddedDims[1], paddedDims[2], inputZLo, paddedDims[0], paddedDims[1], identityOrigin, paddedZ, se,
                                                           *axisSets, foreground, background, firstBoundaryToForeground, shouldCancel);
        }
      }
    }
    if(firstDepth == 0)
    {
      firstZLo = paddedZ;
    }
    ++firstDepth;
    return {};
  };

  const auto writeFinalPlane = [&](usize paddedFirstZ) -> Result<> {
    if(paddedFirstZ < radiusZ + cropOffset[2])
    {
      return {};
    }
    const usize outputZ = paddedFirstZ - radiusZ - cropOffset[2];
    if(outputZ >= dims[2])
    {
      return {};
    }
    if(shouldCancel)
    {
      return {};
    }
    // Completed output planes accumulate here and flush to the store in one bulk copyFromBuffer call every
    // stagingPlanes planes (or whatever remains at the end); see flushOutputStaging above. outputZ is always
    // contiguous with whatever is already pending, since paddedFirstZ (and therefore outputZ) strictly
    // increases by one across every call this lambda ever receives -- see the caller loops below.
    if(outputStagingCount == 0)
    {
      outputStagingBase = outputZ;
    }
    T* outputPlane = outputStagingBuffer.get() + outputStagingCount * outputSliceValues;
    if(usePackedComposite)
    {
      const SizeVec3 outputOrigin{cropOffset[0], cropOffset[1], cropOffset[2] + outputZ};
      const detail::PackedBinaryPlanesView firstView = packedFirstRing->view();
      if(secondDilate)
      {
        if(!detail::RunPrepackedBinaryMorphologyBlock<T, true>(firstView, outputPlane, paddedDims, SizeVec3{dims[0], dims[1], 1}, outputOrigin, se, foreground, background, secondBoundaryToForeground,
                                                               shouldCancel))
        {
          return {};
        }
      }
      else
      {
        if(!detail::RunPrepackedBinaryMorphologyBlock<T, false>(firstView, outputPlane, paddedDims, SizeVec3{dims[0], dims[1], 1}, outputOrigin, se, foreground, background, secondBoundaryToForeground,
                                                                shouldCancel))
        {
          return {};
        }
      }
    }
    else if(se.offsets.empty())
    {
      const usize sourceCenterZ = paddedFirstZ - radiusZ;
      const T* sourcePlane = firstRing.get() + (sourceCenterZ - firstZLo) * paddedSliceValues;
      for(usize y = 0; y < dims[1]; ++y)
      {
        std::copy_n(sourcePlane + (cropOffset[1] + y) * paddedDims[0] + cropOffset[0], dims[0], outputPlane + y * dims[0]);
      }
    }
    else
    {
      if(useFlatFold)
      {
        const SizeVec3 outputOrigin{cropOffset[0], cropOffset[1], cropOffset[2] + outputZ};
        if(secondDilate)
        {
          detail::RunBinaryMorphologyFlatFoldBlock<T, true>(firstRing.get(), outputPlane, paddedDims, firstZLo, SizeVec3{dims[0], dims[1], 1}, outputOrigin, se, foreground, background,
                                                            secondBoundaryToForeground, flatOffsets, shouldCancel);
        }
        else
        {
          detail::RunBinaryMorphologyFlatFoldBlock<T, false>(firstRing.get(), outputPlane, paddedDims, firstZLo, SizeVec3{dims[0], dims[1], 1}, outputOrigin, se, foreground, background,
                                                             secondBoundaryToForeground, flatOffsets, shouldCancel);
        }
      }
      else
      {
        if(secondDilate)
        {
          detail::RunBinaryMorphologyPlaneWindow<T, true>(firstRing.get(), outputPlane, paddedDims[0], paddedDims[1], paddedDims[2], firstZLo, dims[0], dims[1], cropOffset, outputZ, se, *axisSets,
                                                          foreground, background, secondBoundaryToForeground, shouldCancel);
        }
        else
        {
          detail::RunBinaryMorphologyPlaneWindow<T, false>(firstRing.get(), outputPlane, paddedDims[0], paddedDims[1], paddedDims[2], firstZLo, dims[0], dims[1], cropOffset, outputZ, se, *axisSets,
                                                           foreground, background, secondBoundaryToForeground, shouldCancel);
        }
      }
    }
    if(shouldCancel)
    {
      return {};
    }
    ++outputStagingCount;
    if(outputStagingCount == stagingPlanes)
    {
      return flushOutputStaging();
    }
    return {};
  };

  const usize paddedZCount = paddedDims[2];
  for(usize paddedZ = 0; paddedZ < paddedZCount; ++paddedZ)
  {
    if(shouldCancel)
    {
      // Persist whatever output planes already finished before giving up the rest of this run.
      return flushOutputStaging();
    }
    if(Result<> result = appendInputPlane(paddedZ); result.invalid())
    {
      return result;
    }
    if(paddedZ >= radiusZ)
    {
      const usize firstZ = paddedZ - radiusZ;
      if(Result<> result = appendFirstPlane(firstZ); result.invalid())
      {
        return result;
      }
      if(Result<> result = writeFinalPlane(firstZ); result.invalid())
      {
        return result;
      }
    }
  }
  const usize firstTailBegin = paddedZCount > radiusZ ? paddedZCount - radiusZ : 0;
  for(usize firstZ = firstTailBegin; firstZ < paddedZCount; ++firstZ)
  {
    if(shouldCancel)
    {
      // Persist whatever output planes already finished before giving up the rest of this run.
      return flushOutputStaging();
    }
    if(Result<> result = appendFirstPlane(firstZ); result.invalid())
    {
      return result;
    }
    if(Result<> result = writeFinalPlane(firstZ); result.invalid())
    {
      return result;
    }
  }
  if(!safeBorder)
  {
    if(radiusZ > std::numeric_limits<usize>::max() - paddedZCount)
    {
      return MakeErrorResult(-8485, fmt::format("SafeBorder=false binary morphology-composite virtual tail index overflows. Padded Z count: {}; Z radius: {}; dimensions: {}.", paddedZCount, radiusZ,
                                                StringUtilities::formatDimensions3D(dims)));
    }
    const usize virtualFirstZEnd = paddedZCount + radiusZ;
    for(usize virtualFirstZ = paddedZCount; virtualFirstZ < virtualFirstZEnd; ++virtualFirstZ)
    {
      if(shouldCancel)
      {
        // Persist whatever output planes already finished before giving up the rest of this run.
        return flushOutputStaging();
      }
      if(Result<> result = writeFinalPlane(virtualFirstZ); result.invalid())
      {
        return result;
      }
    }
  }
  return flushOutputStaging();
}
} // namespace nx::core::ImageProcessing
