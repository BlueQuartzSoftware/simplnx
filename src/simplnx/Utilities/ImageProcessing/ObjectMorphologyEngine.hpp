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
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief The object-morphology operation to apply through a flat structuring element.
 *
 * Object morphology is a boundary-SCATTER (NOT the min/max SE-gather of grayscale/binary morphology): only
 * the BOUNDARY voxels of the object (voxels equal to the object value that touch at least one non-object
 * voxel in the fixed radius-1 full box) paint the structuring element around themselves into a SEPARATE
 * output that starts as a copy of the input. Dilate paints the object value; Erode paints the background
 * value. This reproduces ITK 5.4.4's itkObjectMorphologyImageFilter family exactly.
 */
enum class ObjectMorphOp
{
  Dilate,
  Erode
};

namespace detail
{
inline bool ObjectMorphologyCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

struct ObjectMorphologyResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseObjectMorphologyResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateObjectMorphologyResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize residentValues = 0;
  usize requiredBytes = 0;
  if(!ObjectMorphologyCheckedMultiply(dims[0], dims[1], sliceValues) || !ObjectMorphologyCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !ObjectMorphologyCheckedMultiply(volumeValues, usize{2}, residentValues) || !ObjectMorphologyCheckedMultiply(residentValues, sizeof(T), requiredBytes))
  {
    return MakeErrorResult<usize>(-8763, fmt::format("Object morphology dimensions ({}) and element size ({} bytes) overflow while sizing the resident input and output state.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<ObjectMorphologyResidentMemoryAllocation> ReserveObjectMorphologyResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateObjectMorphologyResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<ObjectMorphologyResidentMemoryAllocation>(std::move(requiredResult));
  }
  // Request the complete useful state. The cache manager still grants at most its live aggregate headroom,
  // and the bounded gather consumes any partial grant without allocating beyond it.
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {ObjectMorphologyResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

struct ObjectMorphology3DPlan
{
  usize outputBatchDepth = 0;
  usize computeBatchDepth = 0;
  usize inputSlabDepth = 0;
  usize outputWindowDepth = 0;
  usize residentBytes = 0;
};

template <class T>
Result<ObjectMorphology3DPlan> CreateObjectMorphology3DPlan(const SizeVec3& dims, usize radiusZ, usize targetBytes)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] <= 1)
  {
    return MakeErrorResult<ObjectMorphology3DPlan>(
        -8764, fmt::format("Object morphology 3D batching requires nonzero X and Y dimensions and Z greater than one. Actual dimensions: {}.", StringUtilities::formatDimensions3D(dims)));
  }
  usize sliceValues = 0;
  const usize inputHalo = std::max<usize>(radiusZ, 1);
  auto boundedWindowDepth = [depth = dims[2]](usize batchDepth, usize halo) {
    usize doubledHalo = 0;
    if(!ObjectMorphologyCheckedMultiply(halo, usize{2}, doubledHalo) || doubledHalo > std::numeric_limits<usize>::max() - batchDepth)
    {
      return depth;
    }
    return std::min(depth, batchDepth + doubledHalo);
  };
  if(!ObjectMorphologyCheckedMultiply(dims[0], dims[1], sliceValues))
  {
    return MakeErrorResult<ObjectMorphology3DPlan>(
        -8764, fmt::format("Object morphology dimensions ({}) and radius Z ({}) overflow while sizing the 3D rolling-scatter buffers.", StringUtilities::formatDimensions3D(dims), radiusZ));
  }
  auto bytesForBatch = [&](usize batchDepth, usize& inputDepth, usize& outputDepth, usize& residentBytes) {
    inputDepth = boundedWindowDepth(batchDepth, inputHalo);
    outputDepth = boundedWindowDepth(batchDepth, radiusZ);
    usize inputValues = 0;
    usize outputValues = 0;
    usize inputBytes = 0;
    usize outputBytes = 0;
    return ObjectMorphologyCheckedMultiply(inputDepth, sliceValues, inputValues) && ObjectMorphologyCheckedMultiply(outputDepth, sliceValues, outputValues) &&
           ObjectMorphologyCheckedMultiply(inputValues, sizeof(T), inputBytes) && ObjectMorphologyCheckedMultiply(outputValues, sizeof(T), outputBytes) &&
           inputBytes <= std::numeric_limits<usize>::max() - outputBytes && (residentBytes = inputBytes + outputBytes, true);
  };
  usize minimumInputDepth = 0;
  usize minimumOutputDepth = 0;
  usize minimumBytes = 0;
  if(!bytesForBatch(1, minimumInputDepth, minimumOutputDepth, minimumBytes))
  {
    return MakeErrorResult<ObjectMorphology3DPlan>(
        -8764, fmt::format("Object morphology dimensions ({}) and radius Z ({}) overflow while sizing the 3-D minimum batch.", StringUtilities::formatDimensions3D(dims), radiusZ));
  }
  if(targetBytes < minimumBytes)
  {
    return MakeErrorResult<ObjectMorphology3DPlan>(-8765, fmt::format("Object morphology's {}-byte adaptive grant cannot hold its {}-byte 3-D rolling input and output windows for dimensions {}.",
                                                                      targetBytes, minimumBytes, StringUtilities::formatDimensions3D(dims)));
  }
  usize low = 1;
  usize high = dims[2];
  while(low < high)
  {
    const usize candidate = low + (high - low + 1) / 2;
    usize inputDepth = 0;
    usize outputDepth = 0;
    usize residentBytes = 0;
    if(bytesForBatch(candidate, inputDepth, outputDepth, residentBytes) && residentBytes <= targetBytes)
    {
      low = candidate;
    }
    else
    {
      high = candidate - 1;
    }
  }
  ObjectMorphology3DPlan plan;
  const usize batchCount = 1 + (dims[2] - 1) / low;
  const usize normalizedBatchDepth = 1 + (dims[2] - 1) / batchCount;
  plan.computeBatchDepth = normalizedBatchDepth;
  plan.outputBatchDepth = normalizedBatchDepth;
  if(!bytesForBatch(normalizedBatchDepth, plan.inputSlabDepth, plan.outputWindowDepth, plan.residentBytes))
  {
    return MakeErrorResult<ObjectMorphology3DPlan>(
        -8764, fmt::format("Object morphology 3-D rolling-scatter batch sizing overflowed. Normalized batch: {}; dimensions: {}; radius Z: {}; input depth: {}; output depth: {}.",
                           normalizedBatchDepth, StringUtilities::formatDimensions3D(dims), radiusZ, plan.inputSlabDepth, plan.outputWindowDepth));
  }
  return {plan};
}

/**
 * @brief Build the FIXED radius-1 full-box neighbor offsets (26 in 3D / 8 in 2D), center EXCLUDED.
 *
 * This is the boundary-test neighborhood: it is INDEPENDENT of the paint kernel (KernelType/KernelRadius)
 * and mirrors ITK's @c bKernelSize.Fill(1) neighborhood in @c ObjectMorphologyImageFilter (the center is
 * skipped because a boundary pixel's center always equals the object value, so it can never mark the pixel
 * as a boundary). Built once per run and shared by the resident and bounded rolling-scatter paths.
 */
inline std::vector<SEOffset> MakeFullBoxNeighborOffsets()
{
  StructuringElement box = MakeStructuringElement(KernelType::Box, {1, 1, 1});
  std::vector<SEOffset> offsets;
  offsets.reserve(box.offsets.size() - 1);
  for(const SEOffset& o : box.offsets)
  {
    if(o.dx != 0 || o.dy != 0 || o.dz != 0)
    {
      offsets.push_back(o);
    }
  }
  return offsets;
}

/**
 * @brief Drops @p droppedPlanes from the front of a contiguous plane window and shifts the retained
 *        planes toward slot zero. The window stores numeric voxel or mask values, so overlapping movement
 *        uses @c memmove.
 * @return The number of retained planes.
 */
template <class T>
usize ShiftPlaneWindowToFront(T* window, usize planeValues, usize currentDepth, usize droppedPlanes)
{
  if(droppedPlanes == 0)
  {
    return currentDepth;
  }
  if(droppedPlanes >= currentDepth)
  {
    return 0;
  }

  const usize retainedDepth = currentDepth - droppedPlanes;
  std::memmove(window, window + droppedPlanes * planeValues, retainedDepth * planeValues * sizeof(T));
  return retainedDepth;
}

template <class T>
Result<usize> ShiftObjectMorphologyPlaneWindowToFront(T* window, usize capacityPlanes, usize planeValues, usize currentDepth, usize droppedPlanes)
{
  const usize retainedDepth = droppedPlanes >= currentDepth ? 0 : currentDepth - droppedPlanes;
  usize capacityValues = 0;
  usize droppedValues = 0;
  usize retainedValues = 0;
  if(!ObjectMorphologyCheckedMultiply(capacityPlanes, planeValues, capacityValues) || !ObjectMorphologyCheckedMultiply(std::min(droppedPlanes, currentDepth), planeValues, droppedValues) ||
     !ObjectMorphologyCheckedMultiply(retainedDepth, planeValues, retainedValues) || retainedValues > std::numeric_limits<usize>::max() / sizeof(T))
  {
    return MakeErrorResult<usize>(-8766, fmt::format("Object morphology rolling window shift overflow. Capacity planes: {}; planeValues: {}; current depth: {}; dropped depth: {}; retained depth: {}.",
                                                     capacityPlanes, planeValues, currentDepth, droppedPlanes, retainedDepth));
  }
  if(currentDepth > capacityPlanes || droppedValues > capacityValues || retainedValues > capacityValues)
  {
    return MakeErrorResult<usize>(-8766, fmt::format("Object morphology rolling window shift exceeds capacity. Capacity planes: {}; planeValues: {}; current depth: {}; dropped depth: {}; retained "
                                                     "depth: {}; capacity values: {}; dropped values: {}; retained values: {}.",
                                                     capacityPlanes, planeValues, currentDepth, droppedPlanes, retainedDepth, capacityValues, droppedValues, retainedValues));
  }
  return {ShiftPlaneWindowToFront(window, planeValues, currentDepth, droppedPlanes)};
}

struct ObjectMorphologyPlaneWindow
{
  usize origin = 0;
  usize end = 0;
};

inline Result<ObjectMorphologyPlaneWindow> CreateObjectMorphologyPlaneWindow(usize batchBegin, usize batchEnd, usize halo, usize dimZ)
{
  if(batchBegin > batchEnd || batchEnd > dimZ)
  {
    return MakeErrorResult<ObjectMorphologyPlaneWindow>(
        -8766,
        fmt::format("Object morphology rolling window requires 0 <= batchBegin <= batchEnd <= dimZ. Actual batchBegin: {}; batchEnd: {}; halo: {}; dimZ: {}.", batchBegin, batchEnd, halo, dimZ));
  }
  const usize origin = batchBegin > halo ? batchBegin - halo : 0;
  const usize end = halo >= dimZ || batchEnd > dimZ - halo ? dimZ : batchEnd + halo;
  return {ObjectMorphologyPlaneWindow{origin, end}};
}

template <class T>
Result<> AdvanceObjectMorphologyInputWindow(const AbstractDataStore<T>& store, T* window, usize capacityPlanes, usize planeValues, ObjectMorphologyPlaneWindow& current,
                                            const ObjectMorphologyPlaneWindow& desired)
{
  if(planeValues == 0 || current.end < current.origin || desired.end < desired.origin || desired.origin < current.origin || desired.end < current.end || desired.end - desired.origin > capacityPlanes)
  {
    return MakeErrorResult(
        -8766,
        fmt::format(
            "Object morphology rolling input window requires nonzero planeValues, valid half-open ranges, monotonic origins/ends, and desired depth <= capacity. Current: [{}, {}); desired: [{}, {}); "
            "capacity planes: {}; planeValues: {}.",
            current.origin, current.end, desired.origin, desired.end, capacityPlanes, planeValues));
  }
  const usize currentDepth = current.end - current.origin;
  const usize droppedPlanes = desired.origin - current.origin;
  auto retainedResult = ShiftObjectMorphologyPlaneWindowToFront(window, capacityPlanes, planeValues, currentDepth, droppedPlanes);
  if(retainedResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(retainedResult));
  }
  const usize retainedDepth = retainedResult.value();
  const usize retainedEnd = current.origin + currentDepth;
  const usize readBegin = std::max(retainedEnd, desired.origin);
  if(readBegin < desired.end)
  {
    usize readStart = 0;
    usize destinationOffset = 0;
    usize readValues = 0;
    if(!ObjectMorphologyCheckedMultiply(readBegin, planeValues, readStart) || !ObjectMorphologyCheckedMultiply(retainedDepth, planeValues, destinationOffset) ||
       !ObjectMorphologyCheckedMultiply(desired.end - readBegin, planeValues, readValues))
    {
      return MakeErrorResult(
          -8766, fmt::format("Object morphology rolling input window offset overflow. Current: [{}, {}); desired: [{}, {}); capacity planes: {}; planeValues: {}; read begin: {}; retained depth: {}.",
                             current.origin, current.end, desired.origin, desired.end, capacityPlanes, planeValues, readBegin, retainedDepth));
    }
    // A failed read terminates the caller; the pre-read shifted scratch window is intentionally not reusable.
    if(Result<> result = store.copyIntoBuffer(readStart, nonstd::span<T>(window + destinationOffset, readValues)); result.invalid())
    {
      return result;
    }
  }
  current = desired;
  return {};
}

