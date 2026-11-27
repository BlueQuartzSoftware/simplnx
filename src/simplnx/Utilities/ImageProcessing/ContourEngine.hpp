#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/RadiusOneStencil2D.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

namespace nx::core::ImageProcessing
{
// A radius-1 connectivity neighborhood with the center excluded has at most 26 neighbors (Box: 3^3 - 1). This
// single invariant sizes the per-voxel gather buffer, bounds the offset-count assert, and backs the docs below.
inline constexpr usize k_MaxRadius1Neighbors = 26;

/**
 * @brief Build the radius-1 local-neighbor offset list for a contour engine, EXCLUDING the {0,0,0} center.
 *
 * A contour voxel is decided by comparing the center to its connectivity neighbors, so the center itself is
 * never part of that neighbor set. Face connectivity (6-connected in 3D / 4-connected in 2D) uses the Cross
 * radius-1 kernel; full connectivity (26-connected / 8-connected) uses the Box radius-1 kernel. This matches
 * ITK's contour filters, whose FullyConnectedOff/On selects minimum (face) vs maximum (face+edge+vertex)
 * connectivity. The returned offsets always fit a radius-1 box, so a downstream engine only needs the
 * [z-1, z+1] Z-slab, and there are at most 26 of them.
 *
 * @param fullyConnected false -> Cross (face) connectivity; true -> Box (full) connectivity.
 */
inline std::vector<SEOffset> MakeContourNeighborOffsets(bool fullyConnected)
{
  StructuringElement se = MakeStructuringElement(fullyConnected ? KernelType::Box : KernelType::Cross, {1, 1, 1});
  std::vector<SEOffset> offsets;
  offsets.reserve(se.offsets.size());
  for(const SEOffset& off : se.offsets)
  {
    if(off.dx == 0 && off.dy == 0 && off.dz == 0)
    {
      continue; // exclude the center; a voxel is never its own contour neighbor
    }
    offsets.push_back(off);
  }
  return offsets;
}

/**
 * @brief Per-voxel predicate for the BINARY contour operation, faithful to itk::BinaryContourImageFilter.
 *
 * A voxel that equals @c foreground is on the contour (emits @c foreground) iff at least one of its in-bounds
 * connectivity neighbors is NOT foreground; a foreground voxel completely surrounded by foreground is interior
 * and becomes @c background. A voxel that is NOT foreground passes its OWN value through unchanged -- exactly
 * ITK, which writes every non-foreground pixel to its input value (so on a strictly binary image "background
 * stays background", and on a labeled image a stray non-foreground label survives). This own-value passthrough
 * is why the binary contour needs no binary-input safeguard: the predicate is ITK-faithful on any integer input.
 *
 * The neighbor span holds ONLY the in-bounds neighbors (the engine drops out-of-image offsets), matching ITK's
 * contour filters, which have no boundary condition and compare a border voxel only against real neighbors.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
struct BinaryContourPredicate
{
  T foreground;
  T background;

  T operator()(T center, nonstd::span<const T> inBoundsNeighbors) const
  {
    if(center != foreground)
    {
      return center; // ITK writes non-foreground pixels to their own value
    }
    for(T n : inBoundsNeighbors)
    {
      if(n != foreground)
      {
        return foreground; // a foreground voxel with a differing in-bounds neighbor is on the contour
      }
    }
    return background; // foreground interior (every in-bounds neighbor is foreground)
  }
};

/**
 * @brief Factory that builds a @ref BinaryContourPredicate for a specific element type from the filter's
 *        Float64 foreground/background parameters. The Float64 values are cast to the integer element type
 *        here (the caller MUST have range-validated them at preflight so the narrowing cast is well-defined).
 *        A non-templated factory lets @ref ExecuteContourImageFilter dispatch over the integer element types
 *        while the per-voxel predicate stays typed; a later Label contour supplies its own factory the same way.
 */
struct BinaryContourPredicateFactory
{
  float64 foreground;
  float64 background;

