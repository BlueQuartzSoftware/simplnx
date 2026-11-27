#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/task_arena.h>
#endif

namespace nx::core::ImageProcessing
{
namespace detail
{
// Clamp a (possibly negative or over-range) neighbor coordinate to [0, dim-1] (ZeroFluxNeumann).
inline usize ClampCoord(int64 coord, usize dim)
{
  if(coord < 0)
  {
    return 0;
  }
  if(coord >= static_cast<int64>(dim))
  {
    return dim - 1;
  }
  return static_cast<usize>(coord);
}

inline constexpr usize k_BoxNeighborhood2DTargetBytes = 64ULL * 1024ULL * 1024ULL;

struct BoxNeighborhood2DPlan
{
  bool full_width = false;
  usize core_rows = 0;
  usize core_columns = 0;
  usize maximum_input_rows = 0;
  usize maximum_input_columns = 0;
  usize input_buffer_values = 0;
  usize output_buffer_values = 0;
  usize resident_bytes = 0;
};

inline usize BoxNeighborhood2DWorkerCount()
{
  return std::max<usize>(1, std::thread::hardware_concurrency());
}

inline bool TryMultiplyBoxNeighborhoodSize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool TryAddBoxNeighborhoodSize(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline bool TryComputeBoxNeighborhood2DLayout(usize inputRows, usize inputColumns, usize outputRows, usize outputColumns, usize valueBytes, usize workerScratchBytes, usize& inputValues,
                                              usize& outputValues, usize& residentBytes)
{
  usize inputBytes = 0;
  usize outputBytes = 0;
  usize bufferBytes = 0;
  return TryMultiplyBoxNeighborhoodSize(inputRows, inputColumns, inputValues) && TryMultiplyBoxNeighborhoodSize(outputRows, outputColumns, outputValues) &&
         TryMultiplyBoxNeighborhoodSize(inputValues, valueBytes, inputBytes) && TryMultiplyBoxNeighborhoodSize(outputValues, valueBytes, outputBytes) &&
         TryAddBoxNeighborhoodSize(inputBytes, outputBytes, bufferBytes) && TryAddBoxNeighborhoodSize(bufferBytes, workerScratchBytes, residentBytes);
}

inline Result<BoxNeighborhood2DPlan> CreateBoxNeighborhood2DPlan(usize dimX, usize dimY, usize radiusX, usize radiusY, usize valueBytes, usize targetBytes, usize workerScratchBytes)
{
  if(dimX == 0 || dimY == 0 || valueBytes == 0)
  {
    return MakeErrorResult<BoxNeighborhood2DPlan>(-8615,
                                                  fmt::format("Box-neighborhood 2D dimensions and value size must be nonzero. Dimensions: {} x {}; value size: {} bytes.", dimX, dimY, valueBytes));
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dimX > k_Int64Max || dimY > k_Int64Max || radiusX > k_Int64Max || radiusY > k_Int64Max)
  {
    return MakeErrorResult<BoxNeighborhood2DPlan>(
        -8615, fmt::format("Box-neighborhood 2D dimensions or radius exceed the supported signed coordinate range. Dimensions: {} x {}; radius: {} x {}.", dimX, dimY, radiusX, radiusY));
  }

  usize haloX = 0;
  usize haloY = 0;
  if(!TryMultiplyBoxNeighborhoodSize(radiusX, 2, haloX) || !TryMultiplyBoxNeighborhoodSize(radiusY, 2, haloY))
  {
    return MakeErrorResult<BoxNeighborhood2DPlan>(-8615, fmt::format("Box-neighborhood 2D radius overflows the halo size. Radius: {} x {}.", radiusX, radiusY));
  }
  const auto maximumInputExtent = [](usize coreExtent, usize haloExtent, usize dimension) { return coreExtent + std::min(haloExtent, dimension - coreExtent); };

  BoxNeighborhood2DPlan plan;
  auto fullWidthFits = [&](usize coreRows) {
    usize inputValues = 0;
    usize outputValues = 0;
    usize residentBytes = 0;
    return TryComputeBoxNeighborhood2DLayout(maximumInputExtent(coreRows, haloY, dimY), dimX, coreRows, dimX, valueBytes, workerScratchBytes, inputValues, outputValues, residentBytes) &&
           residentBytes <= targetBytes;
  };
  if(fullWidthFits(1))
  {
    usize lower = 1;
    usize upper = dimY;
    while(lower < upper)
    {
      const usize midpoint = lower + (upper - lower) / 2 + 1;
      if(fullWidthFits(midpoint))
      {
        lower = midpoint;
      }
      else
      {
        upper = midpoint - 1;
      }
    }
    plan.full_width = true;
    plan.core_rows = lower;
    plan.core_columns = dimX;
  }
  else
  {
    const usize inputRows = maximumInputExtent(1, haloY, dimY);
    auto tileFits = [&](usize coreColumns) {
      usize inputValues = 0;
      usize outputValues = 0;
      usize residentBytes = 0;
      return TryComputeBoxNeighborhood2DLayout(inputRows, maximumInputExtent(coreColumns, haloX, dimX), 1, coreColumns, valueBytes, workerScratchBytes, inputValues, outputValues, residentBytes) &&
             residentBytes <= targetBytes;
    };
    if(!tileFits(1))
    {
      return MakeErrorResult<BoxNeighborhood2DPlan>(
          -8615,
          fmt::format("Box-neighborhood 2D target ({} bytes) cannot hold one output value, its clipped {} x {} halo, and {} bytes of worker scratch for dimensions {} x {} and value size {} bytes.",
                      targetBytes, radiusX, radiusY, workerScratchBytes, dimX, dimY, valueBytes));
    }
    usize lower = 1;
    usize upper = dimX;
    while(lower < upper)
    {
      const usize midpoint = lower + (upper - lower) / 2 + 1;
      if(tileFits(midpoint))
      {
        lower = midpoint;
      }
      else
      {
        upper = midpoint - 1;
      }
    }
    plan.full_width = false;
    plan.core_rows = 1;
    plan.core_columns = lower;
  }

  plan.maximum_input_rows = plan.full_width ? maximumInputExtent(plan.core_rows, haloY, dimY) : maximumInputExtent(1, haloY, dimY);
  plan.maximum_input_columns = plan.full_width ? dimX : maximumInputExtent(plan.core_columns, haloX, dimX);
  if(!TryComputeBoxNeighborhood2DLayout(plan.maximum_input_rows, plan.maximum_input_columns, plan.core_rows, plan.core_columns, valueBytes, workerScratchBytes, plan.input_buffer_values,
                                        plan.output_buffer_values, plan.resident_bytes))
  {
    return MakeErrorResult<BoxNeighborhood2DPlan>(
        -8615, fmt::format("Box-neighborhood 2D buffer layout overflows for dimensions {} x {}, radius {} x {}, and value size {} bytes.", dimX, dimY, radiusX, radiusY, valueBytes));
  }
  return {plan};
}

template <class T, class ReduceFnT>
struct BoxPlaneBody
{
  const T* slab; // read-only slab covering Z in [zLo, zHi], laid out [slabZ][y][x]
  T* out_plane;  // output plane, sizeX*sizeY
  usize dim_x;
  usize dim_y;
  usize dim_z;
  usize rx;
  usize ry;
  usize rz;
  usize z_lo; // slab's first Z index
  usize z;    // output plane's Z index
  usize neighborhood_size;
  const ReduceFnT& reduce_fn;

  void operator()(const Range& range) const
  {
    const usize sliceValues = dim_x * dim_y;
    std::vector<T> scratch(neighborhood_size); // one per worker invocation, reused across tuples
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize x = tuple % dim_x;
      const usize y = tuple / dim_x;
      usize idx = 0;
      for(usize dz = 0; dz <= 2 * rz; ++dz)
      {
        const usize nz = ClampCoord(static_cast<int64>(z) - static_cast<int64>(rz) + static_cast<int64>(dz), dim_z);
        const usize slabZ = nz - z_lo; // nz is guaranteed within [zLo, zHi] because zLo/zHi are the clamped extremes
        const usize slabZBase = slabZ * sliceValues;
        for(usize dy = 0; dy <= 2 * ry; ++dy)
        {
          const usize ny = ClampCoord(static_cast<int64>(y) - static_cast<int64>(ry) + static_cast<int64>(dy), dim_y);
          const usize rowBase = slabZBase + ny * dim_x;
          for(usize dx = 0; dx <= 2 * rx; ++dx)
          {
            const usize nx = ClampCoord(static_cast<int64>(x) - static_cast<int64>(rx) + static_cast<int64>(dx), dim_x);
            scratch[idx++] = slab[rowBase + nx];
          }
        }
      }
      out_plane[tuple] = reduce_fn(nonstd::span<T>(scratch.data(), scratch.size()));
    }
  }
};

template <class T, class ReduceFnT>
struct Box2DBlockBody
{
  const T* input;
  T* output;
  usize dim_x;
  usize dim_y;
  usize radius_x;
  usize radius_y;
  usize radius_z;
  usize input_x_begin;
  usize input_y_begin;
  usize input_width;
  usize output_x_begin;
  usize output_y_begin;
  usize output_width;
  usize neighborhood_size;
  const ReduceFnT& reduce_fn;
  const std::atomic_bool& should_cancel;