template <class T>
Result<> AdvanceObjectMorphologyOutputWindow(T* output, usize capacityPlanes, usize planeValues, ObjectMorphologyPlaneWindow& current, const ObjectMorphologyPlaneWindow& desired, const T* input,
                                             const ObjectMorphologyPlaneWindow& inputRange)
{
  if(planeValues == 0 || current.end < current.origin || desired.end < desired.origin || inputRange.end < inputRange.origin || desired.origin < current.origin || desired.end < current.end ||
     desired.origin < inputRange.origin || desired.end > inputRange.end)
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology output window requires valid monotonic current/desired ranges, desired subset of complete input range, and nonzero planeValues. "
                                              "Current: [{}, {}); desired: [{}, {}); input: [{}, {}); capacity: {}; planeValues: {}.",
                                              current.origin, current.end, desired.origin, desired.end, inputRange.origin, inputRange.end, capacityPlanes, planeValues));
  }
  const usize currentDepth = current.end - current.origin;
  const usize desiredDepth = desired.end - desired.origin;
  const usize droppedDepth = desired.origin - current.origin;
  if(currentDepth > capacityPlanes || desiredDepth > capacityPlanes)
  {
    return MakeErrorResult(
        -8766,
        fmt::format("Object morphology output window depth exceeds capacity. Current: [{}, {}); desired: [{}, {}); input: [{}, {}); currentDepth: {}; desiredDepth: {}; capacity: {}; planeValues: {}.",
                    current.origin, current.end, desired.origin, desired.end, inputRange.origin, inputRange.end, currentDepth, desiredDepth, capacityPlanes, planeValues));
  }
  auto retainedResult = ShiftObjectMorphologyPlaneWindowToFront(output, capacityPlanes, planeValues, currentDepth, droppedDepth);
  if(retainedResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(retainedResult));
  }
  const usize retainedDepth = retainedResult.value();
  const usize retainedEnd = current.origin + currentDepth;
  const usize copyBegin = std::max(retainedEnd, desired.origin);
  const usize copyDepth = desired.end - copyBegin;
  if(copyDepth > std::numeric_limits<usize>::max() - retainedDepth || retainedDepth + copyDepth != desiredDepth)
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology output window derived depths are inconsistent. Current: [{}, {}); desired: [{}, {}); input: [{}, {}); dropped: {}; retained: {}; "
                                              "copy: {}; capacity: {}; planeValues: {}.",
                                              current.origin, current.end, desired.origin, desired.end, inputRange.origin, inputRange.end, droppedDepth, retainedDepth, copyDepth, capacityPlanes,
                                              planeValues));
  }
  if(copyBegin < desired.end)
  {
    usize sourceOffset = 0;
    usize destinationOffset = 0;
    usize copyValues = 0;
    if(!ObjectMorphologyCheckedMultiply(copyBegin - inputRange.origin, planeValues, sourceOffset) || !ObjectMorphologyCheckedMultiply(retainedDepth, planeValues, destinationOffset) ||
       !ObjectMorphologyCheckedMultiply(copyDepth, planeValues, copyValues))
    {
      return MakeErrorResult(
          -8766, fmt::format(
                     "Object morphology output window copy offset overflow. Current: [{}, {}); desired: [{}, {}); input: [{}, {}); dropped: {}; retained: {}; copy: {}; capacity: {}; planeValues: {}.",
                     current.origin, current.end, desired.origin, desired.end, inputRange.origin, inputRange.end, droppedDepth, retainedDepth, copyDepth, capacityPlanes, planeValues));
    }
    std::copy_n(input + sourceOffset, copyValues, output + destinationOffset);
  }
  current = desired;
  return {};
}

