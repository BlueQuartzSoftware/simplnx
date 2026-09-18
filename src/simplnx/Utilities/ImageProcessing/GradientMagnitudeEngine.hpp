#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
inline constexpr usize k_GradientMagnitude2DTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_GradientMagnitude2DMetadataBytes = 64ULL * 1024ULL;

inline bool GradientMagnitudeCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool GradientMagnitudeCheckedAdd(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

struct GradientMagnitudeResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseGradientMagnitudeResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateGradientMagnitudeResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize residentValueBytes = 0;
  usize residentBytes = 0;
  usize rollingInputBytes = 0;
  usize rollingValueBytes = 0;
  usize rollingBytes = 0;
  usize requiredBytes = 0;
  if(!GradientMagnitudeCheckedMultiply(dims[0], dims[1], sliceValues) || !GradientMagnitudeCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !GradientMagnitudeCheckedAdd(sizeof(T), sizeof(float32), residentValueBytes) || !GradientMagnitudeCheckedMultiply(volumeValues, residentValueBytes, residentBytes) ||
     !GradientMagnitudeCheckedMultiply(sizeof(T), usize{3}, rollingInputBytes) || !GradientMagnitudeCheckedAdd(rollingInputBytes, sizeof(float32), rollingValueBytes) ||
     !GradientMagnitudeCheckedMultiply(sliceValues, rollingValueBytes, rollingBytes) || !GradientMagnitudeCheckedAdd(residentBytes, rollingBytes, requiredBytes))
  {
    return MakeErrorResult<usize>(
        -8760, fmt::format("Gradient magnitude dimensions ({}) and input element size ({} bytes) overflow while sizing the resident input, float32 output, and rolling-plane working state.",
                           StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<GradientMagnitudeResidentMemoryAllocation> ReserveGradientMagnitudeResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateGradientMagnitudeResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<GradientMagnitudeResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {GradientMagnitudeResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

enum class GradientMagnitude2DRoute : uint8
{
  Rows,
  Tiles
};

struct GradientMagnitude2DPlan
{
  GradientMagnitude2DRoute route = GradientMagnitude2DRoute::Rows;
  usize blockRows = 0;
  usize tileColumns = 0;
  usize inputValues = 0;
  usize outputValues = 0;
  usize residentBytes = 0;
};

template <class T>
Result<GradientMagnitude2DPlan> CreateGradientMagnitude2DPlan(const SizeVec3& dims, usize targetBytes = k_GradientMagnitude2DTargetBytes)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  if(dimX == 0 || dimY == 0 || dims[2] != 1)
  {
    return MakeErrorResult<GradientMagnitude2DPlan>(-8754,
                                                    fmt::format("Gradient magnitude true-2-D planning requires positive X/Y dimensions and Z=1, but received {} x {} x {}.", dimX, dimY, dims[2]));
  }

  usize cellCount = 0;
  usize inputRowBytes = 0;
  usize outputRowBytes = 0;
  if(!GradientMagnitudeCheckedMultiply(dimX, dimY, cellCount) || !GradientMagnitudeCheckedMultiply(dimX, sizeof(T), inputRowBytes) ||
     !GradientMagnitudeCheckedMultiply(dimX, sizeof(float32), outputRowBytes))
  {
    return MakeErrorResult<GradientMagnitude2DPlan>(
        -8755, fmt::format("Gradient magnitude true-2-D planning overflowed for dimensions {} x {} x {} and {}-byte input values.", dimX, dimY, dims[2], sizeof(T)));
  }

  if(targetBytes <= k_GradientMagnitude2DMetadataBytes)
  {
    return MakeErrorResult<GradientMagnitude2DPlan>(-8756, fmt::format("Gradient magnitude true-2-D target ({} bytes) must exceed the {}-byte metadata reserve for dimensions {} x {} x {}.",
                                                                       targetBytes, k_GradientMagnitude2DMetadataBytes, dimX, dimY, dims[2]));
  }
  const usize availableBytes = targetBytes - k_GradientMagnitude2DMetadataBytes;

  usize haloBytes = 0;
  usize bytesPerOutputRow = 0;
  usize minimumRowBytes = 0;
  if(!GradientMagnitudeCheckedMultiply(inputRowBytes, 2, haloBytes) || !GradientMagnitudeCheckedAdd(inputRowBytes, outputRowBytes, bytesPerOutputRow) ||
     !GradientMagnitudeCheckedAdd(haloBytes, bytesPerOutputRow, minimumRowBytes))
  {
    return MakeErrorResult<GradientMagnitude2DPlan>(
        -8755, fmt::format("Gradient magnitude true-2-D row planning overflowed for dimensions {} x {} x {} and {}-byte input values.", dimX, dimY, dims[2], sizeof(T)));
  }

  GradientMagnitude2DPlan plan;
  if(minimumRowBytes <= availableBytes)
  {
    plan.route = GradientMagnitude2DRoute::Rows;
    plan.blockRows = std::min(dimY, (availableBytes - haloBytes) / bytesPerOutputRow);
    plan.tileColumns = dimX;
    usize inputRows = 0;
    if(!GradientMagnitudeCheckedAdd(plan.blockRows, 2, inputRows) || !GradientMagnitudeCheckedMultiply(inputRows, dimX, plan.inputValues) ||
       !GradientMagnitudeCheckedMultiply(plan.blockRows, dimX, plan.outputValues))
    {
      return MakeErrorResult<GradientMagnitude2DPlan>(
          -8755, fmt::format("Gradient magnitude true-2-D row-buffer planning overflowed for dimensions {} x {} x {} and {} block rows.", dimX, dimY, dims[2], plan.blockRows));
    }
  }
  else
  {
    constexpr usize k_InputRows = 3;
    constexpr usize k_HaloValues = 2 * k_InputRows;
    usize fixedHaloBytes = 0;
    usize inputColumnBytes = 0;
    usize bytesPerOutputColumn = 0;
    if(!GradientMagnitudeCheckedMultiply(k_HaloValues, sizeof(T), fixedHaloBytes) || !GradientMagnitudeCheckedMultiply(k_InputRows, sizeof(T), inputColumnBytes) ||
       !GradientMagnitudeCheckedAdd(inputColumnBytes, sizeof(float32), bytesPerOutputColumn) || fixedHaloBytes >= availableBytes)
    {
      return MakeErrorResult<GradientMagnitude2DPlan>(
          -8756, fmt::format("Gradient magnitude true-2-D target ({} bytes) cannot hold one haloed output tile for dimensions {} x {} x {} and {}-byte input values.", targetBytes, dimX, dimY, dims[2],
                             sizeof(T)));
    }

    plan.route = GradientMagnitude2DRoute::Tiles;
    plan.blockRows = 1;
    plan.tileColumns = std::min(dimX, (availableBytes - fixedHaloBytes) / bytesPerOutputColumn);
    usize haloedColumns = 0;
    if(plan.tileColumns == 0 || !GradientMagnitudeCheckedAdd(plan.tileColumns, 2, haloedColumns) || !GradientMagnitudeCheckedMultiply(k_InputRows, haloedColumns, plan.inputValues))
    {
      return MakeErrorResult<GradientMagnitude2DPlan>(
          -8756, fmt::format("Gradient magnitude true-2-D target ({} bytes) cannot hold one output column and its radius-one halo for dimensions {} x {} x {} and {}-byte input values.", targetBytes,
                             dimX, dimY, dims[2], sizeof(T)));
    }
    plan.outputValues = plan.tileColumns;
  }

  usize inputBytes = 0;
  usize outputBytes = 0;
  usize residentCellBytes = 0;
  if(!GradientMagnitudeCheckedMultiply(plan.inputValues, sizeof(T), inputBytes) || !GradientMagnitudeCheckedMultiply(plan.outputValues, sizeof(float32), outputBytes) ||
     !GradientMagnitudeCheckedAdd(inputBytes, outputBytes, residentCellBytes) || !GradientMagnitudeCheckedAdd(residentCellBytes, k_GradientMagnitude2DMetadataBytes, plan.residentBytes) ||
     plan.residentBytes > targetBytes)
  {
    return MakeErrorResult<GradientMagnitude2DPlan>(-8755,
                                                    fmt::format("Gradient magnitude true-2-D resident-byte accounting overflowed for dimensions {} x {} x {}, {} input values, and {} output values.",
                                                                dimX, dimY, dims[2], plan.inputValues, plan.outputValues));
  }
  return {plan};
}

template <class T>
Result<> ApplyGradientMagnitude2DRows(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, double invSpacingX, double invSpacingY,
                                      const std::atomic_bool& shouldCancel, const GradientMagnitude2DPlan& plan)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  std::vector<T> inputBuffer(plan.inputValues);
  std::vector<float32> outputBuffer(plan.outputValues);

  for(usize yStart = 0; yStart < dimY;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize rowCount = std::min(plan.blockRows, dimY - yStart);
    const usize readYStart = yStart == 0 ? 0 : yStart - 1;
    const usize outputYEnd = yStart + rowCount;
    const usize readYEnd = outputYEnd == dimY ? dimY : outputYEnd + 1;
    const usize readValues = (readYEnd - readYStart) * dimX;
    if(Result<> result = inStore.copyIntoBuffer(readYStart * dimX, nonstd::span<T>(inputBuffer.data(), readValues)); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }

    auto calculateRows = [&](const Range& range) {
      for(usize localY = range.min(); localY < range.max(); ++localY)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize y = yStart + localY;
        const usize previousY = y == 0 ? 0 : y - 1;
        const usize nextY = y + 1 == dimY ? y : y + 1;
        const usize currentRow = (y - readYStart) * dimX;
        const usize previousRow = (previousY - readYStart) * dimX;
        const usize nextRow = (nextY - readYStart) * dimX;
        const usize outputRow = localY * dimX;
        for(usize x = 0; x < dimX; ++x)
        {
          const usize previousX = x == 0 ? 0 : x - 1;
          const usize nextX = x + 1 == dimX ? x : x + 1;
          const double gradientX = 0.5 * (static_cast<double>(inputBuffer[currentRow + previousX]) - static_cast<double>(inputBuffer[currentRow + nextX])) * invSpacingX;
          const double gradientY = 0.5 * (static_cast<double>(inputBuffer[previousRow + x]) - static_cast<double>(inputBuffer[nextRow + x])) * invSpacingY;
          const double magnitudeSquared = gradientX * gradientX + gradientY * gradientY;
          outputBuffer[outputRow + x] = static_cast<float32>(std::sqrt(magnitudeSquared));
        }
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, rowCount);
    parallelAlgorithm.execute(calculateRows);
    if(shouldCancel)
    {
      return {};
    }
    const usize outputValues = rowCount * dimX;
    if(Result<> result = outStore.copyFromBuffer(yStart * dimX, nonstd::span<const float32>(outputBuffer.data(), outputValues)); result.invalid())
    {
      return result;
    }
    yStart = outputYEnd;
  }
  return {};
}

template <class T>
Result<> ApplyGradientMagnitude2DTiles(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, double invSpacingX, double invSpacingY,
                                       const std::atomic_bool& shouldCancel, const GradientMagnitude2DPlan& plan)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  std::array<std::vector<T>, 3> inputRows;
  for(auto& inputRow : inputRows)
  {
    inputRow.resize(plan.tileColumns + 2);
  }
  std::vector<float32> outputBuffer(plan.outputValues);

  for(usize y = 0; y < dimY; ++y)
  {
    const std::array<usize, 3> rowIndices = {y == 0 ? 0 : y - 1, y, y + 1 == dimY ? y : y + 1};
    for(usize xStart = 0; xStart < dimX;)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize tileColumns = std::min(plan.tileColumns, dimX - xStart);
      const usize readXStart = xStart == 0 ? 0 : xStart - 1;
      const usize outputXEnd = xStart + tileColumns;
      const usize readXEnd = outputXEnd == dimX ? dimX : outputXEnd + 1;
      const usize readColumns = readXEnd - readXStart;
      for(usize rowSlot = 0; rowSlot < rowIndices.size(); ++rowSlot)
      {
        if(Result<> result = inStore.copyIntoBuffer(rowIndices[rowSlot] * dimX + readXStart, nonstd::span<T>(inputRows[rowSlot].data(), readColumns)); result.invalid())
        {
          return result;
        }
      }
      if(shouldCancel)
      {
        return {};
      }

      auto calculateColumns = [&](const Range& range) {
        for(usize localX = range.min(); localX < range.max(); ++localX)
        {
          const usize x = xStart + localX;
          const usize previousX = x == 0 ? 0 : x - 1;
          const usize nextX = x + 1 == dimX ? x : x + 1;
          const usize centerOffset = x - readXStart;
          const usize previousOffset = previousX - readXStart;
          const usize nextOffset = nextX - readXStart;
          const double gradientX = 0.5 * (static_cast<double>(inputRows[1][previousOffset]) - static_cast<double>(inputRows[1][nextOffset])) * invSpacingX;
          const double gradientY = 0.5 * (static_cast<double>(inputRows[0][centerOffset]) - static_cast<double>(inputRows[2][centerOffset])) * invSpacingY;
          const double magnitudeSquared = gradientX * gradientX + gradientY * gradientY;
          outputBuffer[localX] = static_cast<float32>(std::sqrt(magnitudeSquared));
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, tileColumns);
      parallelAlgorithm.execute(calculateColumns);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = outStore.copyFromBuffer(y * dimX + xStart, nonstd::span<const float32>(outputBuffer.data(), tileColumns)); result.invalid())
      {
        return result;
      }
      xStart = outputXEnd;
    }
  }
  return {};
}

template <class T>
Result<> ApplyGradientMagnitude2D(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, bool useSpacing, FloatVec3 spacing,
                                  const std::atomic_bool& shouldCancel, usize targetBytes = k_GradientMagnitude2DTargetBytes)
{
  if(shouldCancel)
  {
    return {};
  }
  auto planResult = CreateGradientMagnitude2DPlan<T>(dims, targetBytes);
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const GradientMagnitude2DPlan& plan = planResult.value();
  const double invSpacingX = useSpacing ? 1.0 / static_cast<double>(spacing[0]) : 1.0;
  const double invSpacingY = useSpacing ? 1.0 / static_cast<double>(spacing[1]) : 1.0;
  if(plan.route == GradientMagnitude2DRoute::Rows)
  {
    return ApplyGradientMagnitude2DRows(inStore, outStore, dims, invSpacingX, invSpacingY, shouldCancel, plan);
  }
  return ApplyGradientMagnitude2DTiles(inStore, outStore, dims, invSpacingX, invSpacingY, shouldCancel, plan);
}
} // namespace detail