  void operator()(const Range& range) const
  {
    std::vector<T> scratch(neighborhood_size);
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      if(((tuple - range.min()) & 4095ULL) == 0 && should_cancel)
      {
        return;
      }
      const usize localX = tuple % output_width;
      const usize localY = tuple / output_width;
      const usize x = output_x_begin + localX;
      const usize y = output_y_begin + localY;
      usize index = 0;
      for(usize dz = 0; dz <= 2 * radius_z; ++dz)
      {
        static_cast<void>(dz);
        for(usize dy = 0; dy <= 2 * radius_y; ++dy)
        {
          const usize neighborY = ClampCoord(static_cast<int64>(y) - static_cast<int64>(radius_y) + static_cast<int64>(dy), dim_y);
          const usize inputRow = (neighborY - input_y_begin) * input_width;
          for(usize dx = 0; dx <= 2 * radius_x; ++dx)
          {
            const usize neighborX = ClampCoord(static_cast<int64>(x) - static_cast<int64>(radius_x) + static_cast<int64>(dx), dim_x);
            scratch[index++] = input[inputRow + neighborX - input_x_begin];
          }
        }
      }
      output[tuple] = reduce_fn(nonstd::span<T>(scratch.data(), scratch.size()));
    }
  }
};

template <class T, class ReduceFnT>
Result<> ExecuteBoxNeighborhood2D(const AbstractDataStore<T>& inputStore, AbstractDataStore<T>& outputStore, const SizeVec3Type& dims, const std::array<usize, 3>& radius, usize neighborhoodSize,
                                  const BoxNeighborhood2DPlan& plan, ReduceFnT& reduceFn, const std::atomic_bool& shouldCancel, usize maximumWorkers, ThrottledMessageHandler& progressThrottle)
{
  auto inputBuffer = std::make_unique<T[]>(plan.input_buffer_values);
  auto outputBuffer = std::make_unique<T[]>(plan.output_buffer_values);
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize yStep = plan.full_width ? plan.core_rows : 1;
  const usize xStep = plan.full_width ? dimX : plan.core_columns;
  progressThrottle.reset(dimX * dimY, "Applying 2D neighborhood filter");
  for(usize outputYBegin = 0; outputYBegin < dimY; outputYBegin += yStep)
  {
    const usize outputRows = std::min(yStep, dimY - outputYBegin);
    for(usize outputXBegin = 0; outputXBegin < dimX; outputXBegin += xStep)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize outputColumns = std::min(xStep, dimX - outputXBegin);
      const usize outputXEnd = outputXBegin + outputColumns;
      const usize outputYEnd = outputYBegin + outputRows;
      const usize inputXBegin = outputXBegin > radius[0] ? outputXBegin - radius[0] : 0;
      const usize inputXEnd = outputXEnd + std::min(radius[0], dimX - outputXEnd);
      const usize inputYBegin = outputYBegin > radius[1] ? outputYBegin - radius[1] : 0;
      const usize inputYEnd = outputYEnd + std::min(radius[1], dimY - outputYEnd);
      const usize inputWidth = inputXEnd - inputXBegin;
      if(plan.full_width)
      {
        const usize inputValues = (inputYEnd - inputYBegin) * dimX;
        if(Result<> result = inputStore.copyIntoBuffer(inputYBegin * dimX, nonstd::span<T>(inputBuffer.get(), inputValues)); result.invalid())
        {
          return result;
        }
      }
      else
      {
        for(usize inputY = inputYBegin; inputY < inputYEnd; ++inputY)
        {
          if(Result<> result = inputStore.copyIntoBuffer(inputY * dimX + inputXBegin, nonstd::span<T>(inputBuffer.get() + (inputY - inputYBegin) * inputWidth, inputWidth)); result.invalid())
          {
            return result;
          }
          if(shouldCancel)
          {
            return {};
          }
        }
      }
      if(shouldCancel)
      {
        return {};
      }

      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, outputRows * outputColumns);
      const auto body = Box2DBlockBody<T, ReduceFnT>{.input = inputBuffer.get(),
                                                     .output = outputBuffer.get(),
                                                     .dim_x = dimX,
                                                     .dim_y = dimY,
                                                     .radius_x = radius[0],
                                                     .radius_y = radius[1],
                                                     .radius_z = radius[2],
                                                     .input_x_begin = inputXBegin,
                                                     .input_y_begin = inputYBegin,
                                                     .input_width = inputWidth,
                                                     .output_x_begin = outputXBegin,
                                                     .output_y_begin = outputYBegin,
                                                     .output_width = outputColumns,
                                                     .neighborhood_size = neighborhoodSize,
                                                     .reduce_fn = reduceFn,
                                                     .should_cancel = shouldCancel};
#ifdef SIMPLNX_ENABLE_MULTICORE
      const usize boundedWorkers = std::min(maximumWorkers, static_cast<usize>(std::numeric_limits<int>::max()));
      tbb::task_arena arena(static_cast<int>(boundedWorkers));
      arena.execute([&] { parallelAlgorithm.execute(body); });
#else
      parallelAlgorithm.execute(body);
#endif
      if(shouldCancel)
      {
        return {};
      }

      const usize outputStart = outputYBegin * dimX + outputXBegin;
      const usize outputValues = plan.full_width ? outputRows * dimX : outputColumns;
      if(Result<> result = outputStore.copyFromBuffer(outputStart, nonstd::span<const T>(outputBuffer.get(), outputValues)); result.invalid())
      {
        return result;
      }
      progressThrottle.incrementPercent(outputValues);
    }
  }
  return {};
}