template <class T>
Result<> FinalizeObjectMorphologyOutputWindow(AbstractDataStore<T>& outStore, T* output, usize capacityPlanes, usize planeValues, ObjectMorphologyPlaneWindow& current, usize finalEnd,
                                              const std::atomic_bool& shouldCancel)
{
  if(planeValues == 0 || current.end < current.origin || finalEnd < current.origin || finalEnd > current.end || current.end - current.origin > capacityPlanes ||
     (current.origin != current.end && output == nullptr))
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology output finalization has invalid pointer, range, capacity, or plane size. Current: [{}, {}); final end: {}; capacity planes: {}; "
                                              "planeValues: {}; output null: {}.",
                                              current.origin, current.end, finalEnd, capacityPlanes, planeValues, output == nullptr));
  }
  const usize finalizedPlanes = finalEnd - current.origin;
  if(finalizedPlanes == 0)
  {
    return {};
  }

  usize writeStart = 0;
  usize writeValues = 0;
  usize capacityValues = 0;
  usize retainedValues = 0;
  const usize retainedPlanes = current.end - finalEnd;
  if(!ObjectMorphologyCheckedMultiply(current.origin, planeValues, writeStart) || !ObjectMorphologyCheckedMultiply(finalizedPlanes, planeValues, writeValues) ||
     !ObjectMorphologyCheckedMultiply(capacityPlanes, planeValues, capacityValues) || !ObjectMorphologyCheckedMultiply(retainedPlanes, planeValues, retainedValues) ||
     writeStart > std::numeric_limits<usize>::max() - writeValues || retainedValues > capacityValues || retainedValues > std::numeric_limits<usize>::max() / sizeof(T))
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology output finalization write/shift sizing overflow. Current: [{}, {}); final end: {}; finalized planes: {}; retained planes: {}; "
                                              "capacity planes: {}; planeValues: {}; write start: {}; write values: {}.",
                                              current.origin, current.end, finalEnd, finalizedPlanes, retainedPlanes, capacityPlanes, planeValues, writeStart, writeValues));
  }
  if(shouldCancel)
  {
    return {};
  }
  if(Result<> result = outStore.copyFromBuffer(writeStart, nonstd::span<const T>(output, writeValues)); result.invalid())
  {
    return result;
  }
  const usize oldEnd = current.end;
  // Pre-I/O capacity, dropped, and retained sizing proved this shift cannot fail after the successful write.
  auto retainedResult = ShiftObjectMorphologyPlaneWindowToFront(output, capacityPlanes, planeValues, oldEnd - current.origin, finalizedPlanes);
  if(retainedResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(retainedResult));
  }
  current = {finalEnd, oldEnd};
  return {};
}

struct ObjectMorphology2DPlan
{
  bool fullWidth = true;
  usize coreRows = 0;
  usize coreColumns = 0;
  usize inputCapacityValues = 0;
  usize maskCapacityValues = 0;
  usize outputCapacityValues = 0;
  usize residentBytes = 0;
};