  template <class T>
  BinaryContourPredicate<T> make() const
  {
    return BinaryContourPredicate<T>{static_cast<T>(foreground), static_cast<T>(background)};
  }
};

/**
 * @brief Per-voxel predicate for the LABEL contour operation, faithful to itk::LabelContourImageFilter.
 *
 * There is NO foreground concept: every value other than @c background is its own object/region. A voxel equal
 * to @c background is NEVER a contour (it always emits @c background, even when it sits next to a labeled
 * region). A labeled voxel is on the contour -- and KEEPS its own label -- iff at least one of its in-bounds
 * connectivity neighbors has a DIFFERENT value (a different label, or the background); a labeled voxel whose
 * in-bounds neighbors are all its own label is interior and becomes @c background. Labels are preserved exactly
 * between input and output on the contour.
 *
 * The neighbor span holds ONLY the in-bounds neighbors (the engine drops out-of-image offsets), matching ITK's
 * contour filters, which have no boundary condition and compare a border voxel only against real neighbors.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
struct LabelContourPredicate
{
  T background;

  T operator()(T center, nonstd::span<const T> inBoundsNeighbors) const
  {
    if(center == background)
    {
      return background; // a background voxel is never a contour, even next to a labeled region
    }
    for(T n : inBoundsNeighbors)
    {
      if(n != center)
      {
        return center; // a labeled voxel touching a different value is on the contour; keep its own label
      }
    }
    return background; // interior of a region (every in-bounds neighbor is the same label)
  }
};

/**
 * @brief Factory that builds a @ref LabelContourPredicate for a specific element type from the filter's Float64
 *        background parameter (cast to the integer element type here; the caller MUST have range-validated it at
 *        preflight so the narrowing cast is well-defined). Mirrors @ref BinaryContourPredicateFactory so it plugs
 *        into @ref ExecuteContourImageFilter with no façade change.
 */
struct LabelContourPredicateFactory
{
  float64 background;

  template <class T>
  LabelContourPredicate<T> make() const
  {
    return LabelContourPredicate<T>{static_cast<T>(background)};
  }
};

namespace detail
{
inline bool ContourCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

struct ContourResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize required_bytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return required_bytes > 0 && reservation.sizeBytes() == required_bytes;
  }
};