// ITK-free port of GradientMagnitudeImageFilter: single streamed pass, order-1 central difference [0.5,0,-0.5] per
// axis (scaled by 1/spacing when useSpacing), out = sqrt(sum of squares), accumulated in double, stored float32,
// ZeroFluxNeumann (edge-clamp) boundary. effDim = (dims[2]>1)?3:2. Resident and bounded-fallback true-3-D calls use
// a rolling 3-plane Z window. A true-3-D call with a real OOC endpoint first tries one budgeted full-volume transfer
// in each direction. A true-2-D call with an OOC endpoint uses checked row blocks or overwide X tiles instead.
// `messageHandler` is currently unused: progress messaging is deferred (see engine-level TODO); the parameter is kept
// for facade signature parity with the other ImageProcessing engines.
template <class T>
Result<> ApplyGradientMagnitude(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, bool useSpacing, FloatVec3 spacing,
                                const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_GradientMagnitude2DTargetBytes)
{
  const usize nX = dims[0];
  const usize nY = dims[1];
  const usize nZ = dims[2];
  if(nX == 0 || nY == 0 || nZ == 0)
  {
    return MakeErrorResult(-8356, fmt::format("Gradient magnitude requires nonzero image dimensions. Dimensions: {} x {} x {}.", nX, nY, nZ));
  }

  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  usize slice = 0;
  usize volume = 0;
  if(!checkedMultiply(nX, nY, slice) || !checkedMultiply(slice, nZ, volume))
  {
    return MakeErrorResult(-8357, fmt::format("Gradient magnitude image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", nX, nY, nZ));
  }
  if(inStore.getSize() != volume)
  {
    return MakeErrorResult(-8358, fmt::format("Gradient magnitude input store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              inStore.getSize(), volume, nX, nY, nZ));
  }
  if(outStore.getSize() != volume)
  {
    return MakeErrorResult(-8359, fmt::format("Gradient magnitude output store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              outStore.getSize(), volume, nX, nY, nZ));
  }

  const bool hasZ = nZ > 1;
  if(useSpacing)
  {
    const usize effectiveDimensions = hasZ ? 3 : 2;
    for(usize axis = 0; axis < effectiveDimensions; ++axis)
    {
      if(!(std::abs(static_cast<double>(spacing[axis])) > 0.0))
      {
        return MakeErrorResult(k_GradientMagnitudeZeroSpacing,
                               fmt::format("Gradient magnitude image spacing for filtered axis {} is degenerate ({}); its absolute value must be greater than zero.", axis, spacing[axis]));
      }
    }
  }
  if(shouldCancel)
  {
    return {};
  }

  const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(!hasZ)
  {
    const bool useBounded2D = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(useBounded2D ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
    if(useBounded2D)
    {
      return detail::ApplyGradientMagnitude2D(inStore, outStore, dims, useSpacing, spacing, shouldCancel, target2DBytes);
    }
  }

  if(usesOutOfCoreStore && !ForceInCoreAlgorithm() && detail::ShouldUseGradientMagnitudeResidentState(dims))
  {
    auto allocationResult = detail::ReserveGradientMagnitudeResidentWorkingMemory<T>(dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        DataStore<T> residentInput(ShapeType{nZ, nY, nX}, ShapeType{1}, std::nullopt);
        DataStore<float32> residentOutput(ShapeType{nZ, nY, nX}, ShapeType{1}, std::nullopt);
        if(Result<> result = inStore.copyIntoBuffer(0, residentInput.createSpan()); result.invalid())
        {
          return result;
        }
        if(Result<> result = ApplyGradientMagnitude(residentInput, residentOutput, dims, useSpacing, spacing, shouldCancel, messageHandler, target2DBytes); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        const auto outputSpan = residentOutput.createSpan();
        return outStore.copyFromBuffer(0, nonstd::span<const float32>(outputSpan.data(), outputSpan.size()));
      } catch(const std::bad_alloc&)
      {
        // Release the complete-state reservation before entering the rolling-plane fallback.
      }
    }
  }

  const double invSpX = useSpacing ? (1.0 / static_cast<double>(spacing[0])) : 1.0;
  const double invSpY = useSpacing ? (1.0 / static_cast<double>(spacing[1])) : 1.0;
  const double invSpZ = useSpacing && hasZ ? (1.0 / static_cast<double>(spacing[2])) : 1.0;

  // 3-plane rolling input window: win[0]=z-1, win[1]=z, win[2]=z+1 (clamped). For 2D (nZ==1) all three alias plane 0.
  std::array<std::vector<T>, 3> win;
  for(auto& p : win)
  {
    p.resize(slice);
  }
  std::vector<float32> outPlane(slice);

  auto loadPlane = [&](std::vector<T>& dst, usize z) -> Result<> { return inStore.copyIntoBuffer(z * slice, nonstd::span<T>(dst.data(), slice)); };

  if(Result<> r = loadPlane(win[0], 0); r.invalid())
  {
    return r;
  }
  std::copy(win[0].cbegin(), win[0].cend(), win[1].begin());
  if(hasZ)
  {
    if(Result<> r = loadPlane(win[2], 1); r.invalid())
    {
      return r;
    }
  }
  else
  {
    std::copy(win[0].cbegin(), win[0].cend(), win[2].begin());
  }

  for(usize z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto& pm = win[0]; // z-1
    const auto& p0 = win[1]; // z
    const auto& pp = win[2]; // z+1
    auto calculateRows = [&](const Range& range) {
      if(shouldCancel)
      {
        return;
      }
      for(usize y = range.min(); y < range.max(); ++y)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize row = y * nX;
        const usize previousRow = (y == 0 ? 0 : y - 1) * nX;
        const usize nextRow = (y + 1 == nY ? y : y + 1) * nX;
        for(usize x = 0; x < nX; ++x)
        {
          const usize previousX = x == 0 ? 0 : x - 1;
          const usize nextX = x + 1 == nX ? x : x + 1;
          const usize index = row + x;
          const double gx = 0.5 * (static_cast<double>(p0[row + previousX]) - static_cast<double>(p0[row + nextX])) * invSpX;
          const double gy = 0.5 * (static_cast<double>(p0[previousRow + x]) - static_cast<double>(p0[nextRow + x])) * invSpY;
          double magnitudeSquared = gx * gx + gy * gy;
          if(hasZ)
          {
            const double gz = 0.5 * (static_cast<double>(pm[index]) - static_cast<double>(pp[index])) * invSpZ;
            magnitudeSquared += gz * gz;
          }
          outPlane[index] = static_cast<float32>(std::sqrt(magnitudeSquared));
        }
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, nY);
    parallelAlgorithm.execute(calculateRows);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> r = outStore.copyFromBuffer(z * slice, nonstd::span<const float32>(outPlane.data(), slice)); r.invalid())
    {
      return r;
    }
    if(z + 1 < nZ)
    {
      std::swap(win[0], win[1]);
      std::swap(win[1], win[2]);
      if(z + 2 < nZ)
      {
        if(Result<> r = loadPlane(win[2], z + 2); r.invalid())
        {
          return r;
        }
      }
      else
      {
        std::copy(win[1].cbegin(), win[1].cend(), win[2].begin());
      }
    }
  }
  return {};
}
} // namespace nx::core::ImageProcessing