inline Result<ObjectMorphology2DPlan> CreateObjectMorphology2DPlan(usize dimX, usize dimY, usize radiusX, usize radiusY, usize valueBytes, usize targetBytes)
{
  constexpr usize k_Max = std::numeric_limits<usize>::max();
  auto checkedAdd = [k_Max](usize left, usize right, usize& result) {
    if(left > k_Max - right)
    {
      return false;
    }
    result = left + right;
    return true;
  };
  auto checkedMultiply = [k_Max](usize left, usize right, usize& result) {
    if(left != 0 && right > k_Max / left)
    {
      return false;
    }
    result = left * right;
    return true;
  };

  if(dimX == 0 || dimY == 0 || valueBytes == 0 || targetBytes == 0 || radiusX > (k_Max - 2) / 2 || radiusY > (k_Max - 2) / 2)
  {
    return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology plan has invalid dimensions, radii, value size, or resident target.");
  }

  const usize inputHaloColumns = 2 * (radiusX + 1);
  const usize inputHaloRows = 2 * (radiusY + 1);
  const usize maskHaloColumns = 2 * radiusX;
  const usize maskHaloRows = 2 * radiusY;

  usize perCoreValueBytes = 0;
  if(!checkedMultiply(2, valueBytes, perCoreValueBytes) || !checkedAdd(perCoreValueBytes, 1, perCoreValueBytes))
  {
    return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology value size overflows its resident-byte calculation.");
  }

  usize fullWidthCoreBytes = 0;
  usize inputHaloBytes = 0;
  usize maskHaloBytes = 0;
  usize fullWidthFixedBytes = 0;
  if(!checkedMultiply(dimX, perCoreValueBytes, fullWidthCoreBytes) || !checkedMultiply(dimX, inputHaloRows, inputHaloBytes) || !checkedMultiply(inputHaloBytes, valueBytes, inputHaloBytes) ||
     !checkedMultiply(dimX, maskHaloRows, maskHaloBytes) || !checkedAdd(inputHaloBytes, maskHaloBytes, fullWidthFixedBytes))
  {
    return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology full-width plan overflows its resident-byte calculation.");
  }

  if(fullWidthFixedBytes <= targetBytes && fullWidthCoreBytes <= targetBytes - fullWidthFixedBytes)
  {
    const usize coreRows = std::min(dimY, (targetBytes - fullWidthFixedBytes) / fullWidthCoreBytes);
    ObjectMorphology2DPlan plan;
    plan.fullWidth = true;
    plan.coreRows = coreRows;
    plan.coreColumns = dimX;
    usize inputRows = 0;
    usize maskRows = 0;
    if(!checkedAdd(coreRows, inputHaloRows, inputRows) || !checkedAdd(coreRows, maskHaloRows, maskRows) || !checkedMultiply(inputRows, dimX, plan.inputCapacityValues) ||
       !checkedMultiply(maskRows, dimX, plan.maskCapacityValues) || !checkedMultiply(coreRows, dimX, plan.outputCapacityValues) ||
       !checkedMultiply(plan.inputCapacityValues, valueBytes, inputHaloBytes) || !checkedMultiply(plan.outputCapacityValues, valueBytes, fullWidthCoreBytes) ||
       !checkedAdd(inputHaloBytes, plan.maskCapacityValues, plan.residentBytes) || !checkedAdd(plan.residentBytes, fullWidthCoreBytes, plan.residentBytes))
    {
      return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology full-width capacities overflow.");
    }
    return {plan};
  }

  usize inputRows = 0;
  usize maskRows = 0;
  usize inputFixedValues = 0;
  usize maskFixedValues = 0;
  usize inputFixedBytes = 0;
  usize tiledFixedBytes = 0;
  usize perCoreColumnBytes = 0;
  if(!checkedAdd(inputHaloRows, 1, inputRows) || !checkedAdd(maskHaloRows, 1, maskRows) || !checkedMultiply(inputRows, inputHaloColumns, inputFixedValues) ||
     !checkedMultiply(maskRows, maskHaloColumns, maskFixedValues) || !checkedMultiply(inputFixedValues, valueBytes, inputFixedBytes) ||
     !checkedAdd(inputFixedBytes, maskFixedValues, tiledFixedBytes) || !checkedMultiply(inputRows, valueBytes, perCoreColumnBytes) || !checkedAdd(perCoreColumnBytes, maskRows, perCoreColumnBytes) ||
     !checkedAdd(perCoreColumnBytes, valueBytes, perCoreColumnBytes))
  {
    return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology tiled plan overflows its resident-byte calculation.");
  }
  if(tiledFixedBytes > targetBytes || perCoreColumnBytes > targetBytes - tiledFixedBytes)
  {
    return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology target cannot hold one output column and its input/mask halos.");
  }

  ObjectMorphology2DPlan plan;
  plan.fullWidth = false;
  plan.coreRows = 1;
  plan.coreColumns = std::min(dimX, (targetBytes - tiledFixedBytes) / perCoreColumnBytes);
  usize inputColumns = 0;
  usize maskColumns = 0;
  if(!checkedAdd(plan.coreColumns, inputHaloColumns, inputColumns) || !checkedAdd(plan.coreColumns, maskHaloColumns, maskColumns) ||
     !checkedMultiply(inputRows, inputColumns, plan.inputCapacityValues) || !checkedMultiply(maskRows, maskColumns, plan.maskCapacityValues) ||
     !checkedMultiply(plan.coreRows, plan.coreColumns, plan.outputCapacityValues) || !checkedMultiply(plan.inputCapacityValues, valueBytes, inputFixedBytes) ||
     !checkedMultiply(plan.outputCapacityValues, valueBytes, fullWidthCoreBytes) || !checkedAdd(inputFixedBytes, plan.maskCapacityValues, plan.residentBytes) ||
     !checkedAdd(plan.residentBytes, fullWidthCoreBytes, plan.residentBytes))
  {
    return MakeErrorResult<ObjectMorphology2DPlan>(-8670, "The bounded 2-D object-morphology tiled capacities overflow.");
  }
  return {plan};
}

template <class T>
struct ObjectBoundaryMask2DBody
{
  const T* input;
  uint8* mask;
  const SEOffset* boxOffsets;
  usize numBoxOffsets;
  usize dimX;
  usize dimY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize maskXBegin;
  usize maskYBegin;
  usize maskWidth;
  T objectValue;

  void operator()(const Range& range) const
  {
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize localX = tuple % maskWidth;
      const usize localY = tuple / maskWidth;
      const usize x = maskXBegin + localX;
      const usize y = maskYBegin + localY;
      const usize inputPosition = (y - inputYBegin) * inputWidth + (x - inputXBegin);
      bool boundary = false;
      if(input[inputPosition] == objectValue)
      {
        for(usize offsetIndex = 0; offsetIndex < numBoxOffsets; ++offsetIndex)
        {
          const SEOffset& offset = boxOffsets[offsetIndex];
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
          const usize neighbor = (static_cast<usize>(neighborY) - inputYBegin) * inputWidth + (static_cast<usize>(neighborX) - inputXBegin);
          if(input[neighbor] != objectValue)
          {
            boundary = true;
            break;
          }
        }
      }
      mask[tuple] = boundary ? uint8{1} : uint8{0};
    }
  }
};

template <class T>
struct ObjectMorphGather2DBody
{
  const T* input;
  const uint8* mask;
  T* output;
  const SEOffset* offsets;
  usize numOffsets;
  usize dimX;
  usize dimY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize maskXBegin;
  usize maskYBegin;
  usize maskWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;
  bool dilate;
  T objectValue;
  T backgroundValue;

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
      const T inputValue = input[(y - inputYBegin) * inputWidth + (x - inputXBegin)];
      bool hit = false;
      for(usize offsetIndex = 0; offsetIndex < numOffsets && !hit; ++offsetIndex)
      {
        const SEOffset& offset = offsets[offsetIndex];
        if(offset.dz != 0)
        {
          continue;
        }
        const int64 sourceX = static_cast<int64>(x) - offset.dx;
        const int64 sourceY = static_cast<int64>(y) - offset.dy;
        if(sourceX < 0 || sourceX >= dimXi || sourceY < 0 || sourceY >= dimYi)
        {
          continue;
        }
        const usize maskPosition = (static_cast<usize>(sourceY) - maskYBegin) * maskWidth + (static_cast<usize>(sourceX) - maskXBegin);
        hit = mask[maskPosition] != 0;
      }
      output[tuple] = dilate ? ((inputValue == objectValue || hit) ? objectValue : inputValue) : (hit ? backgroundValue : inputValue);
    }
  }
};

/**
 * @brief True iff global voxel (xi,yi,zi) is a boundary object pixel: its value equals @p objectValue AND at
 *        least one IN-BOUNDS full-box neighbor is != @p objectValue (out-of-bounds neighbors are ignored,
 *        matching ITK's @c UseBoundaryCondition == false). Reads only @p in, a flat [z][y][x] buffer whose
 *        first plane is global z == @p zOrigin (0 for a whole-volume buffer, @c zLo for an OOC slab). All
 *        coordinates are global; global-in-bounds is checked before every buffer index. @p boxOffsets /
 *        @p numBoxOffsets is the fixed 26-neighborhood passed as a raw pointer + count so the parallel plane
 *        bodies invoke this with no per-call allocation.
 */