inline bool ShouldUseContourResidentState(const SizeVec3Type& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateContourResidentWorkingMemoryBytes(const SizeVec3Type& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize residentValues = 0;
  usize requiredBytes = 0;
  if(!ContourCheckedMultiply(dims[0], dims[1], sliceValues) || !ContourCheckedMultiply(sliceValues, dims[2], volumeValues) || !ContourCheckedMultiply(volumeValues, usize{2}, residentValues) ||
     !ContourCheckedMultiply(residentValues, sizeof(T), requiredBytes))
  {
    return MakeErrorResult<usize>(
        -8762, fmt::format("Contour dimensions ({}) and element size ({} bytes) overflow while sizing the resident input and output state.", string_utilities::FormatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<ContourResidentMemoryAllocation> ReserveContourResidentWorkingMemory(const SizeVec3Type& dims)
{
  auto requiredResult = CalculateContourResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<ContourResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {ContourResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

inline usize Contour2DWorkerCount()
{
  return std::max<usize>(1, std::thread::hardware_concurrency());
}

template <class T>
usize Contour2DWorkerScratchBytes(usize workerCount)
{
  return k_MaxRadius1Neighbors * sizeof(T) * workerCount;
}

/**
 * @brief ParallelDataAlgorithm body that fills one output Z-plane for the radius-1 contour engine. For each
 *        (y,x) in the plane it reads the center from the already-read Z-slab, gathers only the IN-BOUNDS
 *        connectivity neighbors into a small stack-local buffer (out-of-bounds offsets are dropped, never
 *        substituted), and applies @p predicate. Stateless and side-effect-free apart from writing distinct
 *        @c outPlane slots, so it is safe to run concurrently across the plane's tuples.
 *
 * @tparam T scalar voxel type. @tparam PredicateT `T(T center, nonstd::span<const T> inBoundsNeighbors) const`.
 */
template <class T, class PredicateT>
struct ContourPlaneBody
{
  const T* slab;           // read-only slab or volume beginning at zLo, laid out [slabZ][y][x]
  T* out_plane;             // output plane, dimX*dimY
  const SEOffset* offsets; // neighbor offsets (center {0,0,0} already excluded by the caller)
  usize num_offsets;
  usize dim_x;
  usize dim_y;
  usize dim_z;
  usize z_lo; // slab's first Z index
  usize z;   // output plane's Z index
  const PredicateT& predicate;

  void operator()(const Range& range) const
  {
    const usize sliceValues = dim_x * dim_y;
    const int64 dimXi = static_cast<int64>(dim_x);
    const int64 dimYi = static_cast<int64>(dim_y);
    const int64 dimZi = static_cast<int64>(dim_z);
    const int64 zi = static_cast<int64>(z); // loop-invariant across the whole plane (only x/y vary per tuple)
    // A radius-1 connectivity neighborhood with the center excluded holds at most k_MaxRadius1Neighbors, so
    // gather into a fixed-capacity, per-worker stack buffer -- no per-voxel heap allocation, and thread-safe
    // because it is local to this call.
    std::array<T, k_MaxRadius1Neighbors> neighbors{};

    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize x = tuple % dim_x;
      const usize y = tuple / dim_x;
      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);

      // The output plane's Z is always inside the slab or resident volume.
      const T center = slab[(z - z_lo) * sliceValues + y * dim_x + x];

      usize count = 0;
      for(usize k = 0; k < num_offsets; ++k)
      {
        const SEOffset& off = offsets[k];
        const int64 nx = xi + off.dx;
        const int64 ny = yi + off.dy;
        const int64 nz = zi + off.dz;
        if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
        {
          continue; // out-of-image neighbor is ignored (ITK contour compares only in-bounds neighbors)
        }
        const usize slabZ = static_cast<usize>(nz) - z_lo;
        neighbors[count++] = slab[slabZ * sliceValues + static_cast<usize>(ny) * dim_x + static_cast<usize>(nx)];
      }
      out_plane[tuple] = predicate(center, nonstd::span<const T>(neighbors.data(), count));
    }
  }
};

// Computes one output plane from three resident Z planes. Boundary flags omit
// neighbors before the body selects a plane pointer.
template <class T, class PredicateT>
struct ContourRollingPlaneBody
{
  const T* previous_plane;
  const T* current_plane;
  const T* next_plane;
  T* out_plane;
  const SEOffset* offsets;
  usize num_offsets;
  usize dim_x;
  usize dim_y;
  bool has_previous;
  bool has_next;
  const PredicateT& predicate;

  void operator()(const Range& range) const
  {
    const int64 dimXi = static_cast<int64>(dim_x);
    const int64 dimYi = static_cast<int64>(dim_y);
    std::array<T, k_MaxRadius1Neighbors> neighbors{};
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize x = tuple % dim_x;
      const usize y = tuple / dim_x;
      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);
      const T center = current_plane[y * dim_x + x];

      usize count = 0;
      for(usize offsetIndex = 0; offsetIndex < num_offsets; ++offsetIndex)
      {
        const SEOffset& offset = offsets[offsetIndex];
        const int64 neighborX = xi + offset.dx;
        const int64 neighborY = yi + offset.dy;
        const bool needsPreviousPlane = offset.dz < 0;
        const bool needsNextPlane = offset.dz > 0;
        if(neighborX < 0 || neighborX >= dimXi || neighborY < 0 || neighborY >= dimYi || (needsPreviousPlane && !has_previous) || (needsNextPlane && !has_next))
        {
          continue;
        }
        const T* neighborPlanePtr = needsPreviousPlane ? previous_plane : (needsNextPlane ? next_plane : current_plane);
        neighbors[count++] = neighborPlanePtr[static_cast<usize>(neighborY) * dim_x + static_cast<usize>(neighborX)];
      }
      out_plane[tuple] = predicate(center, nonstd::span<const T>(neighbors.data(), count));
    }
  }
};

template <class T, class PredicateT>
struct Contour2DBlockBody
{
  const T* input;
  T* output;
  const SEOffset* offsets;
  usize num_offsets;
  usize dim_x;
  usize dim_y;
  usize input_x_begin;
  usize input_y_begin;
  usize input_width;
  usize output_x_begin;
  usize output_y_begin;
  usize output_width;
  const PredicateT& predicate;

  void operator()(const Range& range) const
  {
    const int64 dimXi = static_cast<int64>(dim_x);
    const int64 dimYi = static_cast<int64>(dim_y);
    std::array<T, k_MaxRadius1Neighbors> neighbors{};
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize localX = tuple % output_width;
      const usize localY = tuple / output_width;
      const usize x = output_x_begin + localX;
      const usize y = output_y_begin + localY;
      const usize centerIndex = (y - input_y_begin) * input_width + (x - input_x_begin);
      const T center = input[centerIndex];

      usize count = 0;
      for(usize offsetIndex = 0; offsetIndex < num_offsets; ++offsetIndex)
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
        const usize inputIndex = (static_cast<usize>(neighborY) - input_y_begin) * input_width + (static_cast<usize>(neighborX) - input_x_begin);
        neighbors[count++] = input[inputIndex];
      }
      output[tuple] = predicate(center, nonstd::span<const T>(neighbors.data(), count));
    }
  }
};
} // namespace detail