template <class T, class ReduceFnT>
struct BoxVolumeBody
{
  const T* input;
  T* output;
  usize dim_x;
  usize dim_y;
  usize dim_z;
  usize rx;
  usize ry;
  usize rz;
  usize neighborhood_size;
  const ReduceFnT& reduce_fn;
  const std::atomic_bool& should_cancel;
  const std::function<void(usize)>& report_progress;

  void operator()(const Range& range) const
  {
    const usize sliceValues = dim_x * dim_y;
    std::vector<T> scratch(neighborhood_size);
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      if(((tuple - range.min()) & 4095ULL) == 0)
      {
        if(should_cancel)
        {
          return;
        }
        if(tuple != range.min())
        {
          report_progress(4096);
        }
      }
      const usize z = tuple / sliceValues;
      const usize planeIndex = tuple - z * sliceValues;
      const usize y = planeIndex / dim_x;
      const usize x = planeIndex - y * dim_x;
      usize index = 0;
      for(usize dz = 0; dz <= 2 * rz; ++dz)
      {
        const usize neighborZ = ClampCoord(static_cast<int64>(z) - static_cast<int64>(rz) + static_cast<int64>(dz), dim_z);
        const usize planeBase = neighborZ * sliceValues;
        for(usize dy = 0; dy <= 2 * ry; ++dy)
        {
          const usize neighborY = ClampCoord(static_cast<int64>(y) - static_cast<int64>(ry) + static_cast<int64>(dy), dim_y);
          const usize rowBase = planeBase + neighborY * dim_x;
          for(usize dx = 0; dx <= 2 * rx; ++dx)
          {
            const usize neighborX = ClampCoord(static_cast<int64>(x) - static_cast<int64>(rx) + static_cast<int64>(dx), dim_x);
            scratch[index++] = input[rowBase + neighborX];
          }
        }
      }
      output[tuple] = reduce_fn(nonstd::span<T>(scratch.data(), scratch.size()));
    }
    if(!should_cancel && !range.empty())
    {
      report_progress((range.size() - 1) % 4096 + 1);
    }
  }
};
} // namespace detail