template <class T>
inline bool IsBoundaryObjectPixel(const T* in, T objectValue, const SEOffset* boxOffsets, usize numBoxOffsets, int64 xi, int64 yi, int64 zi, int64 dimXi, int64 dimYi, int64 dimZi, usize zOrigin,
                                  usize sliceValues, usize dimX)
{
  const usize centerIdx = (static_cast<usize>(zi) - zOrigin) * sliceValues + static_cast<usize>(yi) * dimX + static_cast<usize>(xi);
  if(in[centerIdx] != objectValue)
  {
    return false;
  }
  for(usize i = 0; i < numBoxOffsets; ++i)
  {
    const SEOffset& o = boxOffsets[i];
    const int64 nx = xi + o.dx;
    const int64 ny = yi + o.dy;
    const int64 nz = zi + o.dz;
    if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
    {
      continue; // OOB neighbor ignored (matches ITK UseBoundaryCondition == false)
    }
    const usize nIdx = (static_cast<usize>(nz) - zOrigin) * sliceValues + static_cast<usize>(ny) * dimX + static_cast<usize>(nx);
    if(in[nIdx] != objectValue)
    {
      return true;
    }
  }
  return false;
}

template <class T>
// inputRange and outputRange describe the complete allocated backing windows; their depths are bounded by the caller's checked plan.
Result<> ScatterObjectMorphologyInputBatch(const T* input, const ObjectMorphologyPlaneWindow& inputRange, T* output, const ObjectMorphologyPlaneWindow& outputRange, usize outputCapacityPlanes,
                                           const ObjectMorphologyPlaneWindow& sourceRange, const SizeVec3& dims, const SEOffset* boxOffsets, usize numBoxOffsets, const SEOffset* paintOffsets,
                                           usize numPaintOffsets, T objectValue, T paintValue, const std::atomic_bool& shouldCancel)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0 || inputRange.end < inputRange.origin || outputRange.end < outputRange.origin || inputRange.end > dims[2] || outputRange.end > dims[2] ||
     sourceRange.end < sourceRange.origin || sourceRange.end > dims[2] || sourceRange.origin < inputRange.origin || sourceRange.end > inputRange.end ||
     outputRange.end - outputRange.origin > outputCapacityPlanes || (sourceRange.origin != sourceRange.end && (input == nullptr || output == nullptr)) ||
     (numBoxOffsets != 0 && boxOffsets == nullptr) || (numPaintOffsets != 0 && paintOffsets == nullptr))
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology batch scatter has invalid dimensions, ranges, pointers, or capacity. Input: [{}, {}); output: [{}, {}); source: [{}, {}); "
                                              "capacity: {}; dimensions: {}; box offsets: {}; paint offsets: {}.",
                                              inputRange.origin, inputRange.end, outputRange.origin, outputRange.end, sourceRange.origin, sourceRange.end, outputCapacityPlanes,
                                              StringUtilities::formatDimensions3D(dims), numBoxOffsets, numPaintOffsets));
  }
  if(sourceRange.origin == sourceRange.end)
  {
    return {};
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  usize sliceValues = 0;
  usize volumeValues = 0;
  usize outputCapacityValues = 0;
  if(dims[0] > k_Int64Max || dims[1] > k_Int64Max || dims[2] > k_Int64Max || !ObjectMorphologyCheckedMultiply(dims[0], dims[1], sliceValues) ||
     !ObjectMorphologyCheckedMultiply(sliceValues, dims[2], volumeValues) || !ObjectMorphologyCheckedMultiply(outputCapacityPlanes, sliceValues, outputCapacityValues))
  {
    return MakeErrorResult(-8766,
                           fmt::format("Object morphology batch scatter dimensions or output capacity do not fit signed coordinates or XY/XYZ value counts. Dimensions: {}; capacity planes: {}.",
                                       StringUtilities::formatDimensions3D(dims), outputCapacityPlanes));
  }
  int32 minimumBoxDz = 0;
  int32 maximumBoxDz = 0;
  int32 minimumPaintDz = 0;
  int32 maximumPaintDz = 0;
  int32 maximumPositiveOffset = 0;
  for(usize index = 0; index < numBoxOffsets; ++index)
  {
    minimumBoxDz = std::min(minimumBoxDz, boxOffsets[index].dz);
    maximumBoxDz = std::max(maximumBoxDz, boxOffsets[index].dz);
    maximumPositiveOffset = std::max({maximumPositiveOffset, boxOffsets[index].dx, boxOffsets[index].dy, boxOffsets[index].dz});
  }
  for(usize index = 0; index < numPaintOffsets; ++index)
  {
    minimumPaintDz = std::min(minimumPaintDz, paintOffsets[index].dz);
    maximumPaintDz = std::max(maximumPaintDz, paintOffsets[index].dz);
    maximumPositiveOffset = std::max({maximumPositiveOffset, paintOffsets[index].dx, paintOffsets[index].dy, paintOffsets[index].dz});
  }
  if(maximumPositiveOffset > 0 && (dims[0] > k_Int64Max - static_cast<usize>(maximumPositiveOffset) || dims[1] > k_Int64Max - static_cast<usize>(maximumPositiveOffset) ||
                                   dims[2] > k_Int64Max - static_cast<usize>(maximumPositiveOffset)))
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology batch scatter dimensions plus offsets exceed signed coordinates. Dimensions: {}; maximum positive offset: {}.",
                                              StringUtilities::formatDimensions3D(dims), maximumPositiveOffset));
  }
  const int64 sourceBegin = static_cast<int64>(sourceRange.origin);
  const int64 sourceEnd = static_cast<int64>(sourceRange.end);
  const int64 dimZi = static_cast<int64>(dims[2]);
  const int64 requiredInputBegin = std::max<int64>(0, sourceBegin + minimumBoxDz);
  const int64 requiredInputEnd = std::min<int64>(dimZi, sourceEnd + maximumBoxDz);
  const int64 requiredOutputBegin = std::max<int64>(0, sourceBegin + minimumPaintDz);
  const int64 requiredOutputEnd = std::min<int64>(dimZi, sourceEnd + maximumPaintDz);
  if(static_cast<int64>(inputRange.origin) > requiredInputBegin || static_cast<int64>(inputRange.end) < requiredInputEnd || static_cast<int64>(outputRange.origin) > requiredOutputBegin ||
     static_cast<int64>(outputRange.end) < requiredOutputEnd)
  {
    return MakeErrorResult(-8766, fmt::format("Object morphology batch scatter lacks input halo or output paint coverage. Input: [{}, {}); required input: [{}, {}); output: [{}, {}); "
                                              "required output: [{}, {}); source: [{}, {}); box dz: [{}, {}]; paint dz: [{}, {}].",
                                              inputRange.origin, inputRange.end, requiredInputBegin, requiredInputEnd, outputRange.origin, outputRange.end, requiredOutputBegin, requiredOutputEnd,
                                              sourceRange.origin, sourceRange.end, minimumBoxDz, maximumBoxDz, minimumPaintDz, maximumPaintDz));
  }
  const int64 dimXi = static_cast<int64>(dims[0]);
  const int64 dimYi = static_cast<int64>(dims[1]);
  for(usize z = sourceRange.origin; z < sourceRange.end; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    for(usize y = 0; y < dims[1]; ++y)
    {
      if((y & 0x3FU) == 0U && shouldCancel)
      {
        return {};
      }
      for(usize x = 0; x < dims[0]; ++x)
      {
        const int64 xi = static_cast<int64>(x);
        const int64 yi = static_cast<int64>(y);
        const int64 zi = static_cast<int64>(z);
        if(!IsBoundaryObjectPixel(input, objectValue, boxOffsets, numBoxOffsets, xi, yi, zi, dimXi, dimYi, dimZi, inputRange.origin, sliceValues, dims[0]))
        {
          continue;
        }
        for(usize index = 0; index < numPaintOffsets; ++index)
        {
          const SEOffset& offset = paintOffsets[index];
          const int64 targetX = xi + offset.dx;
          const int64 targetY = yi + offset.dy;
          const int64 targetZ = zi + offset.dz;
          if(targetX >= 0 && targetX < dimXi && targetY >= 0 && targetY < dimYi && targetZ >= 0 && targetZ < dimZi)
          {
            const usize localZ = static_cast<usize>(targetZ) - outputRange.origin;
            output[localZ * sliceValues + static_cast<usize>(targetY) * dims[0] + static_cast<usize>(targetX)] = paintValue;
          }
        }
      }
    }
  }
  return {};
}

} // namespace detail