/**
 * @brief Out-of-core-safe radius-1 local-neighbor "contour" engine.
 *
 * For true 3D, the engine reads input in a bounded, working-memory-derived band of Z planes (never a single
 * fixed plane count; degrades to the same four-plane footprint a tight grant always allowed) and writes output
 * in matching bands, instead of one store call per plane -- a lone Z-plane is exactly one store chunk for this
 * engine's typical geometry, and a single-chunk call always takes the codec's fully serial branch. It loads
 * each input plane once and writes each output plane once, in ascending Z order. The per-voxel radius-1 rolling
 * window (@ref ContourRollingPlaneBody) and its predicate over gathered neighbors are unchanged by the banding.
 *
 * There is a SINGLE implementation for in-core and out-of-core stores. Bulk copies are a memcpy in-core and
 * hyperslab I/O out-of-core. True 3D retains two working-memory-sized input bands (the band being computed and
 * the following band prefetched one band ahead), one output band, and a single carried boundary plane. True 2D
 * enters the checked fixed-capacity, full-width row-block, or overwide X-tile route before allocating a complete
 * plane, keeping typed input halo plus output at or below 64 MiB. Includes cancel checks and throttled progress
 * messages.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values; @p neighborOffsets
 *      excludes {0,0,0} and fits a radius-1 box (see @ref MakeContourNeighborOffsets).
 * @tparam T scalar voxel type. @tparam PredicateT `T(T center, nonstd::span<const T> inBoundsNeighbors) const`.
 */