/**
 * @brief Applies a box-neighborhood reduction to a scalar image. For each output Z-plane, bulk-reads the
 *        clamped Z-slab [z-rz, z+rz] via copyIntoBuffer, then (in parallel over the plane) gathers each
 *        voxel's (2rx+1)(2ry+1)(2rz+1) neighborhood with per-axis edge clamping and applies @p reduceFn.
 *        Resident DataStore endpoints use direct spans. True 3D OOC execution uses an input halo slab and one
 *        output plane. True 2D OOC execution uses a checked <=64 MiB full-width row block or one-row X tile;
 *        the aggregate includes the typed input halo, typed output, and one neighborhood scratch per bounded
 *        worker. Datastore transfers remain outside the parallel voxel loop.
 *
 * @pre scalar array (1 component); inputStore/outputStore have dimX*dimY*dimZ values.
 * @tparam ReduceFnT  `T reduceFn(nonstd::span<T> scratch) const` (may mutate scratch, e.g. nth_element).
 */
template <class T, class ReduceFnT>
Result<> ApplyBoxNeighborhood(const AbstractDataStore<T>& inputStore, AbstractDataStore<T>& outputStore, const SizeVec3Type& dims, const std::array<usize, 3>& radius, ReduceFnT reduceFn,
                              const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_BoxNeighborhood2DTargetBytes)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize rx = radius[0];
  const usize ry = radius[1];
  const usize rz = radius[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return MakeErrorResult(-8610, fmt::format("Box-neighborhood filtering requires nonzero image dimensions. Dimensions: {} x {} x {}.", dimX, dimY, dimZ));
  }
  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  auto checkedDiameter = [](usize value, usize& diameter) {
    if(value > (std::numeric_limits<usize>::max() - 1) / 2)
    {
      return false;
    }
    diameter = 2 * value + 1;
    return true;
  };
  usize sliceValues = 0;
  usize volumeValues = 0;
  if(!checkedMultiply(dimX, dimY, sliceValues) || !checkedMultiply(sliceValues, dimZ, volumeValues))
  {
    return MakeErrorResult(-8611, fmt::format("Box-neighborhood image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dimX, dimY, dimZ));
  }
  usize diameterX = 0;
  usize diameterY = 0;
  usize diameterZ = 0;
  usize neighborhoodPlaneSize = 0;
  usize neighborhoodSize = 0;
  if(!checkedDiameter(rx, diameterX) || !checkedDiameter(ry, diameterY) || !checkedDiameter(rz, diameterZ) || !checkedMultiply(diameterX, diameterY, neighborhoodPlaneSize) ||
     !checkedMultiply(neighborhoodPlaneSize, diameterZ, neighborhoodSize))
  {
    return MakeErrorResult(-8612, fmt::format("Box-neighborhood radius overflows the addressable neighborhood size. Radius: {} x {} x {}.", rx, ry, rz));
  }
  if(inputStore.getSize() != volumeValues)
  {
    return MakeErrorResult(-8613, fmt::format("Box-neighborhood input store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              inputStore.getSize(), volumeValues, dimX, dimY, dimZ));
  }
  if(outputStore.getSize() != volumeValues)
  {
    return MakeErrorResult(-8614, fmt::format("Box-neighborhood output store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              outputStore.getSize(), volumeValues, dimX, dimY, dimZ));
  }
  if(shouldCancel)
  {
    return {};
  }

  messageHandler.sendInfoMessage("Applying neighborhood filter");
  ThrottledMessageHandler progressThrottle(messageHandler);

  const auto* inMemoryInputStorePtr = dynamic_cast<const DataStore<T>*>(&inputStore);
  auto* inMemoryOutputStorePtr = dynamic_cast<DataStore<T>*>(&outputStore);
  const bool usesOutOfCoreStore = inputStore.getStoreType() == IDataStore::StoreType::OutOfCore || outputStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(inMemoryInputStorePtr != nullptr && inMemoryOutputStorePtr != nullptr && !usesOutOfCoreStore)
  {
    const nonstd::span<const T> inputValues = inMemoryInputStorePtr->createSpan();
    nonstd::span<T> outputValues = inMemoryOutputStorePtr->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, volumeValues);
    progressThrottle.reset(volumeValues, "Computing resident neighborhoods");
    std::mutex progressMutex;
    const std::function<void(usize)> sendThreadSafeProgress = [&](usize delta) {
      const std::lock_guard<std::mutex> guard(progressMutex);
      progressThrottle.incrementPercent(delta);
    };
    parallelAlgorithm.execute(
        detail::BoxVolumeBody<T, ReduceFnT>{inputValues.data(), outputValues.data(), dimX, dimY, dimZ, rx, ry, rz, neighborhoodSize, reduceFn, shouldCancel, sendThreadSafeProgress});
    return {};
  }

  progressThrottle.reset(dimZ, "Applying neighborhood filter");

  std::unique_ptr<T[]> outPlane;
  if(dimZ > 1)
  {
    outPlane = std::make_unique<T[]>(sliceValues);
  }

  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(dimZ == 1)
    {
      const usize workerCount = detail::BoxNeighborhood2DWorkerCount();
      usize workerScratchValues = 0;
      usize workerScratchBytes = 0;
      if(!detail::TryMultiplyBoxNeighborhoodSize(neighborhoodSize, workerCount, workerScratchValues) || !detail::TryMultiplyBoxNeighborhoodSize(workerScratchValues, sizeof(T), workerScratchBytes))
      {
        return MakeErrorResult(-8612,
                               fmt::format("Box-neighborhood worker scratch overflows for neighborhood size {}, value size {} bytes, and {} workers.", neighborhoodSize, sizeof(T), workerCount));
      }
      auto planResult = detail::CreateBoxNeighborhood2DPlan(dimX, dimY, rx, ry, sizeof(T), target2DBytes, workerScratchBytes);
      if(planResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(planResult));
      }
      Result<> result = detail::ExecuteBoxNeighborhood2D(inputStore, outputStore, dims, radius, neighborhoodSize, planResult.value(), reduceFn, shouldCancel, workerCount, progressThrottle);
      if(result.invalid())
      {
        return result;
      }
    }
    else
    {
      const usize zLo = (z >= rz) ? (z - rz) : 0;
      const usize zHi = std::min(dimZ - 1, z + rz);
      const usize slabDepth = zHi - zLo + 1;
      auto slab = std::make_unique<T[]>(slabDepth * sliceValues);
      if(Result<> r = inputStore.copyIntoBuffer(zLo * sliceValues, nonstd::span<T>(slab.get(), slabDepth * sliceValues)); r.invalid())
      {
        return r;
      }

      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, sliceValues);
      parallelAlgorithm.execute(detail::BoxPlaneBody<T, ReduceFnT>{slab.get(), outPlane.get(), dimX, dimY, dimZ, rx, ry, rz, zLo, z, neighborhoodSize, reduceFn});

      if(Result<> r = outputStore.copyFromBuffer(z * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
      {
        return r;
      }
    }
    if(dimZ > 1 && !shouldCancel)
    {
      progressThrottle.incrementPercent(1, 1);
    }
  }
  return {};
}
} // namespace nx::core::ImageProcessing