/**
 * @brief In-core object morphology (Scatter path): the ITK-faithful reference engine.
 *
 * The whole array is pulled into a flat buffer once (in-core: bounded == the array) and the output is
 * seeded as a copy of the input. A single-threaded pass then visits every voxel; each boundary object pixel
 * (see @ref detail::IsBoundaryObjectPixel) paints the structuring-element offsets around itself into the
 * output (Dilate -> object value, Erode -> background value), skipping out-of-bounds paint targets. This is
 * ITK's exact scatter. It is deliberately single-threaded because paints from adjacent boundary pixels
 * overlap in the output and the legacy implementation's unsynchronized per-thread copy can overwrite a
 * cross-region paint. A single thread preserves the intended copy-then-paint semantics deterministically.
 * The sparse boundary set keeps it cheap.
 *
 * Boundary/object state is read from the ORIGINAL input buffer, never from the partially-painted output, so
 * there is no cascade. Constructor signature is identical to @ref ObjectMorphGather so both drop into
 * @ref DispatchAlgorithm.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class ObjectMorphScatter
{
public:
  ObjectMorphScatter(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const std::vector<SEOffset>& boxOffsets, ObjectMorphOp op,
                     T objectValue, T backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_BoxOffsets(boxOffsets)
  , m_Op(op)
  , m_ObjectValue(objectValue)
  , m_BackgroundValue(backgroundValue)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  ~ObjectMorphScatter() = default;

  ObjectMorphScatter(const ObjectMorphScatter&) = delete;
  ObjectMorphScatter(ObjectMorphScatter&&) noexcept = delete;
  ObjectMorphScatter& operator=(const ObjectMorphScatter&) = delete;
  ObjectMorphScatter& operator=(ObjectMorphScatter&&) noexcept = delete;

  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    const usize sliceValues = dimX * dimY;
    const usize vol = sliceValues * dimZ;
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    const int64 dimZi = static_cast<int64>(dimZ);
    const T paintValue = (m_Op == ObjectMorphOp::Dilate) ? m_ObjectValue : m_BackgroundValue;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying object morphology filter: {:.1f}%");
    auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    // Whole-volume resident read (in-core path only). Output starts as a copy of the input.
    auto inBuf = std::make_unique<T[]>(vol);
    if(Result<> r = m_In.copyIntoBuffer(0, nonstd::span<T>(inBuf.get(), vol)); r.invalid())
    {
      return r;
    }
    auto outBuf = std::make_unique<T[]>(vol);
    std::copy(inBuf.get(), inBuf.get() + vol, outBuf.get());

    for(usize z = 0; z < dimZ; ++z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const int64 zi = static_cast<int64>(z);
      for(usize y = 0; y < dimY; ++y)
      {
        const int64 yi = static_cast<int64>(y);
        for(usize x = 0; x < dimX; ++x)
        {
          const int64 xi = static_cast<int64>(x);
          if(!detail::IsBoundaryObjectPixel<T>(inBuf.get(), m_ObjectValue, m_BoxOffsets.data(), m_BoxOffsets.size(), xi, yi, zi, dimXi, dimYi, dimZi, 0, sliceValues, dimX))
          {
            continue;
          }
          for(const SEOffset& o : m_SE.offsets)
          {
            const int64 nx = xi + o.dx;
            const int64 ny = yi + o.dy;
            const int64 nz = zi + o.dz;
            if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
            {
              continue; // skip OOB paint target (matches ITK's valid-only SetPixel)
            }
            outBuf[static_cast<usize>(nz) * sliceValues + static_cast<usize>(ny) * dimX + static_cast<usize>(nx)] = paintValue;
          }
        }
      }
      progressMessenger.sendProgressMessage(1);
    }
    return m_Out.copyFromBuffer(0, nonstd::span<const T>(outBuf.get(), vol));
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  const std::vector<SEOffset>& m_BoxOffsets;
  ObjectMorphOp m_Op;
  T m_ObjectValue;
  T m_BackgroundValue;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Out-of-core-safe object morphology: the streamed, bounded-memory rolling-scatter engine.
 *
 * The in-core scatter writes into a whole-volume output at arbitrary (v + o) positions. This path retains a
 * rolling input halo and initialized output halo for normalized source-Z batches, then serially scatters only
 * boundary-object pixels into that output window. Completed output prefixes are bulk-written while the trailing
 * paint halo is retained for the next batch. The result preserves input-based boundary decisions, paint-offset
 * order, and no-cascade semantics. Memory is bounded at O((B+2*max(rz,1) + B+2*rz) * dimX * dimY), never O(volume).
 *
 * Constructor signature is identical to @ref ObjectMorphScatter so both drop into @ref DispatchAlgorithm.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 */
template <class T>
class ObjectMorphGather
{
public:
  ObjectMorphGather(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const std::vector<SEOffset>& boxOffsets, ObjectMorphOp op,
                    T objectValue, T backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize targetBufferBytes = 64ULL * 1024ULL * 1024ULL)
  : m_In(in)
  , m_Out(out)
  , m_Dims(dims)
  , m_SE(se)
  , m_BoxOffsets(boxOffsets)
  , m_Op(op)
  , m_ObjectValue(objectValue)
  , m_BackgroundValue(backgroundValue)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_TargetBufferBytes(targetBufferBytes)
  {
  }

  ~ObjectMorphGather() = default;

  ObjectMorphGather(const ObjectMorphGather&) = delete;
  ObjectMorphGather(ObjectMorphGather&&) noexcept = delete;
  ObjectMorphGather& operator=(const ObjectMorphGather&) = delete;
  ObjectMorphGather& operator=(ObjectMorphGather&&) noexcept = delete;

  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    // The paint SE's z-offsets span [-rz, rz]; a negative radius means an empty axis, so clamp to 0 (the
    // offset list is empty in that degenerate case anyway).
    const usize rz = (m_SE.radius[2] > 0) ? static_cast<usize>(m_SE.radius[2]) : 0;

