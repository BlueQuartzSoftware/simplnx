#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <nonstd/span.hpp>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
inline constexpr usize k_AxisProjection2DTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_AxisProjection2DMetadataBytes = 64ULL * 1024ULL;
inline constexpr usize k_AxisProjectionExternalOutputValues = 65536;
inline constexpr usize k_AxisProjectionMedianMaxWorkers = 8;

inline usize AxisProjectionMedianWorkerCount(usize columns)
{
  const usize hardwareWorkers = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  return std::max<usize>(1, std::min({columns, k_AxisProjectionMedianMaxWorkers, hardwareWorkers}));
}

inline bool AxisProjectionCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool AxisProjectionCheckedAdd(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline bool AxisProjectionComputeResidentBytes(usize metadataBytes, std::initializer_list<std::pair<usize, usize>> allocations, usize& residentBytes)
{
  residentBytes = metadataBytes;
  for(const auto& [count, valueBytes] : allocations)
  {
    usize allocationBytes = 0;
    if(!AxisProjectionCheckedMultiply(count, valueBytes, allocationBytes) || !AxisProjectionCheckedAdd(residentBytes, allocationBytes, residentBytes))
    {
      return false;
    }
  }
  return true;
}

struct AxisProjectionShape
{
  usize dim_x = 0;
  usize dim_y = 0;
  usize dim_z = 0;
  usize plane_values = 0;
  usize xz_values = 0;
  usize volume = 0;
  usize projection_extent = 0;
  usize output_slots = 0;
  bool empty = false;
};

template <class TIn, class TOut>
Result<AxisProjectionShape> ValidateAxisProjection(const AbstractDataStore<TIn>& in, const AbstractDataStore<TOut>& out, const SizeVec3Type& dims, usize projDim)
{
  if(projDim > 2)
  {
    return MakeErrorResult<AxisProjectionShape>(-8330, fmt::format("AxisProjectionEngine: projDim must be 0, 1, or 2 (got {}).", projDim));
  }

  AxisProjectionShape shape;
  shape.dim_x = dims[0];
  shape.dim_y = dims[1];
  shape.dim_z = dims[2];
  if(shape.dim_x == 0 || shape.dim_y == 0 || shape.dim_z == 0)
  {
    shape.empty = true;
    return {shape};
  }

  const std::string formattedDims = string_utilities::FormatDimensions3D(dims);
  if(!AxisProjectionCheckedMultiply(shape.dim_x, shape.dim_y, shape.plane_values))
  {
    return MakeErrorResult<AxisProjectionShape>(-8345, fmt::format("Axis projection dimensions overflow usize while computing the XY plane size. Actual dimensions: {}.", formattedDims));
  }
  if(!AxisProjectionCheckedMultiply(shape.dim_x, shape.dim_z, shape.xz_values))
  {
    return MakeErrorResult<AxisProjectionShape>(-8347, fmt::format("Axis projection dimensions overflow usize while computing the XZ staging size. Actual dimensions: {}.", formattedDims));
  }
  if(!AxisProjectionCheckedMultiply(shape.plane_values, shape.dim_z, shape.volume))
  {
    return MakeErrorResult<AxisProjectionShape>(-8346, fmt::format("Axis projection dimensions overflow usize while computing the XYZ volume. Actual dimensions: {}.", formattedDims));
  }

  shape.projection_extent = dims[projDim];
  shape.output_slots = shape.volume / shape.projection_extent;
  if(in.getSize() != shape.volume)
  {
    return MakeErrorResult<AxisProjectionShape>(
        -8348, fmt::format("Axis projection input store size ({}) does not match the expected XYZ volume ({}) for dimensions {}.", in.getSize(), shape.volume, formattedDims));
  }
  if(out.getSize() != shape.output_slots)
  {
    return MakeErrorResult<AxisProjectionShape>(-8349, fmt::format("Axis projection output store size ({}) does not match the expected projected size ({}) for dimensions {} and projection axis {}.",
                                                                   out.getSize(), shape.output_slots, formattedDims, projDim));
  }
  return {shape};
}

enum class AxisProjection2DRoute : uint8
{
  AssociativeXRows,
  AssociativeXChunks,
  AssociativeYRows,
  AssociativeYTiles,
  SingletonZ,
  MedianXRows,
  MedianXExternal,
  MedianYTiles,
  MedianYExternal
};

struct AxisProjection2DPlan
{
  AxisProjection2DRoute route = AxisProjection2DRoute::SingletonZ;
  usize input_values = 0;
  usize output_values = 0;
  usize state_slots = 0;
  usize pencil_values = 0;
  usize block_rows = 0;
  usize tile_columns = 0;
  usize sort_batch_values = 0;
  usize resident_bytes = 0;
};

template <class T>
class AxisProjectionWorkStore
{
public:
  explicit AxisProjectionWorkStore(std::unique_ptr<SweepTemporaryStore<T>> temporaryStore)
  : m_TemporaryStore(std::move(temporaryStore))
  {
  }

  explicit AxisProjectionWorkStore(std::shared_ptr<AbstractDataStore<T>> dataStore)
  : m_DataStoreOwner(std::move(dataStore))
  {
  }

  usize getSize() const
  {
    if(m_TemporaryStore != nullptr)
    {
      return m_TemporaryStore->getSize();
    }
    return m_DataStoreOwner == nullptr ? 0 : m_DataStoreOwner->getSize();
  }

  std::optional<ShapeType> getChunkShape() const
  {
    if(m_TemporaryStore != nullptr)
    {
      return m_TemporaryStore->getChunkShape();
    }
    return m_DataStoreOwner == nullptr ? std::nullopt : m_DataStoreOwner->getChunkShape();
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<T> values) const
  {
    if(m_TemporaryStore != nullptr)
    {
      return m_TemporaryStore->copyIntoBuffer(offset, values);
    }
    if(m_DataStoreOwner != nullptr)
    {
      return m_DataStoreOwner->copyIntoBuffer(offset, values);
    }
    return MakeErrorResult(-8752, "Axis projection work-store read has no backing store.");
  }

  Result<> copyFromBuffer(usize offset, nonstd::span<const T> values)
  {
    if(m_TemporaryStore != nullptr)
    {
      return m_TemporaryStore->copyFromBuffer(offset, values);
    }
    if(m_DataStoreOwner != nullptr)
    {
      return m_DataStoreOwner->copyFromBuffer(offset, values);
    }
    return MakeErrorResult(-8752, "Axis projection work-store write has no backing store.");
  }

private:
  std::unique_ptr<SweepTemporaryStore<T>> m_TemporaryStore;
  std::shared_ptr<AbstractDataStore<T>> m_DataStoreOwner;
};

template <class T>
struct AxisProjection2DServices
{
  std::function<Result<std::unique_ptr<AxisProjectionWorkStore<T>>>(usize valueCount, usize maxBatchValues)> create_transpose_store;
  std::function<Result<std::unique_ptr<IExternalSort>>(const ExternalSortConfig&)> create_external_sort;
};

template <class ResultT>
std::string DescribeAxisProjectionProviderError(const Result<ResultT>& result)
{
  if(result.errors().empty())
  {
    return "provider returned an unspecified error";
  }
  const Error& error = result.errors().front();
  return fmt::format("{} (provider code {})", error.message, error.code);
}

template <class T>
int32 CompareAxisProjectionRecords(nonstd::span<const std::byte> leftBytes, nonstd::span<const std::byte> rightBytes)
{
  if(leftBytes.size() != sizeof(T) || rightBytes.size() != sizeof(T))
  {
    return 0;
  }
  T left{};
  T right{};
  std::memcpy(&left, leftBytes.data(), sizeof(T));
  std::memcpy(&right, rightBytes.data(), sizeof(T));
  if constexpr(std::is_floating_point_v<T>)
  {
    const bool leftIsNan = std::isnan(left);
    const bool rightIsNan = std::isnan(right);
    if(leftIsNan || rightIsNan)
    {
      if(leftIsNan && rightIsNan)
      {
        return 0;
      }
      return leftIsNan ? 1 : -1;
    }
  }
  if(left < right)
  {
    return -1;
  }
  if(right < left)
  {
    return 1;
  }
  return 0;
}

template <class TIn, class TOut>
Result<std::unique_ptr<AxisProjectionWorkStore<TIn>>> CreateAxisProjectionTransposeStore(const AbstractDataStore<TIn>& in, const AbstractDataStore<TOut>& out, const AxisProjectionShape& shape,
                                                                                         usize maxBatchValues, const std::atomic_bool& shouldCancel)
{
  if(data_store_utilities::GetIOCollection().hasTemporaryRecordStoreCapability())
  {
    auto temporaryResult = CreateSweepTemporaryStore<TIn>(shape.volume, std::max<usize>(1, maxBatchValues), shouldCancel, "Axis-projection Median Y transpose");
    if(temporaryResult.invalid())
    {
      return MakeErrorResult<std::unique_ptr<AxisProjectionWorkStore<TIn>>>(
          -8750, fmt::format("Axis projection could not create its {}-value typed transpose store: {}", shape.volume, DescribeAxisProjectionProviderError(temporaryResult)));
    }
    return {std::make_unique<AxisProjectionWorkStore<TIn>>(std::move(temporaryResult.value()))};
  }

  const AbstractDataStore<TIn>* typedEndpointPtr = in.getStoreType() == IDataStore::StoreType::OutOfCore ? &in : nullptr;
  const std::string dataFormat = typedEndpointPtr != nullptr ? typedEndpointPtr->getDataFormat() : out.getDataFormat();
  if(dataFormat.empty())
  {
    return MakeErrorResult<std::unique_ptr<AxisProjectionWorkStore<TIn>>>(
        -8750, fmt::format("Axis projection cannot create its {}-value typed transpose store because no out-of-core endpoint has a data format.", shape.volume));
  }
  try
  {
    auto dataStore = data_store_utilities::CreateDataStoreWithFormat<TIn>(dataFormat, ShapeType{1, shape.dim_x, shape.dim_y}, ShapeType{1});
    if(dataStore == nullptr || dataStore->getSize() != shape.volume || dataStore->getStoreType() != IDataStore::StoreType::OutOfCore)
    {
      return MakeErrorResult<std::unique_ptr<AxisProjectionWorkStore<TIn>>>(
          -8750, fmt::format("Axis projection data-format fallback '{}' did not create a {}-value out-of-core typed transpose store.", dataFormat, shape.volume));
    }
    return {std::make_unique<AxisProjectionWorkStore<TIn>>(std::move(dataStore))};
  } catch(const std::exception& exception)
  {
    return MakeErrorResult<std::unique_ptr<AxisProjectionWorkStore<TIn>>>(
        -8750, fmt::format("Axis projection failed to create its {}-value typed transpose store in format '{}': {}", shape.volume, dataFormat, exception.what()));
  }
}

template <class TIn, class TOut, class ReduceFn>
Result<AxisProjection2DPlan> CreateAxisProjection2DPlan(const SizeVec3Type& dims, usize projDim, usize targetBytes = k_AxisProjection2DTargetBytes)
{
  if(projDim > 2)
  {
    return MakeErrorResult<AxisProjection2DPlan>(-8740, fmt::format("Axis projection true-2-D planning requires projection axis 0, 1, or 2, but got {}.", projDim));
  }

  usize volume = 0;
  usize planeValues = 0;
  if(!AxisProjectionCheckedMultiply(dims[0], dims[1], planeValues) || !AxisProjectionCheckedMultiply(planeValues, dims[2], volume))
  {
    return MakeErrorResult<AxisProjection2DPlan>(-8741,
                                                 fmt::format("Axis projection true-2-D planning overflowed the addressable value count for dimensions {} x {} x {}.", dims[0], dims[1], dims[2]));
  }

  AxisProjection2DPlan plan;
  if(volume == 0)
  {
    return {plan};
  }
  if(dims[2] != 1)
  {
    return MakeErrorResult<AxisProjection2DPlan>(-8740, fmt::format("Axis projection true-2-D planning requires Z=1, but dimensions are {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(targetBytes <= k_AxisProjection2DMetadataBytes)
  {
    return MakeErrorResult<AxisProjection2DPlan>(-8742,
                                                 fmt::format("Axis projection true-2-D target ({} bytes) must exceed its {}-byte metadata reserve.", targetBytes, k_AxisProjection2DMetadataBytes));
  }

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize availableBytes = targetBytes - k_AxisProjection2DMetadataBytes;

  const auto finalizeResidentBytes = [&](std::initializer_list<std::pair<usize, usize>> allocations) -> Result<AxisProjection2DPlan> {
    if(!AxisProjectionComputeResidentBytes(k_AxisProjection2DMetadataBytes, allocations, plan.resident_bytes) || plan.resident_bytes > targetBytes)
    {
      return MakeErrorResult<AxisProjection2DPlan>(
          -8743, fmt::format("Axis projection true-2-D plan overflowed or exceeded its {}-byte target for dimensions {} x {} and projection axis {}.", targetBytes, dimX, dimY, projDim));
    }
    return {plan};
  };

  if(projDim == 2)
  {
    usize bytesPerValue = 0;
    if(!AxisProjectionCheckedAdd(sizeof(TIn), sizeof(TOut), bytesPerValue))
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection singleton-Z value-size planning overflowed.");
    }
    plan.input_values = std::min(volume, availableBytes / bytesPerValue);
    if(plan.input_values == 0)
    {
      return MakeErrorResult<AxisProjection2DPlan>(
          -8742, fmt::format("Axis projection singleton-Z target ({} bytes) cannot hold one {}-byte input and one {}-byte output value.", targetBytes, sizeof(TIn), sizeof(TOut)));
    }
    plan.output_values = plan.input_values;
    plan.route = AxisProjection2DRoute::SingletonZ;
    return finalizeResidentBytes({{plan.input_values, sizeof(TIn)}, {plan.output_values, sizeof(TOut)}});
  }

  if constexpr(ReduceFn::k_IsAssociative)
  {
    using StateType = typename ReduceFn::template State<TOut>;
    if(projDim == 0)
    {
      usize rowInputBytes = 0;
      usize rowBytes = 0;
      if(!AxisProjectionCheckedMultiply(dimX, sizeof(TIn), rowInputBytes) || !AxisProjectionCheckedAdd(rowInputBytes, sizeof(StateType), rowBytes) ||
         !AxisProjectionCheckedAdd(rowBytes, sizeof(TOut), rowBytes))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, fmt::format("Axis projection X-row planning overflowed for width {} and {}-byte input values.", dimX, sizeof(TIn)));
      }
      const usize rowCount = rowBytes == 0 ? 0 : std::min(dimY, availableBytes / rowBytes);
      if(rowCount > 1 || (rowCount == 1 && dimY == 1))
      {
        plan.route = AxisProjection2DRoute::AssociativeXRows;
        plan.block_rows = rowCount;
        plan.tile_columns = dimX;
        plan.state_slots = rowCount;
        plan.output_values = rowCount;
        if(!AxisProjectionCheckedMultiply(rowCount, dimX, plan.input_values))
        {
          return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection X-row input-count planning overflowed.");
        }
        return finalizeResidentBytes({{plan.input_values, sizeof(TIn)}, {plan.state_slots, sizeof(StateType)}, {plan.output_values, sizeof(TOut)}});
      }

      if(availableBytes <= sizeof(StateType) + sizeof(TIn))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection X-chunk target ({} bytes) cannot hold one reducer state and one output value.", targetBytes));
      }
      const usize maximumOutputValues = (availableBytes - sizeof(StateType) - sizeof(TIn)) / sizeof(TOut);
      plan.output_values = std::min({dimY, k_AxisProjectionExternalOutputValues, maximumOutputValues});
      if(plan.output_values == 0)
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection X-chunk target ({} bytes) cannot hold one output value.", targetBytes));
      }
      usize outputBytes = 0;
      usize fixedBytes = 0;
      if(!AxisProjectionCheckedMultiply(plan.output_values, sizeof(TOut), outputBytes) || !AxisProjectionCheckedAdd(sizeof(StateType), outputBytes, fixedBytes) || fixedBytes >= availableBytes)
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection X-chunk output-batch planning overflowed.");
      }
      plan.input_values = std::min(dimX, (availableBytes - fixedBytes) / sizeof(TIn));
      if(plan.input_values == 0)
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection X-chunk target ({} bytes) cannot hold one input value.", targetBytes));
      }
      plan.route = AxisProjection2DRoute::AssociativeXChunks;
      plan.block_rows = 1;
      plan.tile_columns = plan.input_values;
      plan.state_slots = 1;
      return finalizeResidentBytes({{plan.input_values, sizeof(TIn)}, {1, sizeof(StateType)}, {plan.output_values, sizeof(TOut)}});
    }

    usize stateAndOutputBytesPerColumn = 0;
    usize fixedColumnBytes = 0;
    usize rowInputBytes = 0;
    if(!AxisProjectionCheckedAdd(sizeof(StateType), sizeof(TOut), stateAndOutputBytesPerColumn) || !AxisProjectionCheckedMultiply(dimX, stateAndOutputBytesPerColumn, fixedColumnBytes) ||
       !AxisProjectionCheckedMultiply(dimX, sizeof(TIn), rowInputBytes))
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8743, fmt::format("Axis projection Y-row planning overflowed for width {}.", dimX));
    }
    usize minimumRowBytes = 0;
    if(AxisProjectionCheckedAdd(fixedColumnBytes, rowInputBytes, minimumRowBytes) && minimumRowBytes <= availableBytes)
    {
      const usize rowCount = std::min(dimY, (availableBytes - fixedColumnBytes) / rowInputBytes);
      plan.route = AxisProjection2DRoute::AssociativeYRows;
      plan.block_rows = rowCount;
      plan.tile_columns = dimX;
      plan.state_slots = dimX;
      plan.output_values = dimX;
      if(!AxisProjectionCheckedMultiply(rowCount, dimX, plan.input_values))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection Y-row input-count planning overflowed.");
      }
      return finalizeResidentBytes({{plan.input_values, sizeof(TIn)}, {plan.state_slots, sizeof(StateType)}, {plan.output_values, sizeof(TOut)}});
    }

    usize tiledBytesPerColumn = 0;
    if(!AxisProjectionCheckedAdd(stateAndOutputBytesPerColumn, sizeof(TIn), tiledBytesPerColumn))
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection Y-tile per-column planning overflowed.");
    }
    plan.tile_columns = std::min(dimX, availableBytes / tiledBytesPerColumn);
    if(plan.tile_columns == 0)
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection Y-tile target ({} bytes) cannot hold one input value, reducer state, and output value.", targetBytes));
    }
    plan.route = AxisProjection2DRoute::AssociativeYTiles;
    plan.block_rows = 1;
    plan.input_values = plan.tile_columns;
    plan.state_slots = plan.tile_columns;
    plan.output_values = plan.tile_columns;
    return finalizeResidentBytes({{plan.input_values, sizeof(TIn)}, {plan.state_slots, sizeof(StateType)}, {plan.output_values, sizeof(TOut)}});
  }
  else
  {
    if(projDim == 0)
    {
      usize pencilBytes = 0;
      usize rowBytes = 0;
      if(!AxisProjectionCheckedMultiply(dimX, sizeof(TIn), pencilBytes) || !AxisProjectionCheckedAdd(pencilBytes, sizeof(TOut), rowBytes))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, fmt::format("Axis projection Median X-row planning overflowed for width {}.", dimX));
      }
      const usize rowCount = std::min(dimY, availableBytes / rowBytes);
      if(rowCount > 0)
      {
        plan.route = AxisProjection2DRoute::MedianXRows;
        plan.block_rows = rowCount;
        plan.tile_columns = dimX;
        plan.output_values = rowCount;
        if(!AxisProjectionCheckedMultiply(rowCount, dimX, plan.pencil_values))
        {
          return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection Median X-row pencil-count planning overflowed.");
        }
        plan.input_values = plan.pencil_values;
        return finalizeResidentBytes({{plan.pencil_values, sizeof(TIn)}, {plan.output_values, sizeof(TOut)}});
      }

      plan.route = AxisProjection2DRoute::MedianXExternal;
    }
    else
    {
      usize pencilBytes = 0;
      usize bytesPerColumn = 0;
      if(!AxisProjectionCheckedMultiply(dimY, sizeof(TIn), pencilBytes) || !AxisProjectionCheckedAdd(pencilBytes, sizeof(TOut), bytesPerColumn))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, fmt::format("Axis projection Median Y-tile planning overflowed for height {}.", dimY));
      }
      if(bytesPerColumn <= availableBytes)
      {
        plan.route = AxisProjection2DRoute::MedianYTiles;
        plan.block_rows = dimY;
        usize workerCount = AxisProjectionMedianWorkerCount(dimX);
        for(;;)
        {
          usize workerScratchValues = 0;
          usize workerScratchBytes = 0;
          if(!AxisProjectionCheckedMultiply(workerCount, dimY, workerScratchValues) || !AxisProjectionCheckedMultiply(workerScratchValues, sizeof(TIn), workerScratchBytes))
          {
            return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection Median Y worker-scratch planning overflowed.");
          }
          plan.tile_columns = workerScratchBytes < availableBytes ? std::min(dimX, (availableBytes - workerScratchBytes) / bytesPerColumn) : 0;
          if(plan.tile_columns > 0)
          {
            plan.state_slots = workerCount;
            plan.pencil_values = workerScratchValues;
            break;
          }
          if(workerCount == 1)
          {
            // One complete pencil fits, but a separate worker pencil does not. Read the one-column tile directly
            // and run nth_element in that returned buffer without a second allocation.
            plan.tile_columns = 1;
            plan.state_slots = 0;
            plan.pencil_values = 0;
            break;
          }
          --workerCount;
        }
        plan.output_values = plan.tile_columns;
        if(!AxisProjectionCheckedMultiply(plan.tile_columns, dimY, plan.input_values))
        {
          return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection Median Y-tile input-count planning overflowed.");
        }
        return finalizeResidentBytes({{plan.input_values, sizeof(TIn)}, {plan.pencil_values, sizeof(TIn)}, {plan.output_values, sizeof(TOut)}});
      }
      plan.route = AxisProjection2DRoute::MedianYExternal;
    }

    const usize outputSlots = projDim == 0 ? dimY : dimX;
    plan.output_values = std::min(outputSlots, k_AxisProjectionExternalOutputValues);
    usize outputBytes = 0;
    if(!AxisProjectionCheckedMultiply(plan.output_values, sizeof(TOut), outputBytes) || outputBytes >= availableBytes)
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection external Median target ({} bytes) cannot hold its bounded output batch.", targetBytes));
    }
    usize sortBytesPerValue = 0;
    if(!AxisProjectionCheckedAdd(sizeof(TIn), sizeof(uint64), sortBytesPerValue))
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection external Median sort-record planning overflowed.");
    }
    plan.sort_batch_values = (availableBytes - outputBytes) / sortBytesPerValue;
    if(plan.sort_batch_values == 0)
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection external Median target ({} bytes) cannot hold one value and one sort-order entry.", targetBytes));
    }
    plan.input_values = plan.sort_batch_values;

    usize sortPeakBytes = 0;
    if(!AxisProjectionComputeResidentBytes(0, {{plan.sort_batch_values, sortBytesPerValue}, {plan.output_values, sizeof(TOut)}}, sortPeakBytes))
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection external Median sort-peak planning overflowed.");
    }
    usize peakBytes = sortPeakBytes;

    if(plan.route == AxisProjection2DRoute::MedianYExternal)
    {
      const usize transposeCapacity = availableBytes / sizeof(TIn);
      if(transposeCapacity < 2)
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8742, fmt::format("Axis projection external Median Y target ({} bytes) cannot hold a transpose value and row transfer.", targetBytes));
      }
      plan.block_rows = std::min({dimY, static_cast<usize>(256), transposeCapacity / 2});
      plan.tile_columns = std::min(dimX, transposeCapacity / (plan.block_rows + 1));
      if(plan.tile_columns == 0)
      {
        plan.block_rows = 1;
        plan.tile_columns = std::min(dimX, transposeCapacity / 2);
      }
      if(plan.tile_columns == 0 || !AxisProjectionCheckedMultiply(plan.tile_columns, plan.block_rows, plan.pencil_values))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection external Median Y transpose planning overflowed.");
      }
      usize transposeValues = 0;
      if(!AxisProjectionCheckedAdd(plan.pencil_values, plan.tile_columns, transposeValues))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection external Median Y transpose-buffer planning overflowed.");
      }
      usize transposeBytes = 0;
      if(!AxisProjectionCheckedMultiply(transposeValues, sizeof(TIn), transposeBytes))
      {
        return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection external Median Y transpose-byte planning overflowed.");
      }
      peakBytes = std::max(peakBytes, transposeBytes);
      plan.input_values = std::max(plan.input_values, plan.tile_columns);
    }

    if(!AxisProjectionCheckedAdd(k_AxisProjection2DMetadataBytes, peakBytes, plan.resident_bytes) || plan.resident_bytes > targetBytes)
    {
      return MakeErrorResult<AxisProjection2DPlan>(-8743, "Axis projection external Median peak-memory planning exceeded its target.");
    }
    return {plan};
  }
}