template <class T, class PredicateT>
Result<> ApplyContour(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3Type& dims, const std::vector<SEOffset>& neighborOffsets, const PredicateT& predicate,
                      const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_RadiusOneStencil2DTargetBytes)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceValues = dimX * dimY;

  // The plane body gathers in-bounds neighbors into a fixed-capacity std::array<T, k_MaxRadius1Neighbors>; a
  // radius-1 box with the center excluded has at most that many offsets (MakeContourNeighborOffsets honors it).
  // ApplyContour is a public template, so guard the precondition at RUNTIME (not just a debug assert): a caller
  // that passes a larger offset list gets a clean error instead of a release-build stack-buffer overflow.
  if(neighborOffsets.size() > k_MaxRadius1Neighbors)
  {
    return MakeErrorResult(-8590, "Contour engine received more neighbor offsets than a radius-1 box allows (center excluded); the caller's neighbor-offset builder is incorrect.");
  }

  messageHandler.sendInfoMessage("Applying contour filter");
  ThrottledMessageHandler progressThrottle(messageHandler);
  progressThrottle.reset(sliceValues * dimZ, "Computing contour filter");
  std::mutex progressMutex;
  const auto sendThreadSafeProgress = [&](usize delta) {
    const std::lock_guard<std::mutex> guard(progressMutex);
    progressThrottle.incrementPercent(delta);
  };
  const auto reportCompletedPlane = [&]() { sendThreadSafeProgress(sliceValues); };

  auto runResident = [&](nonstd::span<const T> input, nonstd::span<T> output) -> Result<> {
    for(usize z = 0; z < dimZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, sliceValues);
      const auto body = detail::ContourPlaneBody<T, PredicateT>{.slab = input.data(),
                                                                .out_plane = output.data() + z * sliceValues,
                                                                .offsets = neighborOffsets.data(),
                                                                .num_offsets = neighborOffsets.size(),
                                                                .dim_x = dimX,
                                                                .dim_y = dimY,
                                                                .dim_z = dimZ,
                                                                .z_lo = 0,
                                                                .z = z,
                                                                .predicate = predicate};
      parallelAlgorithm.execute([&](const Range& range) {
        constexpr usize k_ProgressBatchValues = 4096;
        for(usize begin = range.min(); begin < range.max();)
        {
          const usize end = begin + std::min(k_ProgressBatchValues, range.max() - begin);
          body(Range(begin, end));
          if(!shouldCancel)
          {
            sendThreadSafeProgress(end - begin);
          }
          begin = end;
        }
      });
    }
    return {};
  };

  const bool usesOutOfCoreStore = in.getStoreType() == IDataStore::StoreType::OutOfCore || out.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(usesOutOfCoreStore && detail::ShouldUseContourResidentState(dims))
  {
    auto allocationResult = detail::ReserveContourResidentWorkingMemory<T>(dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        const usize volumeValues = sliceValues * dimZ;
        auto input = std::make_unique_for_overwrite<T[]>(volumeValues);
        auto output = std::make_unique_for_overwrite<T[]>(volumeValues);
        if(Result<> result = in.copyIntoBuffer(0, nonstd::span<T>(input.get(), volumeValues)); result.invalid())
        {
          return result;
        }
        if(Result<> result = runResident(nonstd::span<const T>(input.get(), volumeValues), nonstd::span<T>(output.get(), volumeValues)); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        return out.copyFromBuffer(0, nonstd::span<const T>(output.get(), volumeValues));
      } catch(const std::bad_alloc&)
      {
        // Release the complete-state reservation before entering the slab fallback.
      }
    }
  }

  progressThrottle.reset(sliceValues * dimZ, "Computing contour filter");
  if(dimZ == 0)
  {
    return {};
  }
  if(dimZ == 1)
  {
    const usize workerCount = detail::Contour2DWorkerCount();
    Result<> result =
        detail::ExecuteRadiusOneStencil2D<T, T>(in, out, dims, shouldCancel, target2DBytes, detail::Contour2DWorkerScratchBytes<T>(workerCount), workerCount,
                                                [&](const T* input, T* output, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
                                                  const auto body = detail::Contour2DBlockBody<T, PredicateT>{.input = input,
                                                                                                              .output = output,
                                                                                                              .offsets = neighborOffsets.data(),
                                                                                                              .num_offsets = neighborOffsets.size(),
                                                                                                              .dim_x = dimX,
                                                                                                              .dim_y = dimY,
                                                                                                              .input_x_begin = inputXBegin,
                                                                                                              .input_y_begin = inputYBegin,
                                                                                                              .input_width = inputWidth,
                                                                                                              .output_x_begin = outputXBegin,
                                                                                                              .output_y_begin = outputYBegin,
                                                                                                              .output_width = outputWidth,
                                                                                                              .predicate = predicate};
                                                  return [body, &sendThreadSafeProgress, &shouldCancel](const Range& range) {
                                                    constexpr usize k_ProgressBatchValues = 4096;
                                                    for(usize begin = range.min(); begin < range.max();)
                                                    {
                                                      const usize end = begin + std::min(k_ProgressBatchValues, range.max() - begin);
                                                      body(Range(begin, end));
                                                      if(!shouldCancel)
                                                      {
                                                        sendThreadSafeProgress(end - begin);
                                                      }
                                                      begin = end;
                                                    }
                                                  };
                                                });
    if(result.invalid())
    {
      return result;
    }
    return {};
  }

  // Size a bounded, working-memory-derived read-ahead band instead of a machine-fixed single-plane window: one
  // band is read (and, symmetrically, one band's worth of output is written) per store call, so a call presents
  // planesPerBand chunks instead of exactly one (a single-chunk call always takes the codec's fully serial
  // branch). The per-voxel compute stays the SAME radius-1 rolling window (ContourRollingPlaneBody, unchanged
  // below) -- only the I/O granularity changes. Two full input bands are held at once (the band being computed
  // and the following band, prefetched one band ahead so the last plane of a band always has its Z+1 neighbor
  // available) plus one output band and a single carried boundary plane, so the resident footprint is
  // (3 * planesPerBand + 1) planes; planesPerBand is solved from the granted bytes and degrades to 1 -- the same
  // four-plane footprint this window always used -- whenever the grant cannot hold even that much.
  usize planeBytes = 0;
  usize fullVolumeBytes = 0;
  if(!detail::ContourCheckedMultiply(sliceValues, sizeof(T), planeBytes) || !detail::ContourCheckedMultiply(planeBytes, dimZ, fullVolumeBytes))
  {
    return MakeErrorResult(-8770,
                           fmt::format("Contour dimensions ({}) and element size ({} bytes) overflow while sizing the streamed band buffers.", string_utilities::FormatDimensions3D(dims), sizeof(T)));
  }
  usize minimumBandBytes = 0;
  if(!detail::ContourCheckedMultiply(planeBytes, usize{4}, minimumBandBytes))
  {
    return MakeErrorResult(-8770,
                           fmt::format("Contour dimensions ({}) and element size ({} bytes) overflow while sizing the streamed band buffers.", string_utilities::FormatDimensions3D(dims), sizeof(T)));
  }
  const uint64 usefulBandBytes = ResolveWorkingMemoryFractionBytes(fullVolumeBytes, 1, 20, minimumBandBytes);
  auto bandReservation = ReserveWorkingMemory(usefulBandBytes, usefulBandBytes);
  usize planesPerBand = 1;
  if(bandReservation.sizeBytes() >= minimumBandBytes)
  {
    planesPerBand = std::max<usize>(1, std::min(dimZ, static_cast<usize>((bandReservation.sizeBytes() - planeBytes) / (3 * planeBytes))));
    usize grantedBandBytes = 0;
    if(detail::ContourCheckedMultiply(planeBytes, 3 * planesPerBand, grantedBandBytes))
    {
      grantedBandBytes += planeBytes; // the single carried boundary plane
      bandReservation.shrinkTo(grantedBandBytes);
    }
  }
  else
  {
    bandReservation.shrinkTo(0); // falls back to the same four-plane footprint this window always used
  }

  const std::array<std::unique_ptr<T[]>, 2> bandStorage = {std::make_unique_for_overwrite<T[]>(planesPerBand * sliceValues), std::make_unique_for_overwrite<T[]>(planesPerBand * sliceValues)};
  const auto outputBand = std::make_unique_for_overwrite<T[]>(planesPerBand * sliceValues);
  const auto previousPlaneCarry = std::make_unique_for_overwrite<T[]>(sliceValues);
  const auto loadBand = [&](usize bandFirstZ, usize bandPlaneCount, T* destination) -> Result<> {
    return in.copyIntoBuffer(bandFirstZ * sliceValues, nonstd::span<T>(destination, bandPlaneCount * sliceValues));
  };

  usize bandParity = 0;
  usize bandStart = 0;
  usize bandCount = std::min(planesPerBand, dimZ);
  if(Result<> result = loadBand(bandStart, bandCount, bandStorage[bandParity].get()); result.invalid())
  {
    return result;
  }
  if(shouldCancel)
  {
    return {};
  }
  bool hasPreviousCarry = false;

  while(true)
  {
    T* currentBandDataPtr = bandStorage[bandParity].get();
    const usize currentBandStart = bandStart;
    const usize currentBandCount = bandCount;
    const bool isLastBand = currentBandStart + currentBandCount >= dimZ;
    // Every non-final band's last plane needs the FOLLOWING band's first plane as its Z+1 neighbor, so its
    // computation is deferred until that band is loaded, below. The last band has no following plane at all, so
    // its final local index is computed here directly (hasNext becomes false for it, exactly as this window's
    // final iteration always reported).
    const usize computeCountNow = isLastBand ? currentBandCount : currentBandCount - 1;

    usize computed = 0;
    for(; computed < computeCountNow; ++computed)
    {
      if(shouldCancel)
      {
        break;
      }
      const usize z = currentBandStart + computed;
      const bool hasPrevious = z != 0;
      const bool hasNext = computed + 1 < currentBandCount;
      const T* previousPlanePtr = computed > 0 ? currentBandDataPtr + (computed - 1) * sliceValues : (hasPreviousCarry ? previousPlaneCarry.get() : nullptr);
      const T* nextPlanePtr = hasNext ? currentBandDataPtr + (computed + 1) * sliceValues : nullptr;
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, sliceValues);
      parallelAlgorithm.execute(detail::ContourRollingPlaneBody<T, PredicateT>{.previous_plane = previousPlanePtr,
                                                                               .current_plane = currentBandDataPtr + computed * sliceValues,
                                                                               .next_plane = nextPlanePtr,
                                                                               .out_plane = outputBand.get() + computed * sliceValues,
                                                                               .offsets = neighborOffsets.data(),
                                                                               .num_offsets = neighborOffsets.size(),
                                                                               .dim_x = dimX,
                                                                               .dim_y = dimY,
                                                                               .has_previous = hasPrevious,
                                                                               .has_next = hasNext,
                                                                               .predicate = predicate});
      reportCompletedPlane();
    }
    if(computed > 0)
    {
      if(Result<> result = out.copyFromBuffer(currentBandStart * sliceValues, nonstd::span<const T>(outputBand.get(), computed * sliceValues)); result.invalid())
      {
        return result;
      }
    }
    if(shouldCancel || isLastBand)
    {
      return {};
    }

    const usize nextBandStart = currentBandStart + currentBandCount;
    const usize nextBandCount = std::min(planesPerBand, dimZ - nextBandStart);
    const usize nextParity = 1 - bandParity;
    if(Result<> result = loadBand(nextBandStart, nextBandCount, bandStorage[nextParity].get()); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }

    // Compute and write the deferred last plane of the band just finished, now that its Z+1 neighbor (the new
    // band's first plane) is resident. Its Z-1 neighbor is either still resident in that band's own buffer, or,
    // for a single-plane band, the boundary plane carried from the band before it.
    const usize deferredLocal = currentBandCount - 1;
    const usize deferredZ = currentBandStart + deferredLocal;
    const T* deferredPreviousPtr = deferredLocal > 0 ? currentBandDataPtr + (deferredLocal - 1) * sliceValues : (hasPreviousCarry ? previousPlaneCarry.get() : nullptr);
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, sliceValues);
    parallelAlgorithm.execute(detail::ContourRollingPlaneBody<T, PredicateT>{.previous_plane = deferredPreviousPtr,
                                                                             .current_plane = currentBandDataPtr + deferredLocal * sliceValues,
                                                                             .next_plane = bandStorage[nextParity].get(),
                                                                             .out_plane = outputBand.get(),
                                                                             .offsets = neighborOffsets.data(),
                                                                             .num_offsets = neighborOffsets.size(),
                                                                             .dim_x = dimX,
                                                                             .dim_y = dimY,
                                                                             .has_previous = deferredZ != 0,
                                                                             .has_next = true,
                                                                             .predicate = predicate});
    if(shouldCancel)
    {
      return {};
    }
    reportCompletedPlane();
    if(Result<> result = out.copyFromBuffer(deferredZ * sliceValues, nonstd::span<const T>(outputBand.get(), sliceValues)); result.invalid())
    {
      return result;
    }

    std::copy_n(currentBandDataPtr + deferredLocal * sliceValues, sliceValues, previousPlaneCarry.get());
    hasPreviousCarry = true;
    bandParity = nextParity;
    bandStart = nextBandStart;
    bandCount = nextBandCount;
  }
}
} // namespace nx::core::ImageProcessing