    MessageHelper messageHelper(m_MessageHandler);
    auto progressHelper = messageHelper.createProgressMessageHelper();
    progressHelper.setMaxProgresss(dimZ);
    progressHelper.setProgressMessageTemplate("Applying object morphology filter: {:.1f}%");
    auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    if(dimZ == 1)
    {
      Result<> result = run2D();
      if(result.valid() && !m_ShouldCancel)
      {
        progressMessenger.sendProgressMessage(1);
      }
      return result;
    }

    usize sliceValues = 0;
    if(!detail::ObjectMorphologyCheckedMultiply(dimX, dimY, sliceValues))
    {
      return MakeErrorResult(-8764, fmt::format("Object morphology dimensions ({}) overflow while sizing the 3-D rolling-scatter plane.", StringUtilities::formatDimensions3D(m_Dims)));
    }
    auto planResult = detail::CreateObjectMorphology3DPlan<T>(m_Dims, rz, m_TargetBufferBytes);
    if(planResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(planResult));
    }
    const detail::ObjectMorphology3DPlan plan = planResult.value();

    return run3DRollingScatter(sliceValues, rz, plan, progressMessenger);
  }

private:
  Result<> run3DRollingScatter(usize sliceValues, usize radiusZ, const detail::ObjectMorphology3DPlan& plan, ProgressMessenger& progressMessenger)
  {
    if(plan.computeBatchDepth == 0 || plan.inputSlabDepth == 0 || plan.outputWindowDepth == 0)
    {
      return MakeErrorResult(-8766, fmt::format("Object morphology rolling scatter requires nonzero checked plan depths. Compute: {}; input: {}; output: {}; dimensions: {}.", plan.computeBatchDepth,
                                                plan.inputSlabDepth, plan.outputWindowDepth, StringUtilities::formatDimensions3D(m_Dims)));
    }
    usize inputWindowValues = 0;
    usize outputWindowValues = 0;
    if(!detail::ObjectMorphologyCheckedMultiply(plan.inputSlabDepth, sliceValues, inputWindowValues) ||
       !detail::ObjectMorphologyCheckedMultiply(plan.outputWindowDepth, sliceValues, outputWindowValues) || inputWindowValues > std::numeric_limits<usize>::max() / sizeof(T) ||
       outputWindowValues > std::numeric_limits<usize>::max() / sizeof(T))
    {
      return MakeErrorResult(-8766,
                             fmt::format("Object morphology rolling scatter window allocation overflow. Input depth: {}; output depth: {}; plane values: {}; input values: {}; output values: {}; "
                                         "dimensions: {}.",
                                         plan.inputSlabDepth, plan.outputWindowDepth, sliceValues, inputWindowValues, outputWindowValues, StringUtilities::formatDimensions3D(m_Dims)));
    }
    auto inputWindow = std::make_unique_for_overwrite<T[]>(inputWindowValues);
    auto outputWindow = std::make_unique_for_overwrite<T[]>(outputWindowValues);
    detail::ObjectMorphologyPlaneWindow inputRange;
    detail::ObjectMorphologyPlaneWindow outputRange;
    const usize inputHalo = std::max<usize>(radiusZ, 1);
    const T paintValue = m_Op == ObjectMorphOp::Dilate ? m_ObjectValue : m_BackgroundValue;

    for(usize batchBegin = 0; batchBegin < m_Dims[2];)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize remainingDepth = m_Dims[2] - batchBegin;
      const usize batchDepth = std::min(plan.computeBatchDepth, remainingDepth);
      const usize batchEnd = batchBegin + batchDepth;
      const detail::ObjectMorphologyPlaneWindow sourceRange{batchBegin, batchEnd};
      auto desiredInputResult = detail::CreateObjectMorphologyPlaneWindow(batchBegin, batchEnd, inputHalo, m_Dims[2]);
      if(desiredInputResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(desiredInputResult));
      }
      if(Result<> result = detail::AdvanceObjectMorphologyInputWindow(m_In, inputWindow.get(), plan.inputSlabDepth, sliceValues, inputRange, desiredInputResult.value()); result.invalid())
      {
        return result;
      }
      auto desiredOutputResult = detail::CreateObjectMorphologyPlaneWindow(batchBegin, batchEnd, radiusZ, m_Dims[2]);
      if(desiredOutputResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(desiredOutputResult));
      }
      if(Result<> result =
             detail::AdvanceObjectMorphologyOutputWindow(outputWindow.get(), plan.outputWindowDepth, sliceValues, outputRange, desiredOutputResult.value(), inputWindow.get(), inputRange);
         result.invalid())
      {
        return result;
      }
      if(Result<> result = detail::ScatterObjectMorphologyInputBatch(inputWindow.get(), inputRange, outputWindow.get(), outputRange, plan.outputWindowDepth, sourceRange, m_Dims, m_BoxOffsets.data(),
                                                                     m_BoxOffsets.size(), m_SE.offsets.data(), m_SE.offsets.size(), m_ObjectValue, paintValue, m_ShouldCancel);
         result.invalid())
      {
        return result;
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize finalEnd = batchEnd == m_Dims[2] ? m_Dims[2] : std::max(outputRange.origin, batchEnd > radiusZ ? batchEnd - radiusZ : 0);
      if(Result<> result = detail::FinalizeObjectMorphologyOutputWindow(m_Out, outputWindow.get(), plan.outputWindowDepth, sliceValues, outputRange, finalEnd, m_ShouldCancel); result.invalid())
      {
        return result;
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      progressMessenger.sendProgressMessage(batchEnd - batchBegin);
      batchBegin = batchEnd;
    }
    if(outputRange.origin != outputRange.end)
    {
      return MakeErrorResult(-8766, fmt::format("Object morphology rolling scatter completed with an unfinalized output range. Output: [{}, {}); input: [{}, {}); dimensions: {}.", outputRange.origin,
                                                outputRange.end, inputRange.origin, inputRange.end, StringUtilities::formatDimensions3D(m_Dims)));
    }
    return {};
  }

  Result<> run2D()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize radiusX = m_SE.radius[0] > 0 ? static_cast<usize>(m_SE.radius[0]) : 0;
    const usize radiusY = m_SE.radius[1] > 0 ? static_cast<usize>(m_SE.radius[1]) : 0;
    auto planResult = detail::CreateObjectMorphology2DPlan(dimX, dimY, radiusX, radiusY, sizeof(T), m_TargetBufferBytes);
    if(planResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(planResult));
    }
    const detail::ObjectMorphology2DPlan plan = planResult.value();
    if(dimX > std::numeric_limits<usize>::max() / dimY)
    {
      return MakeErrorResult(-8671, "The bounded 2-D object-morphology dimensions overflow the addressable value count.");
    }
    const usize totalValues = dimX * dimY;
    if(m_In.getSize() != totalValues || m_Out.getSize() != totalValues)
    {
      return MakeErrorResult(-8671, "The bounded 2-D object-morphology input/output stores do not match the image dimensions.");
    }

    auto inputBuffer = std::make_unique<T[]>(plan.inputCapacityValues);
    auto maskBuffer = std::make_unique<uint8[]>(plan.maskCapacityValues);
    auto outputBuffer = std::make_unique<T[]>(plan.outputCapacityValues);
    const bool dilate = m_Op == ObjectMorphOp::Dilate;

    const usize yStep = plan.fullWidth ? plan.coreRows : 1;
    const usize xStep = plan.fullWidth ? dimX : plan.coreColumns;
    for(usize outputYBegin = 0; outputYBegin < dimY; outputYBegin += yStep)
    {
      const usize outputRows = std::min(yStep, dimY - outputYBegin);
      for(usize outputXBegin = 0; outputXBegin < dimX; outputXBegin += xStep)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize outputColumns = std::min(xStep, dimX - outputXBegin);
        const usize outputXEnd = outputXBegin + outputColumns;
        const usize outputYEnd = outputYBegin + outputRows;
        const usize maskXBegin = outputXBegin > radiusX ? outputXBegin - radiusX : 0;
        const usize maskYBegin = outputYBegin > radiusY ? outputYBegin - radiusY : 0;
        const usize maskXEnd = outputXEnd + std::min(radiusX, dimX - outputXEnd);
        const usize maskYEnd = outputYEnd + std::min(radiusY, dimY - outputYEnd);
        const usize inputXBegin = maskXBegin > 0 ? maskXBegin - 1 : 0;
        const usize inputYBegin = maskYBegin > 0 ? maskYBegin - 1 : 0;
        const usize inputXEnd = maskXEnd < dimX ? maskXEnd + 1 : maskXEnd;
        const usize inputYEnd = maskYEnd < dimY ? maskYEnd + 1 : maskYEnd;
        const usize inputWidth = inputXEnd - inputXBegin;
        const usize maskWidth = maskXEnd - maskXBegin;
        const usize maskRows = maskYEnd - maskYBegin;

        for(usize inputY = inputYBegin; inputY < inputYEnd; ++inputY)
        {
          if(Result<> result = m_In.copyIntoBuffer(inputY * dimX + inputXBegin, nonstd::span<T>(inputBuffer.get() + (inputY - inputYBegin) * inputWidth, inputWidth)); result.invalid())
          {
            return result;
          }
        }

        ParallelDataAlgorithm maskAlgorithm;
        maskAlgorithm.setRange(0, maskRows * maskWidth);
        maskAlgorithm.execute(detail::ObjectBoundaryMask2DBody<T>{.input = inputBuffer.get(),
                                                                  .mask = maskBuffer.get(),
                                                                  .boxOffsets = m_BoxOffsets.data(),
                                                                  .numBoxOffsets = m_BoxOffsets.size(),
                                                                  .dimX = dimX,
                                                                  .dimY = dimY,
                                                                  .inputXBegin = inputXBegin,
                                                                  .inputYBegin = inputYBegin,
                                                                  .inputWidth = inputWidth,
                                                                  .maskXBegin = maskXBegin,
                                                                  .maskYBegin = maskYBegin,
                                                                  .maskWidth = maskWidth,
                                                                  .objectValue = m_ObjectValue});

        ParallelDataAlgorithm gatherAlgorithm;
        gatherAlgorithm.setRange(0, outputRows * outputColumns);
        gatherAlgorithm.execute(detail::ObjectMorphGather2DBody<T>{.input = inputBuffer.get(),
                                                                   .mask = maskBuffer.get(),
                                                                   .output = outputBuffer.get(),
                                                                   .offsets = m_SE.offsets.data(),
                                                                   .numOffsets = m_SE.offsets.size(),
                                                                   .dimX = dimX,
                                                                   .dimY = dimY,
                                                                   .inputXBegin = inputXBegin,
                                                                   .inputYBegin = inputYBegin,
                                                                   .inputWidth = inputWidth,
                                                                   .maskXBegin = maskXBegin,
                                                                   .maskYBegin = maskYBegin,
                                                                   .maskWidth = maskWidth,
                                                                   .outputXBegin = outputXBegin,
                                                                   .outputYBegin = outputYBegin,
                                                                   .outputWidth = outputColumns,
                                                                   .dilate = dilate,
                                                                   .objectValue = m_ObjectValue,
                                                                   .backgroundValue = m_BackgroundValue});

        if(plan.fullWidth)
        {
          if(Result<> result = m_Out.copyFromBuffer(outputYBegin * dimX, nonstd::span<const T>(outputBuffer.get(), outputRows * dimX)); result.invalid())
          {
            return result;
          }
        }
        else if(Result<> result = m_Out.copyFromBuffer(outputYBegin * dimX + outputXBegin, nonstd::span<const T>(outputBuffer.get(), outputColumns)); result.invalid())
        {
          return result;
        }
      }
    }
    return {};
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  const StructuringElement& m_SE;
  const std::vector<SEOffset>& m_BoxOffsets;
  ObjectMorphOp m_Op;
  T m_ObjectValue;
  T m_BackgroundValue;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_TargetBufferBytes;
};