template <class TIn, class TOut, class ReduceFn>
Result<> ApplyAssociativeProjection2DX(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                                       const AxisProjection2DPlan& plan)
{
  using StateType = typename ReduceFn::template State<TOut>;
  std::vector<TIn> inputBuffer(plan.input_values);
  std::vector<TOut> outputBuffer(plan.output_values);
  std::vector<StateType> states(plan.state_slots);

  if(plan.route == AxisProjection2DRoute::AssociativeXRows)
  {
    for(usize rowStart = 0; rowStart < shape.dim_y; rowStart += plan.block_rows)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(plan.block_rows, shape.dim_y - rowStart);
      const usize inputCount = rowCount * shape.dim_x;
      Result<> readResult = in.copyIntoBuffer(rowStart * shape.dim_x, nonstd::span<TIn>(inputBuffer.data(), inputCount));
      if(readResult.invalid())
      {
        return readResult;
      }
      if(shouldCancel)
      {
        return {};
      }
      std::fill(states.begin(), states.begin() + rowCount, StateType{});

      auto reduceRows = [&](const Range& range) {
        for(usize localRow = range.min(); localRow < range.max(); ++localRow)
        {
          if(shouldCancel)
          {
            return;
          }
          StateType& state = states[localRow];
          const usize rowOffset = localRow * shape.dim_x;
          for(usize x = 0; x < shape.dim_x; ++x)
          {
            reduce.accumulate(state, inputBuffer[rowOffset + x]);
          }
          outputBuffer[localRow] = reduce.finalize(state);
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, rowCount);
      parallelAlgorithm.execute(reduceRows);
      if(shouldCancel)
      {
        return {};
      }
      Result<> writeResult = out.copyFromBuffer(rowStart, nonstd::span<const TOut>(outputBuffer.data(), rowCount));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    return {};
  }

  usize outputStart = 0;
  usize outputCount = 0;
  for(usize row = 0; row < shape.dim_y; ++row)
  {
    if(shouldCancel)
    {
      return {};
    }
    StateType state{};
    for(usize xStart = 0; xStart < shape.dim_x; xStart += plan.input_values)
    {
      const usize inputCount = std::min(plan.input_values, shape.dim_x - xStart);
      Result<> readResult = in.copyIntoBuffer(row * shape.dim_x + xStart, nonstd::span<TIn>(inputBuffer.data(), inputCount));
      if(readResult.invalid())
      {
        return readResult;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize index = 0; index < inputCount; ++index)
      {
        reduce.accumulate(state, inputBuffer[index]);
      }
    }
    outputBuffer[outputCount++] = reduce.finalize(state);
    if(outputCount == plan.output_values || row + 1 == shape.dim_y)
    {
      if(shouldCancel)
      {
        return {};
      }
      Result<> writeResult = out.copyFromBuffer(outputStart, nonstd::span<const TOut>(outputBuffer.data(), outputCount));
      if(writeResult.invalid())
      {
        return writeResult;
      }
      outputStart += outputCount;
      outputCount = 0;
    }
  }
  return {};
}

template <class TIn, class TOut, class ReduceFn>
Result<> ApplyAssociativeProjection2DY(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                                       const AxisProjection2DPlan& plan)
{
  using StateType = typename ReduceFn::template State<TOut>;
  std::vector<TIn> inputBuffer(plan.input_values);
  std::vector<TOut> outputBuffer(plan.output_values);
  std::vector<StateType> states(plan.state_slots);

  if(plan.route == AxisProjection2DRoute::AssociativeYRows)
  {
    for(usize yStart = 0; yStart < shape.dim_y; yStart += plan.block_rows)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(plan.block_rows, shape.dim_y - yStart);
      const usize inputCount = rowCount * shape.dim_x;
      Result<> readResult = in.copyIntoBuffer(yStart * shape.dim_x, nonstd::span<TIn>(inputBuffer.data(), inputCount));
      if(readResult.invalid())
      {
        return readResult;
      }
      if(shouldCancel)
      {
        return {};
      }

      auto accumulateColumns = [&](const Range& range) {
        for(usize x = range.min(); x < range.max(); ++x)
        {
          if(shouldCancel)
          {
            return;
          }
          StateType& state = states[x];
          for(usize localY = 0; localY < rowCount; ++localY)
          {
            reduce.accumulate(state, inputBuffer[localY * shape.dim_x + x]);
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, shape.dim_x);
      parallelAlgorithm.execute(accumulateColumns);
      if(shouldCancel)
      {
        return {};
      }
    }

    auto finalizeColumns = [&](const Range& range) {
      for(usize x = range.min(); x < range.max(); ++x)
      {
        outputBuffer[x] = reduce.finalize(states[x]);
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, shape.dim_x);
    parallelAlgorithm.execute(finalizeColumns);
    if(shouldCancel)
    {
      return {};
    }
    return out.copyFromBuffer(0, nonstd::span<const TOut>(outputBuffer.data(), shape.dim_x));
  }

  for(usize xStart = 0; xStart < shape.dim_x; xStart += plan.tile_columns)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize tileColumns = std::min(plan.tile_columns, shape.dim_x - xStart);
    std::fill(states.begin(), states.begin() + tileColumns, StateType{});
    for(usize y = 0; y < shape.dim_y; ++y)
    {
      Result<> readResult = in.copyIntoBuffer(y * shape.dim_x + xStart, nonstd::span<TIn>(inputBuffer.data(), tileColumns));
      if(readResult.invalid())
      {
        return readResult;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize localX = 0; localX < tileColumns; ++localX)
      {
        reduce.accumulate(states[localX], inputBuffer[localX]);
      }
    }

    auto finalizeTile = [&](const Range& range) {
      for(usize localX = range.min(); localX < range.max(); ++localX)
      {
        outputBuffer[localX] = reduce.finalize(states[localX]);
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, tileColumns);
    parallelAlgorithm.execute(finalizeTile);
    if(shouldCancel)
    {
      return {};
    }
    Result<> writeResult = out.copyFromBuffer(xStart, nonstd::span<const TOut>(outputBuffer.data(), tileColumns));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

template <class TIn, class TOut, class ReduceFn>
Result<> ApplyMedianProjection2DXRows(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                                      const AxisProjection2DPlan& plan)
{
  std::vector<TIn> pencilBuffer(plan.pencil_values);
  std::vector<TOut> outputBuffer(plan.output_values);
  for(usize rowStart = 0; rowStart < shape.dim_y; rowStart += plan.block_rows)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize rowCount = std::min(plan.block_rows, shape.dim_y - rowStart);
    const usize inputCount = rowCount * shape.dim_x;
    Result<> readResult = in.copyIntoBuffer(rowStart * shape.dim_x, nonstd::span<TIn>(pencilBuffer.data(), inputCount));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(shouldCancel)
    {
      return {};
    }

    auto reduceRows = [&](const Range& range) {
      for(usize localRow = range.min(); localRow < range.max(); ++localRow)
      {
        if(shouldCancel)
        {
          return;
        }
        outputBuffer[localRow] = reduce.template reducePencil<TIn, TOut>(nonstd::span<TIn>(pencilBuffer.data() + localRow * shape.dim_x, shape.dim_x));
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, rowCount);
    parallelAlgorithm.execute(reduceRows);
    if(shouldCancel)
    {
      return {};
    }
    Result<> writeResult = out.copyFromBuffer(rowStart, nonstd::span<const TOut>(outputBuffer.data(), rowCount));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

template <class TIn, class TOut, class ReduceFn>
Result<> ApplyMedianProjection2DYTiles(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                                       const AxisProjection2DPlan& plan)
{
  std::vector<TOut> outputBuffer(plan.output_values);
  for(usize xStart = 0; xStart < shape.dim_x; xStart += plan.tile_columns)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize tileColumns = std::min(plan.tile_columns, shape.dim_x - xStart);
    const Extent tileExtent(std::vector<uint64>{0, 0, static_cast<uint64>(xStart)}, std::vector<uint64>{0, static_cast<uint64>(shape.dim_y - 1), static_cast<uint64>(xStart + tileColumns - 1)});
    Result<std::vector<TIn>> tileResult = in.readExtent(tileExtent);
    if(tileResult.invalid())
    {
      return MakeErrorResult(-8753, fmt::format("Axis projection Median Y extent read failed for X columns [{}, {}] in dimensions {}: {}", xStart, xStart + tileColumns - 1,
                                                string_utilities::FormatDimensions3D(SizeVec3Type{shape.dim_x, shape.dim_y, shape.dim_z}), tileResult.errors().front().message));
    }
    std::vector<TIn> rowMajorTile = std::move(tileResult.value());
    if(shouldCancel)
    {
      return {};
    }
    const usize tileValues = tileColumns * shape.dim_y;
    if(rowMajorTile.size() != tileValues)
    {
      return MakeErrorResult(-8753,
                             fmt::format("Axis projection Median Y extent read returned {} of {} values for X columns [{}, {}].", rowMajorTile.size(), tileValues, xStart, xStart + tileColumns - 1));
    }
    if(plan.state_slots == 0)
    {
      outputBuffer[0] = reduce.template reducePencil<TIn, TOut>(nonstd::span<TIn>(rowMajorTile.data(), rowMajorTile.size()));
    }
    else
    {
      const auto reduceColumn = [&](usize localX, std::vector<TIn>& pencil) {
        for(usize y = 0; y < shape.dim_y; ++y)
        {
          pencil[y] = rowMajorTile[y * tileColumns + localX];
        }
        outputBuffer[localX] = reduce.template reducePencil<TIn, TOut>(nonstd::span<TIn>(pencil.data(), pencil.size()));
      };

      bool parallelized = false;
#ifdef SIMPLNX_ENABLE_MULTICORE
      ParallelDataAlgorithm parallelControl;
      if(parallelControl.getParallelizationEnabled() && plan.state_slots > 1)
      {
        tbb::task_arena arena(static_cast<int>(plan.state_slots));
        tbb::enumerable_thread_specific<std::vector<TIn>> workerPencils([&]() { return std::vector<TIn>(shape.dim_y); });
        arena.execute([&]() {
          tbb::parallel_for(
              tbb::blocked_range<usize>(0, tileColumns),
              [&](const tbb::blocked_range<usize>& range) {
                std::vector<TIn>& pencil = workerPencils.local();
                for(usize localX = range.begin(); localX < range.end(); ++localX)
                {
                  if(shouldCancel)
                  {
                    return;
                  }
                  reduceColumn(localX, pencil);
                }
              },
              tbb::auto_partitioner{});
        });
        parallelized = true;
      }
#endif
      if(!parallelized)
      {
        std::vector<TIn> pencil(shape.dim_y);
        for(usize localX = 0; localX < tileColumns; ++localX)
        {
          if(shouldCancel)
          {
            return {};
          }
          reduceColumn(localX, pencil);
        }
      }
    }
    if(shouldCancel)
    {
      return {};
    }
    Result<> writeResult = out.copyFromBuffer(xStart, nonstd::span<const TOut>(outputBuffer.data(), tileColumns));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

template <class TIn, class TOut, class ReduceFn>
Result<> ApplyProjection2DSingletonZ(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                                     const AxisProjection2DPlan& plan)
{
  std::vector<TIn> inputBuffer(plan.input_values);
  std::vector<TOut> outputBuffer(plan.output_values);
  for(usize start = 0; start < shape.volume; start += plan.input_values)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(plan.input_values, shape.volume - start);
    Result<> readResult = in.copyIntoBuffer(start, nonstd::span<TIn>(inputBuffer.data(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(shouldCancel)
    {
      return {};
    }
    for(usize index = 0; index < count; ++index)
    {
      if constexpr(ReduceFn::k_IsAssociative)
      {
        typename ReduceFn::template State<TOut> state{};
        reduce.accumulate(state, inputBuffer[index]);
        outputBuffer[index] = reduce.finalize(state);
      }
      else
      {
        outputBuffer[index] = reduce.template reducePencil<TIn, TOut>(nonstd::span<TIn>(inputBuffer.data() + index, 1));
      }
    }
    if(shouldCancel)
    {
      return {};
    }
    Result<> writeResult = out.copyFromBuffer(start, nonstd::span<const TOut>(outputBuffer.data(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

template <class TIn, class TOut, class StoreT>
Result<TOut> ReduceExternalMedianPencil(const StoreT& store, usize valueOffset, usize valueCount, const AxisProjection2DPlan& plan, const AxisProjection2DServices<TIn>& services,
                                        const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return {TOut{}};
  }
  if(!services.create_external_sort)
  {
    return MakeErrorResult<TOut>(-8746, fmt::format("Axis projection cannot reduce a {}-value Median pencil because no external-sort factory is available.", valueCount));
  }

  ExternalSortConfig config;
  config.record_size = sizeof(TIn);
  config.max_records_per_batch = plan.sort_batch_values;
  config.compare = CompareAxisProjectionRecords<TIn>;
  auto sortResult = services.create_external_sort(config);
  if(sortResult.invalid())
  {
    return MakeErrorResult<TOut>(-8746, fmt::format("Axis projection failed to create external sorting for a {}-value Median pencil: {}", valueCount, DescribeAxisProjectionProviderError(sortResult)));
  }
  std::unique_ptr<IExternalSort> sorter = std::move(sortResult.value());
  if(sorter == nullptr)
  {
    return MakeErrorResult<TOut>(-8746, fmt::format("Axis projection external-sort factory returned null for a {}-value Median pencil.", valueCount));
  }

  std::vector<TIn> inputBuffer(plan.sort_batch_values);
  for(usize localOffset = 0; localOffset < valueCount; localOffset += plan.sort_batch_values)
  {
    if(shouldCancel)
    {
      return {TOut{}};
    }
    const usize count = std::min(plan.sort_batch_values, valueCount - localOffset);
    Result<> readResult = store.copyIntoBuffer(valueOffset + localOffset, nonstd::span<TIn>(inputBuffer.data(), count));
    if(readResult.invalid())
    {
      return {nonstd::make_unexpected(std::move(readResult.errors()))};
    }
    if(shouldCancel)
    {
      return {TOut{}};
    }
    const auto* bytesPtr = reinterpret_cast<const std::byte*>(inputBuffer.data());
    Result<> appendResult = sorter->append(static_cast<uint64>(count), nonstd::span<const std::byte>(bytesPtr, count * sizeof(TIn)), shouldCancel, ExternalSortProgressCallbackType{});
    if(appendResult.invalid())
    {
      return MakeErrorResult<TOut>(
          -8747, fmt::format("Axis projection external Median append failed at pencil offset {} with {} values: {}", localOffset, count, DescribeAxisProjectionProviderError(appendResult)));
    }
  }

  Result<> finishResult = sorter->finish(shouldCancel, ExternalSortProgressCallbackType{});
  if(finishResult.invalid())
  {
    return MakeErrorResult<TOut>(-8748, fmt::format("Axis projection external Median finish failed for a {}-value pencil: {}", valueCount, DescribeAxisProjectionProviderError(finishResult)));
  }
  if(sorter->recordCount() != valueCount)
  {
    return MakeErrorResult<TOut>(-8749, fmt::format("Axis projection external Median sorter retained {} of {} pencil values.", sorter->recordCount(), valueCount));
  }

  std::array<std::byte, sizeof(TIn)> medianBytes{};
  auto readResult = sorter->read(static_cast<uint64>(valueCount / 2), 1, nonstd::span<std::byte>(medianBytes.data(), medianBytes.size()), shouldCancel);
  if(readResult.invalid())
  {
    return MakeErrorResult<TOut>(-8749, fmt::format("Axis projection external Median read failed at sorted index {}: {}", valueCount / 2, DescribeAxisProjectionProviderError(readResult)));
  }
  if(readResult.value() != 1)
  {
    return MakeErrorResult<TOut>(-8749, fmt::format("Axis projection external Median read returned {} of 1 value at sorted index {}.", readResult.value(), valueCount / 2));
  }
  TIn median{};
  std::memcpy(&median, medianBytes.data(), sizeof(TIn));
  return {static_cast<TOut>(median)};
}

template <class TIn, class TOut>
Result<> ApplyMedianProjection2DXExternal(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, const std::atomic_bool& shouldCancel,
                                          const AxisProjection2DPlan& plan, const AxisProjection2DServices<TIn>& services)
{
  std::vector<TOut> outputBuffer(plan.output_values);
  usize outputStart = 0;
  usize outputCount = 0;
  for(usize row = 0; row < shape.dim_y; ++row)
  {
    if(shouldCancel)
    {
      return {};
    }
    auto medianResult = ReduceExternalMedianPencil<TIn, TOut>(in, row * shape.dim_x, shape.dim_x, plan, services, shouldCancel);
    if(medianResult.invalid())
    {
      return ConvertResult(std::move(medianResult));
    }
    if(shouldCancel)
    {
      return {};
    }
    outputBuffer[outputCount++] = medianResult.value();
    if(outputCount == plan.output_values || row + 1 == shape.dim_y)
    {
      Result<> writeResult = out.copyFromBuffer(outputStart, nonstd::span<const TOut>(outputBuffer.data(), outputCount));
      if(writeResult.invalid())
      {
        return writeResult;
      }
      outputStart += outputCount;
      outputCount = 0;
    }
  }
  return {};
}

template <class TIn, class TOut>
Result<> ApplyMedianProjection2DYExternal(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, const std::atomic_bool& shouldCancel,
                                          const AxisProjection2DPlan& plan, const AxisProjection2DServices<TIn>& services)
{
  if(!services.create_transpose_store)
  {
    return MakeErrorResult(-8750, fmt::format("Axis projection cannot transpose {} values for external Median Y because no typed work-store factory is available.", shape.volume));
  }
  auto storeResult = services.create_transpose_store(shape.volume, std::max<usize>(1, plan.input_values));
  if(storeResult.invalid())
  {
    return MakeErrorResult(-8750, fmt::format("Axis projection failed to create its {}-value typed Median Y transpose store: {}", shape.volume, DescribeAxisProjectionProviderError(storeResult)));
  }
  std::unique_ptr<AxisProjectionWorkStore<TIn>> transposeStore = std::move(storeResult.value());
  if(transposeStore == nullptr || transposeStore->getSize() != shape.volume)
  {
    return MakeErrorResult(
        -8750, fmt::format("Axis projection Median Y transpose factory returned a store with {} values; expected {}.", transposeStore == nullptr ? 0 : transposeStore->getSize(), shape.volume));
  }

  {
    std::vector<TIn> rowBuffer(plan.tile_columns);
    std::vector<TIn> transposeBuffer(plan.pencil_values);
    for(usize xStart = 0; xStart < shape.dim_x; xStart += plan.tile_columns)
    {
      const usize tileColumns = std::min(plan.tile_columns, shape.dim_x - xStart);
      for(usize yStart = 0; yStart < shape.dim_y; yStart += plan.block_rows)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize rowCount = std::min(plan.block_rows, shape.dim_y - yStart);
        for(usize localY = 0; localY < rowCount; ++localY)
        {
          Result<> readResult = in.copyIntoBuffer((yStart + localY) * shape.dim_x + xStart, nonstd::span<TIn>(rowBuffer.data(), tileColumns));
          if(readResult.invalid())
          {
            return readResult;
          }
          if(shouldCancel)
          {
            return {};
          }
          for(usize localX = 0; localX < tileColumns; ++localX)
          {
            transposeBuffer[localX * rowCount + localY] = rowBuffer[localX];
          }
        }
        for(usize localX = 0; localX < tileColumns; ++localX)
        {
          Result<> writeResult = transposeStore->copyFromBuffer((xStart + localX) * shape.dim_y + yStart, nonstd::span<const TIn>(transposeBuffer.data() + localX * rowCount, rowCount));
          if(writeResult.invalid())
          {
            return writeResult;
          }
          if(shouldCancel)
          {
            return {};
          }
        }
      }
    }
  }

  std::vector<TOut> outputBuffer(plan.output_values);
  usize outputStart = 0;
  usize outputCount = 0;
  for(usize x = 0; x < shape.dim_x; ++x)
  {
    if(shouldCancel)
    {
      return {};
    }
    auto medianResult = ReduceExternalMedianPencil<TIn, TOut>(*transposeStore, x * shape.dim_y, shape.dim_y, plan, services, shouldCancel);
    if(medianResult.invalid())
    {
      return ConvertResult(std::move(medianResult));
    }
    if(shouldCancel)
    {
      return {};
    }
    outputBuffer[outputCount++] = medianResult.value();
    if(outputCount == plan.output_values || x + 1 == shape.dim_x)
    {
      Result<> writeResult = out.copyFromBuffer(outputStart, nonstd::span<const TOut>(outputBuffer.data(), outputCount));
      if(writeResult.invalid())
      {
        return writeResult;
      }
      outputStart += outputCount;
      outputCount = 0;
    }
  }
  return {};
}

template <class TIn, class TOut, class ReduceFn>
Result<> ApplyAxisProjection2D(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, usize projDim, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                               usize targetBytes = k_AxisProjection2DTargetBytes, const AxisProjection2DServices<TIn>* services = nullptr)
{
  if(shouldCancel)
  {
    return {};
  }
  const SizeVec3Type dims{shape.dim_x, shape.dim_y, shape.dim_z};
  auto planResult = CreateAxisProjection2DPlan<TIn, TOut, ReduceFn>(dims, projDim, targetBytes);
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const AxisProjection2DPlan& plan = planResult.value();
  AxisProjection2DServices<TIn> defaultServices;
  if(services == nullptr)
  {
    defaultServices.create_external_sort = [](const ExternalSortConfig& config) { return data_store_utilities::GetIOCollection().createExternalSort(config); };
    defaultServices.create_transpose_store = [&](usize, usize maxBatchValues) { return CreateAxisProjectionTransposeStore(in, out, shape, maxBatchValues, shouldCancel); };
    services = &defaultServices;
  }
  try
  {
    if(projDim == 2)
    {
      return ApplyProjection2DSingletonZ(in, out, shape, reduce, shouldCancel, plan);
    }
    if constexpr(ReduceFn::k_IsAssociative)
    {
      if(projDim == 0)
      {
        return ApplyAssociativeProjection2DX(in, out, shape, reduce, shouldCancel, plan);
      }
      return ApplyAssociativeProjection2DY(in, out, shape, reduce, shouldCancel, plan);
    }
    else
    {
      if(projDim == 0 && plan.route == AxisProjection2DRoute::MedianXRows)
      {
        return ApplyMedianProjection2DXRows(in, out, shape, reduce, shouldCancel, plan);
      }
      if(projDim == 1 && plan.route == AxisProjection2DRoute::MedianYTiles)
      {
        return ApplyMedianProjection2DYTiles(in, out, shape, reduce, shouldCancel, plan);
      }
      if(projDim == 0 && plan.route == AxisProjection2DRoute::MedianXExternal)
      {
        return ApplyMedianProjection2DXExternal(in, out, shape, shouldCancel, plan, *services);
      }
      if(projDim == 1 && plan.route == AxisProjection2DRoute::MedianYExternal)
      {
        return ApplyMedianProjection2DYExternal(in, out, shape, shouldCancel, plan, *services);
      }
      return MakeErrorResult(-8744, fmt::format("Axis projection bounded Median plan selected an unsupported route for projection axis {}.", projDim));
    }
  } catch(const std::exception& exception)
  {
    return MakeErrorResult(-8745,
                           fmt::format("Axis projection true-2-D execution failed for dimensions {} on projection axis {} while allocating its bounded {}-byte working set from a {}-byte target: {}",
                                       string_utilities::FormatDimensions3D(dims), projDim, plan.resident_bytes, targetBytes, exception.what()));
  }
}
} // namespace detail

namespace detail
{
/**
 * @brief Direct InCore projection path. The concrete DataStore pointer is acquired before parallel execution and
 * remains read-only. Workers write only to disjoint elements of a separate output vector. The final DataStore write
 * stays serial, so no worker calls a DataStore method.
 */
template <class TIn, class TOut, class ReduceFn>
Result<> ApplyAxisProjectionInMemory(const DataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, usize projDim, ReduceFn reduce, const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return {};
  }

  const TIn* const inputValuesPtr = in.data();
  std::vector<TOut> outputValues(shape.output_slots);

  auto executePencils = [&](auto inputBaseForSlot, usize inputStride) {
    auto reducePencils = [&](const Range& range) {
      std::vector<TIn> pencil;
      if constexpr(!ReduceFn::k_IsAssociative)
      {
        pencil.resize(shape.projection_extent);
      }

      for(usize slot = range.min(); slot < range.max(); ++slot)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize inputBase = inputBaseForSlot(slot);
        if constexpr(ReduceFn::k_IsAssociative)
        {
          typename ReduceFn::template State<TOut> state{};
          constexpr usize k_CancelCheckValues = 65536;
          for(usize blockStart = 0; blockStart < shape.projection_extent; blockStart += k_CancelCheckValues)
          {
            if(blockStart != 0 && shouldCancel)
            {
              return;
            }
            const usize blockEnd = blockStart + std::min(k_CancelCheckValues, shape.projection_extent - blockStart);
            for(usize projectionIndex = blockStart; projectionIndex < blockEnd; ++projectionIndex)
            {
              reduce.accumulate(state, inputValuesPtr[inputBase + projectionIndex * inputStride]);
            }
          }
          outputValues[slot] = reduce.finalize(state);
        }
        else
        {
          for(usize projectionIndex = 0; projectionIndex < shape.projection_extent; ++projectionIndex)
          {
            pencil[projectionIndex] = inputValuesPtr[inputBase + projectionIndex * inputStride];
          }
          outputValues[slot] = reduce.template reducePencil<TIn, TOut>(nonstd::span<TIn>(pencil.data(), pencil.size()));
        }
      }
    };

    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, shape.output_slots);
    parallelAlgorithm.execute(reducePencils);
  };

  if(projDim == 0)
  {
    executePencils([&](usize slot) { return slot * shape.dim_x; }, usize{1});
  }
  else if(projDim == 1)
  {
    executePencils(
        [&](usize slot) {
          const usize z = slot / shape.dim_x;
          const usize x = slot - z * shape.dim_x;
          return z * shape.plane_values + x;
        },
        shape.dim_x);
  }
  else
  {
    executePencils([](usize slot) { return slot; }, shape.plane_values);
  }

  if(shouldCancel)
  {
    return {};
  }
  return out.copyFromBuffer(0, nonstd::span<const TOut>(outputValues.data(), outputValues.size()));
}

/**
 * @brief Reduces a 3D scalar image along a single axis, producing an image whose projected axis has
 *        size 1. `projDim` selects the collapsed axis: 0 == X (dims[0], fastest-moving),
 *        1 == Y (dims[1]), 2 == Z (dims[2], slowest-moving). The simplnx flat index for input voxel
 *        (x,y,z) is `z*(Y*X) + y*X + x`. The output slot for that voxel is the same flat index
 *        recomputed in the collapsed dims with the projected coordinate forced to 0:
 *          - projDim==0 -> outDims {1,Y,Z}, slot = z*Y + y
 *          - projDim==1 -> outDims {X,1,Z}, slot = z*X + x
 *          - projDim==2 -> outDims {X,Y,1}, slot = y*X + x
 *        (derived generally below as slot = x*xStep + y*yStep + z*zStep).
 *
 *        The engine stages axis-aware slabs with serial bulk store reads, then reduces independent pencils in
 *        parallel using only local memory. Associative reducers fold each pencil into a local State; non-associative
 *        reducers such as Median receive the complete mutable pencil. Staging is capped near 16 MiB (except when one
 *        indivisible axis-specific staging unit exceeds that target), and output is written with serial bulk store
 *        writes. No parallel worker accesses a DataStore.
 *
 * @pre scalar array (1 component); `in` has dims.X*dims.Y*dims.Z values; `out` has
 *      dims.X*dims.Y*dims.Z/dims[projDim] values; projDim in {0,1,2} and dims[projDim] > 0.
 *
 * @tparam TIn      input element type
 * @tparam TOut     output element type
 * @tparam ReduceFn reduce functor satisfying the ProjectionReducers.hpp contract
 */
template <class TIn, class TOut, class ReduceFn>
Result<> ApplyAxisProjectionResident(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const AxisProjectionShape& shape, usize projDim, ReduceFn reduce,
                                     const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const usize dimX = shape.dim_x;
  const usize dimY = shape.dim_y;
  const usize dimZ = shape.dim_z;
  const usize planeValues = shape.plane_values;
  const usize xzValues = shape.xz_values;
  const usize volume = shape.volume;
  const usize numSlots = shape.output_slots;

  // copyIntoBuffer/copyFromBuffer are intentionally outside every parallel region because DataStore and
  // AbstractDataStore implementations are not thread-safe. Workers touch only staged vectors and write disjoint
  // output-vector elements. One scratch pencil is allocated per worker range for strided axes.
  constexpr usize k_TargetSlabBytes = 16 * 1024 * 1024;
  const usize targetSlabValues = std::max<usize>(1, k_TargetSlabBytes / sizeof(TIn));

  ThrottledMessageHandler progressThrottle(messageHandler);
  progressThrottle.reset(numSlots, "Projecting along axis (slab)");

  const auto reduceStagedPencil = [&](nonstd::span<TIn> pencil) -> TOut {
    if constexpr(ReduceFn::k_IsAssociative)
    {
      typename ReduceFn::template State<TOut> state{};
      constexpr usize k_CancelCheckValues = 65536;
      for(usize blockStart = 0; blockStart < pencil.size(); blockStart += k_CancelCheckValues)
      {
        if(blockStart != 0 && shouldCancel)
        {
          return {};
        }
        const usize blockEnd = blockStart + std::min(k_CancelCheckValues, pencil.size() - blockStart);
        for(usize index = blockStart; index < blockEnd; ++index)
        {
          reduce.accumulate(state, pencil[index]);
        }
      }
      return reduce.finalize(state);
    }
    else
    {
      return reduce.template reducePencil<TIn, TOut>(pencil);
    }
  };

  if(projDim == 0)
  {
    // X pencils are already contiguous in flat storage. Stage as many complete pencils as fit, then
    // run nth_element directly in each pencil's disjoint segment of the staged slab.
    const usize slotsPerBatch = std::max<usize>(1, std::min(numSlots, targetSlabValues / dimX));
    // slotsPerBatch <= numSlots == volume / dimX, so this allocation product is bounded by the checked volume.
    std::vector<TIn> slab(slotsPerBatch * dimX);
    std::vector<TOut> outBuffer(slotsPerBatch);

    for(usize slotStart = 0; slotStart < numSlots; slotStart += slotsPerBatch)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize slotCount = std::min(slotsPerBatch, numSlots - slotStart);
      const usize inputCount = slotCount * dimX;
      if(Result<> r = in.copyIntoBuffer(slotStart * dimX, nonstd::span<TIn>(slab.data(), inputCount)); r.invalid())
      {
        return {nonstd::make_unexpected(std::move(r.errors()))};
      }

      auto reducePencils = [&](const Range& range) {
        for(usize localSlot = range.min(); localSlot < range.max(); ++localSlot)
        {
          if(shouldCancel)
          {
            return;
          }
          TIn* pencilStartPtr = slab.data() + localSlot * dimX;
          outBuffer[localSlot] = reduceStagedPencil(nonstd::span<TIn>(pencilStartPtr, dimX));
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, slotCount);
      parallelAlgorithm.execute(reducePencils);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> r = out.copyFromBuffer(slotStart, nonstd::span<const TOut>(outBuffer.data(), slotCount)); r.invalid())
      {
        return {nonstd::make_unexpected(std::move(r.errors()))};
      }
      progressThrottle.incrementPercent(slotCount, 1);
    }
    messageHandler.sendInfoMessage(fmt::format("Axis projection: processed {} output columns.", numSlots));
    return {};
  }

  if(projDim == 1)
  {
    // A batch of complete Z planes is one contiguous store read. Each output slot is (z, x), and
    // its Y pencil is gathered from the plane batch into worker-local scratch before nth_element.
    const usize planesPerBatch = std::max<usize>(1, std::min(dimZ, targetSlabValues / planeValues));
    // planesPerBatch <= dimZ, so both products are bounded by the checked volume and projected size.
    std::vector<TIn> slab(planesPerBatch * planeValues);
    std::vector<TOut> outBuffer(planesPerBatch * dimX);

    for(usize zStart = 0; zStart < dimZ; zStart += planesPerBatch)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize planeCount = std::min(planesPerBatch, dimZ - zStart);
      const usize inputCount = planeCount * planeValues;
      const usize outputCount = planeCount * dimX;
      if(Result<> r = in.copyIntoBuffer(zStart * planeValues, nonstd::span<TIn>(slab.data(), inputCount)); r.invalid())
      {
        return {nonstd::make_unexpected(std::move(r.errors()))};
      }

      auto reducePencils = [&](const Range& range) {
        std::vector<TIn> pencil(dimY);
        for(usize localSlot = range.min(); localSlot < range.max(); ++localSlot)
        {
          if(shouldCancel)
          {
            return;
          }
          const usize localZ = localSlot / dimX;
          const usize x = localSlot - localZ * dimX;
          const usize planeOffset = localZ * planeValues;
          for(usize y = 0; y < dimY; ++y)
          {
            pencil[y] = slab[planeOffset + y * dimX + x];
          }
          outBuffer[localSlot] = reduceStagedPencil(nonstd::span<TIn>(pencil.data(), pencil.size()));
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, outputCount);
      parallelAlgorithm.execute(reducePencils);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> r = out.copyFromBuffer(zStart * dimX, nonstd::span<const TOut>(outBuffer.data(), outputCount)); r.invalid())
      {
        return {nonstd::make_unexpected(std::move(r.errors()))};
      }
      progressThrottle.incrementPercent(outputCount, 1);
    }
    messageHandler.sendInfoMessage(fmt::format("Axis projection: processed {} output columns.", numSlots));
    return {};
  }

  // Z pencils require values from every plane. Batch consecutive Y rows, read each plane's matching
  // contiguous row block serially into a Z-major slab, then gather each (y, x) pencil in parallel.
  const usize rowsPerBatch = std::max<usize>(1, std::min(dimY, targetSlabValues / xzValues));
  // rowsPerBatch <= dimY, so both products are bounded by the checked volume and projected size.
  std::vector<TIn> slab(rowsPerBatch * xzValues);
  std::vector<TOut> outBuffer(rowsPerBatch * dimX);

  for(usize yStart = 0; yStart < dimY; yStart += rowsPerBatch)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize rowCount = std::min(rowsPerBatch, dimY - yStart);
    const usize planeRowValues = rowCount * dimX;
    const usize outputCount = planeRowValues;
    for(usize z = 0; z < dimZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      // z*planeValues, yStart*dimX, their source-offset sum, and z*planeRowValues are bounded by the checked volume because z < dimZ, yStart < dimY, and rowCount <= dimY.
      if(Result<> r = in.copyIntoBuffer(z * planeValues + yStart * dimX, nonstd::span<TIn>(slab.data() + z * planeRowValues, planeRowValues)); r.invalid())
      {
        return {nonstd::make_unexpected(std::move(r.errors()))};
      }
    }

    auto reducePencils = [&](const Range& range) {
      std::vector<TIn> pencil(dimZ);
      for(usize localSlot = range.min(); localSlot < range.max(); ++localSlot)
      {
        if(shouldCancel)
        {
          return;
        }
        for(usize z = 0; z < dimZ; ++z)
        {
          pencil[z] = slab[z * planeRowValues + localSlot];
        }
        outBuffer[localSlot] = reduceStagedPencil(nonstd::span<TIn>(pencil.data(), pencil.size()));
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, outputCount);
    parallelAlgorithm.execute(reducePencils);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> r = out.copyFromBuffer(yStart * dimX, nonstd::span<const TOut>(outBuffer.data(), outputCount)); r.invalid())
    {
      return {nonstd::make_unexpected(std::move(r.errors()))};
    }
    progressThrottle.incrementPercent(outputCount, 1);
  }

  messageHandler.sendInfoMessage(fmt::format("Axis projection: processed {} output columns.", numSlots));
  return {};
}
} // namespace detail

/**
 * @brief Validates an axis projection and selects resident staging or bounded true-2-D OOC execution.
 */
template <class TIn, class TOut, class ReduceFn>
Result<> ApplyAxisProjection(const AbstractDataStore<TIn>& in, AbstractDataStore<TOut>& out, const SizeVec3Type& dims, usize projDim, ReduceFn reduce, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_AxisProjection2DTargetBytes)
{
  auto shapeResult = detail::ValidateAxisProjection(in, out, dims, projDim);
  if(shapeResult.invalid())
  {
    return ConvertResult(std::move(shapeResult));
  }
  const detail::AxisProjectionShape& shape = shapeResult.value();
  if(shape.empty)
  {
    return {};
  }

  // The non-associative contract accepts a mutable pencil and therefore requires the image store's normal {Z,Y,X}
  // tuple rank. Associative reducers only consume values and remain compatible with flat stores used by direct
  // engine callers.
  if constexpr(!ReduceFn::k_IsAssociative)
  {
    if(in.getTupleShape().size() != 3)
    {
      return MakeErrorResult(-8344, fmt::format("Axis projection (median pencil) requires a 3-dimensional input tuple shape, but got rank {} for projection dimensions {}.", in.getTupleShape().size(),
                                                string_utilities::FormatDimensions3D(dims)));
    }
  }

  const bool usesOutOfCoreEndpoint = in.getStoreType() == IDataStore::StoreType::OutOfCore || out.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(!usesOutOfCoreEndpoint)
  {
    if(const auto* inMemoryStorePtr = dynamic_cast<const DataStore<TIn>*>(&in); inMemoryStorePtr != nullptr)
    {
      return detail::ApplyAxisProjectionInMemory(*inMemoryStorePtr, out, shape, projDim, reduce, shouldCancel);
    }
  }
  if(shape.dim_z == 1 && usesOutOfCoreEndpoint)
  {
    return detail::ApplyAxisProjection2D(in, out, shape, projDim, reduce, shouldCancel, target2DBytes);
  }
  return detail::ApplyAxisProjectionResident(in, out, shape, projDim, reduce, shouldCancel, messageHandler);
}
} // namespace nx::core::ImageProcessing