/**
 * @brief Public entry point for the object-morphology engine.
 *
 * Dispatches on storage type via @ref DispatchAlgorithm: the ITK-faithful in-core @ref ObjectMorphScatter
 * path when all data is resident, or the bounded-memory rolling-scatter @ref ObjectMorphGather path when any array
 * is chunked (or when a test forces the OOC path). Both paths produce byte-identical output. @p boxOffsets is
 * the fixed radius-1 full-box boundary neighborhood (built once by the caller via
 * @ref detail::MakeFullBoxNeighborOffsets); @p se is the paint structuring element.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
Result<> ApplyObjectMorphology(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const StructuringElement& se, const std::vector<SEOffset>& boxOffsets, ObjectMorphOp op,
                               T objectValue, T backgroundValue, const IDataArray& inArray, const IDataArray& outArray, const std::atomic_bool& shouldCancel,
                               const IFilter::MessageHandler& messageHandler)
{
  const bool usesOutOfCoreStore = in.getStoreType() == IDataStore::StoreType::OutOfCore || out.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(usesOutOfCoreStore && !ForceInCoreAlgorithm() && detail::ShouldUseObjectMorphologyResidentState(dims))
  {
    auto allocationResult = detail::ReserveObjectMorphologyResidentWorkingMemory<T>(dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        ObjectMorphScatter<T> scatter{in, out, dims, se, boxOffsets, op, objectValue, backgroundValue, shouldCancel, messageHandler};
        return scatter();
      } catch(const std::bad_alloc&)
      {
        // Reuse the complete reservation for the bounded gather fallback.
      }
    }
    if(allocation.reservation.sizeBytes() > 0 && allocation.reservation.sizeBytes() <= std::numeric_limits<usize>::max())
    {
      const usize radiusZ = se.radius[2] > 0 ? static_cast<usize>(se.radius[2]) : 0;
      auto planResult = detail::CreateObjectMorphology3DPlan<T>(dims, radiusZ, static_cast<usize>(allocation.reservation.sizeBytes()));
      if(planResult.invalid())
      {
        if(!planResult.errors().empty() && planResult.errors().front().code == -8765)
        {
          allocation.reservation.shrinkTo(0);
        }
        else
        {
          return ConvertInvalidResult<void>(std::move(planResult));
        }
      }
      else
      {
        allocation.reservation.shrinkTo(static_cast<uint64>(planResult.value().residentBytes));
        ObjectMorphGather<T> gather{in, out, dims, se, boxOffsets, op, objectValue, backgroundValue, shouldCancel, messageHandler, static_cast<usize>(allocation.reservation.sizeBytes())};
        return gather();
      }
    }
  }
  return DispatchAlgorithm<ObjectMorphScatter<T>, ObjectMorphGather<T>>({&inArray, &outArray}, in, out, dims, se, boxOffsets, op, objectValue, backgroundValue, shouldCancel, messageHandler);
}
} // namespace nx::core::ImageProcessing
