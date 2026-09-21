#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
// Integer sign matching itk::Math::sgn: +1 / -1, and exactly 0 at equality (so u==v contributes only the
// beta*u term of the cumulative function, exactly as ITK does).
inline int AdaptiveEqualizationSgn(float value)
{
  return (value != 0.0f) ? ((value > 0.0f) ? 1 : -1) : 0;
}

/**
 * @brief ParallelDataAlgorithm body that fills one output Z-plane of the Adaptive Histogram Equalization
 *        transform (Stark 2000). For each (x,y) it reads the center value from the already-read Z-slab, then
 *        accumulates the cumulative function over every IN-BOUNDS box-window neighbor (INCLUDING the center
 *        offset (0,0,0)), SKIPPING out-of-bounds neighbors and dividing by the in-bounds count -- i.e. it
 *        over-weights the valid part of the window, exactly as ITK's moving-histogram base does. The center
 *        pixel is deliberately double-counted (once as the transform subject @c u, once as one neighbor
 *        value @c v), matching ITK.
 *
 * u/v/cf are computed in float and the running sum/scale in double. This is CLOSE TO but not identical to ITK,
 * which evaluates the cumulative function itself in double (its 0.5 / beta*0.5 literals are double) and narrows
 * to float only on return, whereas our whole @c cf is float; the resulting ~iscale*1e-6 difference is well within
 * the tolerant parity comparison. The engine guarantees @c iscale > 0 (the constant-image case is handled in
 * @ref ApplyAdaptiveHistogramEqualization before this body runs), and the center offset is always in bounds, so
 * @c inBounds >= 1 and there is no division by zero. Stateless and side-effect-free apart from writing distinct
 * @c outPlane slots, so it is safe to run concurrently across the plane.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
struct AdaptiveHistogramEqualizationPlaneBody
{
  const T* slab; // read-only slab covering Z in [zLo, zHi], laid out [slabZ][y][x]
  T* outPlane;   // output plane, dimX*dimY
  usize dimX;
  usize dimY;
  usize dimZ;
  usize rx;
  usize ry;
  usize rz;
  usize zLo;        // slab's first Z index
  usize z;          // output plane's Z index
  float64 minValue; // global image minimum (as double)
  float64 iscale;   // global (max - min) as double, > 0
  float32 alpha;
  float32 beta;

  void operator()(const Range& range) const
  {
    const usize sliceValues = dimX * dimY;
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    const int64 dimZi = static_cast<int64>(dimZ);
    const int64 rxi = static_cast<int64>(rx);
    const int64 ryi = static_cast<int64>(ry);
    const int64 rzi = static_cast<int64>(rz);

    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize x = tuple % dimX;
      const usize y = tuple / dimX;
      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);
      const int64 zi = static_cast<int64>(z);

      const usize centerSlab = (z - zLo) * sliceValues + y * dimX + x;
      const float u = static_cast<float>((static_cast<float64>(slab[centerSlab]) - minValue) / iscale - 0.5);
      // Loop-invariant terms of the cumulative function (u is fixed for this voxel). Hoisting is arithmetically
      // identical to the per-neighbor form (same multiply, same left-to-right association); any few-ULP
      // FMA-contraction drift is absorbed by the tolerant comparator. Trims the hot loop.
      const float betaU = beta * u;
      const float betaHalf = beta * 0.5f;

      float64 localSum = 0.0;
      usize inBounds = 0;
      for(int64 dz = -rzi; dz <= rzi; ++dz)
      {
        const int64 nz = zi + dz;
        if(nz < 0 || nz >= dimZi)
        {
          continue; // OOB neighbor ignored (over-weight the valid part; see struct docs)
        }
        const usize slabZBase = (static_cast<usize>(nz) - zLo) * sliceValues; // nz in [zLo, zHi] once in-bounds
        for(int64 dy = -ryi; dy <= ryi; ++dy)
        {
          const int64 ny = yi + dy;
          if(ny < 0 || ny >= dimYi)
          {
            continue;
          }
          const usize rowBase = slabZBase + static_cast<usize>(ny) * dimX;
          for(int64 dx = -rxi; dx <= rxi; ++dx)
          {
            const int64 nx = xi + dx;
            if(nx < 0 || nx >= dimXi)
            {
              continue;
            }
            const float v = static_cast<float>((static_cast<float64>(slab[rowBase + static_cast<usize>(nx)]) - minValue) / iscale - 0.5);
            const float sF = static_cast<float>(AdaptiveEqualizationSgn(u - v));
            const float ad = static_cast<float>(std::fabs(2.0 * static_cast<double>(u - v)));
            const float cf = 0.5f * sF * std::pow(ad, alpha) - betaHalf * sF * ad + betaU;
            localSum += static_cast<float64>(cf);
            ++inBounds;
          }
        }
      }
      const float64 result = iscale * (localSum / static_cast<float64>(inBounds) + 0.5) + minValue;
      outPlane[tuple] = static_cast<T>(result);
    }
  }
};

/**
 * @brief Integer bounded-range fast path for @ref AdaptiveHistogramEqualizationPlaneBody. This body is a
 *        near-duplicate of that one: the slab indexing, out-of-bounds skip, in-bounds count, deliberate
 *        center double-count, and the final rescale are kept BYTE-IDENTICAL and the two MUST stay in sync
 *        (any change to the boundary rule in one has to be mirrored in the other). The ONLY difference is the
 *        inner cumulative-function term: instead of recomputing @c std::pow per neighbor, the neighbor term
 *        @c g(d) = 0.5*sgn*pow(|2d/iscale|,alpha) - 0.5*beta*sgn*|2d/iscale| is read from a table keyed by the
 *        integer difference @c d = Vc - Vn. Because the normalization is affine, @c u-v == (Vc-Vn)/iscale in
 *        exact arithmetic, so @c g depends only on @c d; the LUT is built once (see @ref
 *        ApplyAdaptiveHistogramEqualization). The one numeric deviation vs the sibling body is that @c |2d/iscale|
 *        is formed from the exact integer difference rather than the twice-float-rounded @c (u-v) -- a ~1e-5-level
 *        drift, far below the integer-flip threshold and absorbed by the tolerant comparator. The center-only
 *        term @c beta*u still varies per voxel, so it is added per neighbor here (kept out of the LUT).
 *
 * @tparam T integral voxel type.
 */
template <class T>
struct AdaptiveHistogramEqualizationLutPlaneBody
{
  const T* slab; // read-only slab covering Z in [zLo, zHi], laid out [slabZ][y][x]
  T* outPlane;   // output plane, dimX*dimY
  usize dimX;
  usize dimY;
  usize dimZ;
  usize rx;
  usize ry;
  usize rz;
  usize zLo;         // slab's first Z index
  usize z;           // output plane's Z index
  float64 minValue;  // global image minimum (as double)
  float64 iscale;    // global (max - min) as double, > 0
  float32 beta;      // alpha is baked into gLut; only beta's per-voxel term survives here
  const float* gLut; // difference-keyed neighbor term g(d), read-only, shared across the plane
  int64 dMin;        // = -span; the LUT index for difference d is (d - dMin), in [0, 2*span]
  usize lutSize;     // gLut length (2*span + 1); used only by the bounds assert

  void operator()(const Range& range) const
  {
    const usize sliceValues = dimX * dimY;
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    const int64 dimZi = static_cast<int64>(dimZ);
    const int64 rxi = static_cast<int64>(rx);
    const int64 ryi = static_cast<int64>(ry);
    const int64 rzi = static_cast<int64>(rz);

    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize x = tuple % dimX;
      const usize y = tuple / dimX;
      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);
      const int64 zi = static_cast<int64>(z);

      const usize centerSlab = (z - zLo) * sliceValues + y * dimX + x;
      const T centerValue = slab[centerSlab];
      const float u = static_cast<float>((static_cast<float64>(centerValue) - minValue) / iscale - 0.5);
      // Center-only cumulative-function term (independent of the neighbor difference); folded into every
      // neighbor contribution below, exactly as the sibling body's per-voxel betaU.
      const float betaU = beta * u;

      float64 localSum = 0.0;
      usize inBounds = 0;
      for(int64 dz = -rzi; dz <= rzi; ++dz)
      {
        const int64 nz = zi + dz;
        if(nz < 0 || nz >= dimZi)
        {
          continue; // OOB neighbor ignored (over-weight the valid part; see struct docs)
        }
        const usize slabZBase = (static_cast<usize>(nz) - zLo) * sliceValues; // nz in [zLo, zHi] once in-bounds
        for(int64 dy = -ryi; dy <= ryi; ++dy)
        {
          const int64 ny = yi + dy;
          if(ny < 0 || ny >= dimYi)
          {
            continue;
          }
          const usize rowBase = slabZBase + static_cast<usize>(ny) * dimX;
          for(int64 dx = -rxi; dx <= rxi; ++dx)
          {
            const int64 nx = xi + dx;
            if(nx < 0 || nx >= dimXi)
            {
              continue;
            }
            // Promote to int64 BEFORE subtracting: for unsigned T a T - T subtraction would underflow. The
            // gate in ApplyAdaptiveHistogramEqualization guarantees this difference does not overflow int64.
            const int64 d = static_cast<int64>(centerValue) - static_cast<int64>(slab[rowBase + static_cast<usize>(nx)]);
            const usize lutIndex = static_cast<usize>(d - dMin); // in [0, 2*span] by construction
            assert(lutIndex < lutSize);
            localSum += static_cast<float64>(gLut[lutIndex] + betaU);
            ++inBounds;
          }
        }
      }
      const float64 result = iscale * (localSum / static_cast<float64>(inBounds) + 0.5) + minValue;
      outPlane[tuple] = static_cast<T>(result);
    }
  }
};

inline constexpr usize k_AdaptiveHistogramEqualization2DTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_AdaptiveHistogramEqualization2DFixedOutputBytes = 48ULL * 1024ULL * 1024ULL;
inline constexpr usize k_AdaptiveHistogramEqualizationStatisticsTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_AdaptiveHistogramEqualizationMaximumLutEntries = usize{1} << 22;

struct AdaptiveHistogramEqualization2DPlan
{
  bool fullWidth = false;
  bool fixedOutput = false;
  usize totalValues = 0;
  usize coreRows = 0;
  usize coreColumns = 0;
  usize maximumInputRows = 0;
  usize maximumInputColumns = 0;
  usize inputBufferValues = 0;
  usize horizontalBufferValues = 0;
  usize outputBufferValues = 0;
  usize residentBytes = 0;
};

inline bool TryMultiplyAdaptiveHistogramSize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool TryAddAdaptiveHistogramSize(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

struct AdaptiveHistogramEqualizationResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseAdaptiveHistogramEqualizationResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateAdaptiveHistogramEqualizationResidentWorkingMemoryBytes(const SizeVec3& dims, const std::array<usize, 3>& radius, bool useLinearUint8)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }
  if(useLinearUint8 && !std::is_same_v<T, uint8>)
  {
    return MakeErrorResult<usize>(-8766, "Adaptive histogram equalization linear resident-state sizing is only valid for uint8 input.");
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize volumeBytes = 0;
  usize stagedBytes = 0;
  usize outputPlaneBytes = 0;
  usize lookupBytes = 0;
  if(!TryMultiplyAdaptiveHistogramSize(dims[0], dims[1], sliceValues) || !TryMultiplyAdaptiveHistogramSize(sliceValues, dims[2], volumeValues) ||
     !TryMultiplyAdaptiveHistogramSize(volumeValues, sizeof(T), volumeBytes) || !TryMultiplyAdaptiveHistogramSize(volumeBytes, usize{2}, stagedBytes) ||
     !TryMultiplyAdaptiveHistogramSize(sliceValues, sizeof(T), outputPlaneBytes))
  {
    return MakeErrorResult<usize>(-8766, fmt::format("Adaptive histogram equalization dimensions ({}) and {}-byte input values overflow while sizing the resident volume and plane state.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  if constexpr(std::is_integral_v<T>)
  {
    if(!TryMultiplyAdaptiveHistogramSize(k_AdaptiveHistogramEqualizationMaximumLutEntries, sizeof(float), lookupBytes))
    {
      return MakeErrorResult<usize>(-8766, "Adaptive histogram equalization lookup-table byte sizing overflowed.");
    }
  }

  usize bufferDepth = dims[2];
  if(radius[2] < dims[2] / 2)
  {
    if(!TryMultiplyAdaptiveHistogramSize(radius[2], usize{2}, bufferDepth) || !TryAddAdaptiveHistogramSize(bufferDepth, usize{1}, bufferDepth))
    {
      return MakeErrorResult<usize>(-8766, fmt::format("Adaptive histogram equalization Z radius ({}) overflows while sizing resident execution buffers for dimensions {}.", radius[2],
                                                       StringUtilities::formatDimensions3D(dims)));
    }
  }

  usize executionBytes = 0;
  if(useLinearUint8)
  {
    usize ringValues = 0;
    usize sourceBytes = 0;
    usize xySumBytes = 0;
    usize aggregateSumBytes = 0;
    if(!TryMultiplyAdaptiveHistogramSize(bufferDepth, sliceValues, ringValues) || !TryMultiplyAdaptiveHistogramSize(ringValues, sizeof(uint8), sourceBytes) ||
       !TryMultiplyAdaptiveHistogramSize(ringValues, sizeof(uint64), xySumBytes) || !TryMultiplyAdaptiveHistogramSize(sliceValues, 2 * sizeof(uint64), aggregateSumBytes) ||
       !TryAddAdaptiveHistogramSize(outputPlaneBytes, sourceBytes, executionBytes) || !TryAddAdaptiveHistogramSize(executionBytes, xySumBytes, executionBytes) ||
       !TryAddAdaptiveHistogramSize(executionBytes, aggregateSumBytes, executionBytes) || !TryAddAdaptiveHistogramSize(executionBytes, lookupBytes, executionBytes))
    {
      return MakeErrorResult<usize>(-8766, fmt::format("Adaptive histogram equalization dimensions ({}) and radius {} overflow while sizing resident linear execution buffers.",
                                                       StringUtilities::formatDimensions3D(dims), radius[2]));
    }
  }
  else
  {
    usize slabValues = 0;
    usize slabBytes = 0;
    if(!TryMultiplyAdaptiveHistogramSize(bufferDepth, sliceValues, slabValues) || !TryMultiplyAdaptiveHistogramSize(slabValues, sizeof(T), slabBytes) ||
       !TryAddAdaptiveHistogramSize(outputPlaneBytes, slabBytes, executionBytes) || !TryAddAdaptiveHistogramSize(executionBytes, lookupBytes, executionBytes))
    {
      return MakeErrorResult<usize>(-8766, fmt::format("Adaptive histogram equalization dimensions ({}) and radius {} overflow while sizing resident slab execution buffers.",
                                                       StringUtilities::formatDimensions3D(dims), radius[2]));
    }
  }

  const usize statisticsBytes = std::min(volumeBytes, k_AdaptiveHistogramEqualizationStatisticsTargetBytes);
  usize requiredBytes = 0;
  if(!TryAddAdaptiveHistogramSize(stagedBytes, statisticsBytes, requiredBytes) || !TryAddAdaptiveHistogramSize(requiredBytes, executionBytes, requiredBytes))
  {
    return MakeErrorResult<usize>(
        -8766, fmt::format("Adaptive histogram equalization dimensions ({}) overflow while totaling resident staged, statistics, and execution state.", StringUtilities::formatDimensions3D(dims)));
  }
  return {requiredBytes};
}

template <class T>
Result<AdaptiveHistogramEqualizationResidentMemoryAllocation> ReserveAdaptiveHistogramEqualizationResidentWorkingMemory(const SizeVec3& dims, const std::array<usize, 3>& radius, bool useLinearUint8)
{
  auto requiredResult = CalculateAdaptiveHistogramEqualizationResidentWorkingMemoryBytes<T>(dims, radius, useLinearUint8);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<AdaptiveHistogramEqualizationResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {AdaptiveHistogramEqualizationResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

/**
 * @brief Working-memory plan for the linear uint8 Z-ring's band-batched store transfers (see @ref
 *        ApplyAdaptiveHistogramEqualization). @c planesPerBand is the number of consecutive Z-planes read or
 *        written per store call: batching several chunks into one call lets a chunked out-of-core store's
 *        parallel codec engage instead of taking its single-chunk serial path.
 */
struct AdaptiveHistogramEqualizationRingBandPlan
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation; // kept alive for the ring's lifetime; backs planesPerBand's extra buffers
  usize planesPerBand = 1;                                        // consecutive Z-planes per band read/write
};

/**
 * @brief Reserves working memory for the linear uint8 Z-ring's band-batched store transfers and derives
 *        @c planesPerBand from the grant.
 *
 * Each additional unit of band depth costs one extra raw-plane ring slot plus one extra output-band slot, i.e.
 * @c 2*sliceValues bytes. The "useful" request is sized to cover the whole volume twice over -- the largest
 * band depth that could ever be useful -- so it is the grant returned by @ref ReserveWorkingMemory, clamped by
 * CacheMemoryBudgetManager's aggregate working-memory ceiling, that decides @c planesPerBand, never a machine-
 * or dataset-size-tuned constant. The result is never less than 1 (one-plane store calls, the same call
 * granularity the engine used before band batching) and never more than @p dimZ (a band wider than the dataset
 * has no further benefit).
 *
 * @param sliceValues Voxel count of one Z-plane (dimX * dimY).
 * @param dimZ Total Z-plane count.
 */
inline Result<AdaptiveHistogramEqualizationRingBandPlan> ReserveAdaptiveHistogramEqualizationRingBand(usize sliceValues, usize dimZ)
{
  usize perBandUnitBytes = 0;
  usize usefulBytes = 0;
  if(!TryMultiplyAdaptiveHistogramSize(sliceValues, usize{2}, perBandUnitBytes) || !TryMultiplyAdaptiveHistogramSize(perBandUnitBytes, std::max<usize>(dimZ, usize{1}), usefulBytes))
  {
    return MakeErrorResult<AdaptiveHistogramEqualizationRingBandPlan>(
        -8768, fmt::format("Adaptive histogram equalization ring-band sizing overflowed for {} plane values across {} Z planes.", sliceValues, dimZ));
  }
  auto reservation = ReserveWorkingMemory(usefulBytes, usefulBytes);
  usize planesPerBand = perBandUnitBytes == 0 ? usize{1} : std::max<usize>(usize{1}, reservation.sizeBytes() / perBandUnitBytes);
  planesPerBand = std::min(planesPerBand, std::max<usize>(dimZ, usize{1}));
  return {AdaptiveHistogramEqualizationRingBandPlan{std::move(reservation), planesPerBand}};
}

inline bool TryComputeAdaptiveHistogramEqualization2DLayout(usize inputRows, usize inputColumns, usize outputColumns, usize valueBytes, usize outputAllocationValues, usize fixedScratchBytes,
                                                            bool useHorizontalSums, usize& inputValues, usize& horizontalValues, usize& residentBytes)
{
  usize inputBytes = 0;
  usize horizontalBytes = 0;
  usize outputBytes = 0;
  usize bufferBytes = 0;
  horizontalValues = 0;
  if(!TryMultiplyAdaptiveHistogramSize(inputRows, inputColumns, inputValues) || !TryMultiplyAdaptiveHistogramSize(inputValues, valueBytes, inputBytes) ||
     !TryMultiplyAdaptiveHistogramSize(outputAllocationValues, valueBytes, outputBytes))
  {
    return false;
  }
  if(useHorizontalSums && (!TryMultiplyAdaptiveHistogramSize(inputRows, outputColumns, horizontalValues) || !TryMultiplyAdaptiveHistogramSize(horizontalValues, sizeof(uint64), horizontalBytes)))
  {
    return false;
  }
  return TryAddAdaptiveHistogramSize(inputBytes, horizontalBytes, bufferBytes) && TryAddAdaptiveHistogramSize(bufferBytes, outputBytes, bufferBytes) &&
         TryAddAdaptiveHistogramSize(bufferBytes, fixedScratchBytes, residentBytes);
}

inline Result<AdaptiveHistogramEqualization2DPlan> CreateAdaptiveHistogramEqualization2DPlan(usize dimX, usize dimY, usize radiusX, usize radiusY, usize valueBytes, usize targetBytes,
                                                                                             usize fixedScratchBytes, bool useHorizontalSums, bool preferFixedOutput = false)
{
  if(dimX == 0 || dimY == 0 || valueBytes == 0)
  {
    return MakeErrorResult<AdaptiveHistogramEqualization2DPlan>(
        -8545, fmt::format("Adaptive histogram equalization 2D dimensions and value size must be nonzero. Dimensions: {} x {}; value size: {} bytes.", dimX, dimY, valueBytes));
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dimX > k_Int64Max || dimY > k_Int64Max || radiusX > k_Int64Max || radiusY > k_Int64Max)
  {
    return MakeErrorResult<AdaptiveHistogramEqualization2DPlan>(
        -8545,
        fmt::format("Adaptive histogram equalization 2D dimensions or radius exceed the supported signed coordinate range. Dimensions: {} x {}; radius: {} x {}.", dimX, dimY, radiusX, radiusY));
  }

  AdaptiveHistogramEqualization2DPlan plan;
  if(!TryMultiplyAdaptiveHistogramSize(dimX, dimY, plan.totalValues))
  {
    return MakeErrorResult<AdaptiveHistogramEqualization2DPlan>(-8545,
                                                                fmt::format("Adaptive histogram equalization 2D dimensions overflow the addressable value count. Dimensions: {} x {}.", dimX, dimY));
  }
  usize haloX = 0;
  usize haloY = 0;
  if(!TryMultiplyAdaptiveHistogramSize(radiusX, 2, haloX) || !TryMultiplyAdaptiveHistogramSize(radiusY, 2, haloY))
  {
    return MakeErrorResult<AdaptiveHistogramEqualization2DPlan>(-8545, fmt::format("Adaptive histogram equalization 2D radius overflows the halo size. Radius: {} x {}.", radiusX, radiusY));
  }
  const auto maximumInputExtent = [](usize coreExtent, usize haloExtent, usize dimension) { return coreExtent + std::min(haloExtent, dimension - coreExtent); };

  const usize fixedOutputCapacityValues = k_AdaptiveHistogramEqualization2DFixedOutputBytes / valueBytes;
  auto selectRoute = [&](bool fixedOutput) {
    const usize fixedOutputValues = fixedOutput ? fixedOutputCapacityValues : 0;
    auto layoutFits = [&](usize coreRows, usize coreColumns, bool fullWidth) {
      usize outputValues = 0;
      usize inputValues = 0;
      usize horizontalValues = 0;
      usize residentBytes = 0;
      if(!TryMultiplyAdaptiveHistogramSize(coreRows, coreColumns, outputValues))
      {
        return false;
      }
      const usize outputAllocationValues = fixedOutput ? fixedOutputValues : outputValues;
      const usize inputRows = fullWidth ? maximumInputExtent(coreRows, haloY, dimY) : maximumInputExtent(1, haloY, dimY);
      const usize inputColumns = fullWidth ? dimX : maximumInputExtent(coreColumns, haloX, dimX);
      return TryComputeAdaptiveHistogramEqualization2DLayout(inputRows, inputColumns, coreColumns, valueBytes, outputAllocationValues, fixedScratchBytes, useHorizontalSums, inputValues,
                                                             horizontalValues, residentBytes) &&
             residentBytes <= targetBytes;
    };

    if(layoutFits(1, dimX, true))
    {
      usize lower = 1;
      usize upper = dimY;
      while(lower < upper)
      {
        const usize midpoint = lower + (upper - lower) / 2 + 1;
        if(layoutFits(midpoint, dimX, true))
        {
          lower = midpoint;
        }
        else
        {
          upper = midpoint - 1;
        }
      }
      plan.fullWidth = true;
      plan.coreRows = lower;
      plan.coreColumns = dimX;
    }
    else if(layoutFits(1, 1, false))
    {
      usize lower = 1;
      usize upper = dimX;
      while(lower < upper)
      {
        const usize midpoint = lower + (upper - lower) / 2 + 1;
        if(layoutFits(1, midpoint, false))
        {
          lower = midpoint;
        }
        else
        {
          upper = midpoint - 1;
        }
      }
      plan.fullWidth = false;
      plan.coreRows = 1;
      plan.coreColumns = lower;
    }
    else
    {
      return false;
    }

    plan.fixedOutput = fixedOutput;
    plan.maximumInputRows = plan.fullWidth ? maximumInputExtent(plan.coreRows, haloY, dimY) : maximumInputExtent(1, haloY, dimY);
    plan.maximumInputColumns = plan.fullWidth ? dimX : maximumInputExtent(plan.coreColumns, haloX, dimX);
    usize outputValues = 0;
    if(!TryMultiplyAdaptiveHistogramSize(plan.coreRows, plan.coreColumns, outputValues))
    {
      return false;
    }
    plan.outputBufferValues = fixedOutput ? fixedOutputValues : outputValues;
    return TryComputeAdaptiveHistogramEqualization2DLayout(plan.maximumInputRows, plan.maximumInputColumns, plan.coreColumns, valueBytes, plan.outputBufferValues, fixedScratchBytes, useHorizontalSums,
                                                           plan.inputBufferValues, plan.horizontalBufferValues, plan.residentBytes);
  };

  const bool fixedOutputEligible = preferFixedOutput && targetBytes == k_AdaptiveHistogramEqualization2DTargetBytes && plan.totalValues <= fixedOutputCapacityValues;
  if(fixedOutputEligible && selectRoute(true))
  {
    return {plan};
  }
  plan = {};
  if(!TryMultiplyAdaptiveHistogramSize(dimX, dimY, plan.totalValues) || !selectRoute(false))
  {
    return MakeErrorResult<AdaptiveHistogramEqualization2DPlan>(-8545, fmt::format("Adaptive histogram equalization 2D target ({} bytes) cannot hold one output value, its clipped {} x {} halo, {} "
                                                                                   "bytes of fixed scratch, and the selected local sums for dimensions {} x {} and value size {} bytes.",
                                                                                   targetBytes, radiusX, radiusY, fixedScratchBytes, dimX, dimY, valueBytes));
  }
  return {plan};
}

template <class T, class BlockFunction>
Result<> ExecuteAdaptiveHistogramEqualization2D(const AbstractDataStore<T>& inputStore, AbstractDataStore<T>& outputStore, const SizeVec3& dims, const std::array<usize, 3>& radius,
                                                const AdaptiveHistogramEqualization2DPlan& plan, const std::atomic_bool& shouldCancel, BlockFunction&& blockFunction)
{
  auto inputBuffer = std::make_unique<T[]>(plan.inputBufferValues);
  std::unique_ptr<uint64[]> horizontalBuffer;
  if(plan.horizontalBufferValues > 0)
  {
    horizontalBuffer = std::make_unique<uint64[]>(plan.horizontalBufferValues);
  }
  auto outputBuffer = std::make_unique<T[]>(plan.outputBufferValues);
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize yStep = plan.fullWidth ? plan.coreRows : 1;
  const usize xStep = plan.fullWidth ? dimX : plan.coreColumns;
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
      const usize inputRows = inputYEnd - inputYBegin;
      if(plan.fullWidth)
      {
        const usize inputValues = inputRows * dimX;
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

      const usize outputStart = outputYBegin * dimX + outputXBegin;
      T* blockOutput = plan.fixedOutput ? outputBuffer.get() + outputStart : outputBuffer.get();
      blockFunction(inputBuffer.get(), blockOutput, horizontalBuffer.get(), inputXBegin, inputYBegin, inputWidth, inputRows, outputXBegin, outputYBegin, outputColumns, outputRows);
      if(shouldCancel)
      {
        return {};
      }
      if(!plan.fixedOutput)
      {
        const usize outputValues = plan.fullWidth ? outputRows * dimX : outputColumns;
        if(Result<> result = outputStore.copyFromBuffer(outputStart, nonstd::span<const T>(outputBuffer.get(), outputValues)); result.invalid())
        {
          return result;
        }
      }
    }
  }
  if(plan.fixedOutput)
  {
    if(shouldCancel)
    {
      return {};
    }
    return outputStore.copyFromBuffer(0, nonstd::span<const T>(outputBuffer.get(), plan.totalValues));
  }
  return {};
}

template <class T>
struct AdaptiveHistogramEqualization2DBlockBody
{
  const T* input;
  T* output;
  usize dimX;
  usize dimY;
  usize radiusX;
  usize radiusY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;
  float64 minValue;
  float64 iscale;
  float32 alpha;
  float32 beta;
  const std::atomic_bool& shouldCancel;

  void operator()(const Range& range) const
  {
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    const int64 radiusXi = static_cast<int64>(radiusX);
    const int64 radiusYi = static_cast<int64>(radiusY);
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      if(((tuple - range.min()) & 4095ULL) == 0 && shouldCancel)
      {
        return;
      }
      const usize localX = tuple % outputWidth;
      const usize localY = tuple / outputWidth;
      const usize x = outputXBegin + localX;
      const usize y = outputYBegin + localY;
      const T centerValue = input[(y - inputYBegin) * inputWidth + x - inputXBegin];
      const float u = static_cast<float>((static_cast<float64>(centerValue) - minValue) / iscale - 0.5);
      const float betaU = beta * u;
      const float betaHalf = beta * 0.5f;
      float64 localSum = 0.0;
      usize inBounds = 0;
      for(int64 dy = -radiusYi; dy <= radiusYi; ++dy)
      {
        const int64 neighborY = static_cast<int64>(y) + dy;
        if(neighborY < 0 || neighborY >= dimYi)
        {
          continue;
        }
        const usize inputRow = (static_cast<usize>(neighborY) - inputYBegin) * inputWidth;
        for(int64 dx = -radiusXi; dx <= radiusXi; ++dx)
        {
          const int64 neighborX = static_cast<int64>(x) + dx;
          if(neighborX < 0 || neighborX >= dimXi)
          {
            continue;
          }
          const float v = static_cast<float>((static_cast<float64>(input[inputRow + static_cast<usize>(neighborX) - inputXBegin]) - minValue) / iscale - 0.5);
          const float sign = static_cast<float>(AdaptiveEqualizationSgn(u - v));
          const float absoluteDifference = static_cast<float>(std::fabs(2.0 * static_cast<double>(u - v)));
          const float cumulative = 0.5f * sign * std::pow(absoluteDifference, alpha) - betaHalf * sign * absoluteDifference + betaU;
          localSum += static_cast<float64>(cumulative);
          ++inBounds;
        }
      }
      const float64 result = iscale * (localSum / static_cast<float64>(inBounds) + 0.5) + minValue;
      output[tuple] = static_cast<T>(result);
    }
  }
};

template <class T>
struct AdaptiveHistogramEqualizationLut2DBlockBody
{
  const T* input;
  T* output;
  usize dimX;
  usize dimY;
  usize radiusX;
  usize radiusY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;
  float64 minValue;
  float64 iscale;
  float32 beta;
  const float* lookupTable;
  int64 differenceMinimum;
  usize lookupTableSize;
  const std::atomic_bool& shouldCancel;

  void operator()(const Range& range) const
  {
    const int64 dimXi = static_cast<int64>(dimX);
    const int64 dimYi = static_cast<int64>(dimY);
    const int64 radiusXi = static_cast<int64>(radiusX);
    const int64 radiusYi = static_cast<int64>(radiusY);
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      if(((tuple - range.min()) & 4095ULL) == 0 && shouldCancel)
      {
        return;
      }
      const usize localX = tuple % outputWidth;
      const usize localY = tuple / outputWidth;
      const usize x = outputXBegin + localX;
      const usize y = outputYBegin + localY;
      const T centerValue = input[(y - inputYBegin) * inputWidth + x - inputXBegin];
      const float u = static_cast<float>((static_cast<float64>(centerValue) - minValue) / iscale - 0.5);
      const float betaU = beta * u;
      float64 localSum = 0.0;
      usize inBounds = 0;
      for(int64 dy = -radiusYi; dy <= radiusYi; ++dy)
      {
        const int64 neighborY = static_cast<int64>(y) + dy;
        if(neighborY < 0 || neighborY >= dimYi)
        {
          continue;
        }
        const usize inputRow = (static_cast<usize>(neighborY) - inputYBegin) * inputWidth;
        for(int64 dx = -radiusXi; dx <= radiusXi; ++dx)
        {
          const int64 neighborX = static_cast<int64>(x) + dx;
          if(neighborX < 0 || neighborX >= dimXi)
          {
            continue;
          }
          const int64 difference = static_cast<int64>(centerValue) - static_cast<int64>(input[inputRow + static_cast<usize>(neighborX) - inputXBegin]);
          const usize lookupIndex = static_cast<usize>(difference - differenceMinimum);
          assert(lookupIndex < lookupTableSize);
          localSum += static_cast<float64>(lookupTable[lookupIndex] + betaU);
          ++inBounds;
        }
      }
      const float64 result = iscale * (localSum / static_cast<float64>(inBounds) + 0.5) + minValue;
      output[tuple] = static_cast<T>(result);
    }
  }
};

inline void CalculateAdaptiveHistogramEqualizationLinear2DBlock(const uint8* input, uint8* output, uint64* horizontalSums, usize dimX, usize dimY, usize radiusX, usize radiusY, usize inputXBegin,
                                                                usize inputYBegin, usize inputWidth, usize inputRows, usize outputXBegin, usize outputYBegin, usize outputWidth, usize outputRows,
                                                                float64 minValue, float64 iscale, float32 beta, float32 unitDifferenceTerm, const std::atomic_bool& shouldCancel)
{
  auto calculateHorizontalSums = [&](const Range& range) {
    for(usize inputRow = range.min(); inputRow < range.max(); ++inputRow)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize rowOffset = inputRow * inputWidth;
      const usize initialXEnd = outputXBegin + std::min(radiusX, dimX - outputXBegin - 1);
      uint64 sum = 0;
      for(usize sourceX = inputXBegin; sourceX <= initialXEnd; ++sourceX)
      {
        sum += input[rowOffset + sourceX - inputXBegin];
      }
      for(usize localX = 0; localX < outputWidth; ++localX)
      {
        const usize x = outputXBegin + localX;
        horizontalSums[inputRow * outputWidth + localX] = sum;
        if(localX + 1 < outputWidth && x >= radiusX)
        {
          sum -= input[rowOffset + x - radiusX - inputXBegin];
        }
        if(localX + 1 < outputWidth && radiusX < dimX - x - 1)
        {
          sum += input[rowOffset + x + radiusX + 1 - inputXBegin];
        }
      }
    }
  };
  ParallelDataAlgorithm horizontalAlgorithm;
  horizontalAlgorithm.setRange(0, inputRows);
  horizontalAlgorithm.execute(calculateHorizontalSums);
  if(shouldCancel)
  {
    return;
  }

  auto calculateOutput = [&](const Range& range) {
    for(usize localX = range.min(); localX < range.max(); ++localX)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize x = outputXBegin + localX;
      const usize initialYEnd = outputYBegin + std::min(radiusY, dimY - outputYBegin - 1);
      uint64 boxSum = 0;
      for(usize sourceY = inputYBegin; sourceY <= initialYEnd; ++sourceY)
      {
        boxSum += horizontalSums[(sourceY - inputYBegin) * outputWidth + localX];
      }
      const usize xBegin = x > radiusX ? x - radiusX : 0;
      const usize xEnd = x + std::min(radiusX, dimX - x - 1);
      const usize xCount = xEnd - xBegin + 1;
      for(usize localY = 0; localY < outputRows; ++localY)
      {
        const usize y = outputYBegin + localY;
        const usize yBegin = y > radiusY ? y - radiusY : 0;
        const usize yEnd = y + std::min(radiusY, dimY - y - 1);
        const usize count = xCount * (yEnd - yBegin + 1);
        const uint8 center = input[(y - inputYBegin) * inputWidth + x - inputXBegin];
        const float u = static_cast<float>((static_cast<float64>(center) - minValue) / iscale - 0.5);
        const int64 differenceSum = static_cast<int64>(center) * static_cast<int64>(count) - static_cast<int64>(boxSum);
        const float64 localSum = static_cast<float64>(unitDifferenceTerm) * static_cast<float64>(differenceSum) + static_cast<float64>(beta * u) * count;
        const float64 result = iscale * (localSum / static_cast<float64>(count) + 0.5) + minValue;
        output[localY * outputWidth + localX] = static_cast<uint8>(result);
        if(localY + 1 < outputRows && y >= radiusY)
        {
          boxSum -= horizontalSums[(y - radiusY - inputYBegin) * outputWidth + localX];
        }
        if(localY + 1 < outputRows && radiusY < dimY - y - 1)
        {
          boxSum += horizontalSums[(y + radiusY + 1 - inputYBegin) * outputWidth + localX];
        }
      }
    }
  };
  ParallelDataAlgorithm outputAlgorithm;
  outputAlgorithm.setRange(0, outputWidth);
  outputAlgorithm.execute(calculateOutput);
}
} // namespace detail

/**
 * @brief Applies ITK-faithful Adaptive Histogram Equalization (Stark 2000) to a scalar image, out-of-core
 *        safe. A single global-min/max pre-pass (@ref ComputeArrayMinMax) fixes the gray range used to
 *        normalize every pixel to [-0.5, 0.5] (ITK uses the whole-image min/max, NOT a per-window range).
 *        Then, per output Z-plane, the clamped Z-slab [z-rz, z+rz] is bulk-read and each voxel's box-window
 *        cumulative-function transform is computed in parallel (see @ref
 *        detail::AdaptiveHistogramEqualizationPlaneBody). Out-of-bounds neighbors are skipped and the valid
 *        part over-weighted, matching ITK's moving-histogram boundary rule. For a 2D image (dimZ == 1), the
 *        z-neighbors are all out of bounds and the divisor collapses to the in-plane in-bounds count, exactly
 *        ITK's true-2D window.
 *
 * A CONSTANT image (max == min) is passed through unchanged: ITK computes 0.0/0.0 = NaN there (a divide-by-
 * zero bug), which we intentionally do NOT replicate.
 *
 * Integer bounded-range fast path: for an INTEGER T whose gray range spans a modest number of levels, the
 * per-neighbor term of the cumulative function depends only on the integer difference @c d = Vc - Vn (the
 * normalization is affine, so @c u-v == (Vc-Vn)/iscale exactly). That term is precomputed once into a
 * difference-keyed lookup table and read per neighbor by @ref detail::AdaptiveHistogramEqualizationLutPlaneBody,
 * eliminating the @c std::pow that otherwise runs (2rx+1)(2ry+1)(2rz+1) times per voxel. The path is GATED on
 * @c std::is_integral_v<T> AND a bounded table size (@c kMaxAheLutEntries), so a huge dynamic range (or any
 * float type) keeps the exact per-neighbor path (@ref detail::AdaptiveHistogramEqualizationPlaneBody) unchanged.
 * This is a documented, difference-keyed TOLERANT deviation: the only numeric change is that @c |2d/iscale| is
 * formed from the exact integer difference instead of the twice-float-rounded @c (u-v), a ~1e-5-level drift far
 * below the integer-flip threshold. It remains a single implementation building one shared read-only LUT, so
 * in-core and out-of-core results stay bit-identical.
 *
 * Resident and true-3D execution retain their existing plane/ring implementations. A true-2D path with an OOC
 * endpoint uses a checked <=64 MiB fixed-output, full-width row-block, or one-row X-tile plan. The aggregate
 * includes typed input/output, the integer LUT allocation, and uint64 horizontal sums when the linear uint8
 * path is active. The true-3D linear uint8 Z-ring (@ref detail::ReserveAdaptiveHistogramEqualizationRingBand)
 * reads and writes planesPerBand consecutive Z-planes per store call instead of one, so a chunked out-of-core
 * store's parallel codec engages on the batched call instead of taking its single-chunk serial path;
 * planesPerBand is sized from the working-memory grant and degrades to one-plane calls when the grant is
 * tight. Every other store transfer in this function stays serial and outside parallel compute.
 *
 * @pre scalar array (1 component); @p in and @p out both hold dimX*dimY*dimZ values.
 * @tparam T scalar voxel type.
 *
 * @note @p alpha and @p beta follow ITK semantics and are intentionally NOT range-validated or clamped,
 *       matching the legacy filter (which imposes no bounds). The typical valid range is [0, 1]; an alpha < 0
 *       can yield NaN for equal-valued neighbors (pow(0, negative)). Callers that need bounds must enforce
 *       them upstream.
 * @note For an INTEGER output type, the final narrowing @c static_cast<T>(result) can be undefined if the
 *       transform drives @c result outside T's range. This is reachable even with valid parameters on
 *       high-contrast data (e.g. alpha=1, beta=0 pushes a bright spike to ~iscale*1.5), and more easily with
 *       out-of-typical-range beta. It is NOT clamped here because ITK performs the identical unclamped
 *       (TOutputPixel)(...) narrowing -- clamping would diverge from ITK parity. The result on overflow is thus
 *       the same implementation-defined/undefined value ITK produces.
 */
template <class T>
Result<> ApplyAdaptiveHistogramEqualization(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const SizeVec3& dims, const std::array<usize, 3>& radius, float32 alpha, float32 beta,
                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                                            usize target2DBytes = detail::k_AdaptiveHistogramEqualization2DTargetBytes)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return MakeErrorResult(-8541, fmt::format("Adaptive histogram equalization requires nonzero image dimensions. Dimensions: {} x {} x {}.", dimX, dimY, dimZ));
  }
  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  usize sliceValues = 0;
  usize volumeValues = 0;
  if(!checkedMultiply(dimX, dimY, sliceValues) || !checkedMultiply(sliceValues, dimZ, volumeValues))
  {
    return MakeErrorResult(-8542, fmt::format("Adaptive histogram equalization image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dimX, dimY, dimZ));
  }
  if(in.getSize() != volumeValues)
  {
    return MakeErrorResult(-8543, fmt::format("Adaptive histogram equalization input store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              in.getSize(), volumeValues, dimX, dimY, dimZ));
  }
  if(out.getSize() != volumeValues)
  {
    return MakeErrorResult(-8544, fmt::format("Adaptive histogram equalization output store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                              out.getSize(), volumeValues, dimX, dimY, dimZ));
  }
  const usize rx = radius[0];
  const usize ry = radius[1];
  const usize rz = radius[2];

  bool useLinearUint8 = false;
  if constexpr(std::is_same_v<T, uint8>)
  {
    useLinearUint8 = alpha == 1.0f && beta == 0.25f && volumeValues <= static_cast<usize>(std::numeric_limits<int64>::max() / std::numeric_limits<uint8>::max());
  }
  const bool usesOutOfCoreStore = in.getStoreType() == IDataStore::StoreType::OutOfCore || out.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(usesOutOfCoreStore && detail::ShouldUseAdaptiveHistogramEqualizationResidentState(dims))
  {
    if(shouldCancel)
    {
      return {};
    }
    auto allocationResult = detail::ReserveAdaptiveHistogramEqualizationResidentWorkingMemory<T>(dims, radius, useLinearUint8);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        DataStore<T> residentInput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, std::nullopt);
        DataStore<T> residentOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, std::nullopt);
        if(Result<> result = in.copyIntoBuffer(0, residentInput.createSpan()); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        if(Result<> result = ApplyAdaptiveHistogramEqualization(residentInput, residentOutput, dims, radius, alpha, beta, shouldCancel, messageHandler, target2DBytes); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        const auto outputSpan = residentOutput.createSpan();
        return out.copyFromBuffer(0, nonstd::span<const T>(outputSpan.data(), outputSpan.size()));
      } catch(const std::bad_alloc&)
      {
        // Release the complete-state reservation before entering the established bounded fallback.
      }
    }
  }

  MessageHelper messageHelper(messageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(dimZ);
  progressHelper.setProgressMessageTemplate("Applying adaptive histogram equalization: {:.1f}%");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  const bool useBounded2D = dimZ == 1 && (in.getStoreType() == IDataStore::StoreType::OutOfCore || out.getStoreType() == IDataStore::StoreType::OutOfCore);
  std::unique_ptr<T[]> outPlane;
  if(!useBounded2D)
  {
    outPlane = std::make_unique<T[]>(sliceValues);
  }

  // Global gray range: one bounded, OOC-safe streaming pass (min/max in T). ITK's BeforeThreadedGenerateData
  // does the same with MinimumMaximumImageFilter (this is why ITK "cannot stream"). Only min/max are consumed
  // here, so the parallel min/max reduction is used instead of the full (mean/variance-computing) statistics pass.
  Result<ArrayMinMax<T>> statsResult = ComputeArrayMinMax<T>(in, shouldCancel);
  if(statsResult.invalid())
  {
    return {nonstd::make_unexpected(std::move(statsResult.errors()))};
  }
  if(shouldCancel)
  {
    return {};
  }
  const float64 minValue = static_cast<float64>(statsResult.value().min);
  const float64 iscale = static_cast<float64>(statsResult.value().max) - minValue;

  // Constant image (max == min): ITK computes 0/0 = NaN. Pass the input through unchanged instead (documented
  // deviation). Bounded plane-copy loop, reusing outPlane.
  if(iscale == 0.0)
  {
    if(useBounded2D)
    {
      const usize chunkValues = target2DBytes / sizeof(T);
      if(chunkValues == 0)
      {
        return MakeErrorResult(-8545, fmt::format("Adaptive histogram equalization 2D target ({} bytes) cannot hold one {}-byte constant-image value.", target2DBytes, sizeof(T)));
      }
      auto buffer = std::make_unique<T[]>(std::min(chunkValues, volumeValues));
      for(usize start = 0; start < volumeValues; start += chunkValues)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize count = std::min(chunkValues, volumeValues - start);
        if(Result<> result = in.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        if(Result<> result = out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); result.invalid())
        {
          return result;
        }
      }
      return {};
    }
    for(usize z = 0; z < dimZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> r = in.copyIntoBuffer(z * sliceValues, nonstd::span<T>(outPlane.get(), sliceValues)); r.invalid())
      {
        return r;
      }
      if(Result<> r = out.copyFromBuffer(z * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }

  // Optional integer bounded-range LUT fast path (see the doxygen note above). For an INTEGER T whose gray range
  // spans few enough levels, precompute the per-neighbor cumulative-function term g(d) keyed by the integer
  // difference d = Vc - Vn, so the hot loop reads g(d) instead of calling std::pow per neighbor. gLut is empty
  // when the fast path is not taken (float T, or an integer span above the cap), in which case the exact
  // per-neighbor body runs unchanged. The if constexpr keeps float instantiations from ever building a LUT.
  [[maybe_unused]] std::vector<float> gLut; // non-empty iff the LUT fast path is active
  [[maybe_unused]] int64 dMin = 0;          // = -span; LUT index for difference d is (d - dMin)
  if constexpr(std::is_integral_v<T>)
  {
    constexpr usize kMaxAheLutEntries = detail::k_AdaptiveHistogramEqualizationMaximumLutEntries; // ~4M entries (16 MB float), table-size cap
    constexpr int64 kMaxSpan = static_cast<int64>((kMaxAheLutEntries - 1) / 2);                   // 2*span + 1 <= kMaxAheLutEntries
    const T minT = statsResult.value().min;
    const T maxT = statsResult.value().max;
    // Exact span magnitude for any integer width: unsigned subtraction cannot overflow, and maxT >= minT here
    // (iscale > 0 => max > min for integer T). The final guard rejects the exotic unsigned case where the
    // [min,max] band straddles the int64 sign boundary; that would make the int64 difference in the plane body
    // overflow, so it stays on the exact path instead.
    const uint64 uSpan = static_cast<uint64>(maxT) - static_cast<uint64>(minT);
    if(uSpan >= 1 && uSpan <= static_cast<uint64>(kMaxSpan) && static_cast<int64>(minT) <= static_cast<int64>(maxT))
    {
      const int64 span = static_cast<int64>(uSpan);
      dMin = -span;
      gLut.resize(static_cast<usize>(2 * span + 1));
      const float betaHalf = beta * 0.5f;
      for(usize i = 0; i < gLut.size(); ++i)
      {
        const int64 d = static_cast<int64>(i) + dMin; // d runs [-span, +span]
        const double t = static_cast<double>(d) / iscale;
        const float sF = static_cast<float>(detail::AdaptiveEqualizationSgn(static_cast<float>(t)));
        const float ad = static_cast<float>(std::fabs(2.0 * t));
        // Same expression shape as the per-neighbor path, so the documented NaN corner is preserved verbatim:
        // for d == 0 with alpha < 0, pow(0, negative) == inf and 0.0f * inf == NaN.
        gLut[i] = 0.5f * sF * std::pow(ad, alpha) - betaHalf * sF * ad;
      }
    }
  }

  // For uint8 images with alpha == 1 and beta == 0.25, the difference-dependent part of Stark's cumulative
  // function is
  // linear: g(Vc - Vn) = (1 - beta) * (Vc - Vn) / iscale. Therefore its neighborhood sum depends only on
  // the box sum and valid-neighbor count, rather than on every individual neighbor. Maintain each plane's
  // clipped XY box sums in a Z ring and update the Z aggregate as planes enter and leave the window. This
  // reduces the common 21^3 benchmark from O(volume * neighborhood) to O(volume) and reads every source plane
  // exactly once. Reads and writes move planesPerBand consecutive planes per store call (see
  // detail::ReserveAdaptiveHistogramEqualizationRingBand) instead of one, so a chunked out-of-core store's
  // parallel codec engages instead of its single-chunk serial path; the per-plane box-filter and Z-fold
  // arithmetic that follows is untouched by that batching. Memory remains bounded, now at
  // O((2rz+1+planesPerBand) * dimX * dimY): planesPerBand extra raw planes of read-ahead/write-behind headroom
  // atop the (2rz+1)-deep box-sum ring. Other types and other parameter values keep the general implementation
  // below. The beta gate deliberately limits the algebraic reassociation to the histogram configuration covered
  // by both the committed ITK golden and live ITK parity tests; other beta values can straddle additional
  // uint8 truncation boundaries.
  if constexpr(std::is_same_v<T, uint8>)
  {
    if(alpha == 1.0f && beta == 0.25f && volumeValues <= static_cast<usize>(std::numeric_limits<int64>::max() / std::numeric_limits<uint8>::max()))
    {
      if(useBounded2D)
      {
        usize lookupBytes = 0;
        if(!detail::TryMultiplyAdaptiveHistogramSize(gLut.capacity(), sizeof(float), lookupBytes))
        {
          return MakeErrorResult(-8545, fmt::format("Adaptive histogram equalization lookup-table capacity ({}) overflows its byte count.", gLut.capacity()));
        }
        const bool preferFixedOutput = out.getStoreType() == IDataStore::StoreType::OutOfCore;
        auto planResult = detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, rx, ry, sizeof(T), target2DBytes, lookupBytes, /*useHorizontalSums=*/true, preferFixedOutput);
        if(planResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(planResult));
        }
        const float unitDifferenceTerm = gLut[static_cast<usize>(1 - dMin)];
        return detail::ExecuteAdaptiveHistogramEqualization2D<uint8>(in, out, dims, radius, planResult.value(), shouldCancel,
                                                                     [&](const uint8* inputBlock, uint8* outputBlock, uint64* horizontalBlock, usize inputXBegin, usize inputYBegin, usize inputWidth,
                                                                         usize inputRows, usize outputXBegin, usize outputYBegin, usize outputWidth, usize outputRows) {
                                                                       detail::CalculateAdaptiveHistogramEqualizationLinear2DBlock(
                                                                           inputBlock, outputBlock, horizontalBlock, dimX, dimY, rx, ry, inputXBegin, inputYBegin, inputWidth, inputRows, outputXBegin,
                                                                           outputYBegin, outputWidth, outputRows, minValue, iscale, beta, unitDifferenceTerm, shouldCancel);
                                                                     });
      }
      const usize ringDepth = rz >= dimZ / 2 ? dimZ : 2 * rz + 1;

      // Band-batched store transfers (defect class CC-A): at the certified uint8 1024x1024 geometry a chunk is
      // exactly one Z-plane, so a single-plane copyIntoBuffer/copyFromBuffer call always takes
      // ParallelForChunkPositions' single-chunk serial branch -- the whole plane's inflate or deflate runs on
      // one core, in both directions, for the entire ring. Reading/writing planesPerBand consecutive planes per
      // store call makes each call span planesPerBand chunks so the parallel codec path engages. The compute
      // below is unchanged: it still consumes and produces exactly one plane at a time, in the same order, via
      // spans into the band buffers below, so the fold order into zBoxSums -- and therefore every output value
      // -- is bit-identical to the pre-existing per-plane form; only the store-call granularity changes.
      // planesPerBand comes from the working-memory grant (see
      // detail::ReserveAdaptiveHistogramEqualizationRingBand), never from a machine- or dataset-size-tuned
      // constant, and degrades to 1 (one-plane store calls, matching the pre-existing behavior) when the grant
      // is tight.
      auto ringBandResult = detail::ReserveAdaptiveHistogramEqualizationRingBand(sliceValues, dimZ);
      if(ringBandResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(ringBandResult));
      }
      auto ringBand = std::move(ringBandResult.value());
      const usize planesPerBand = ringBand.planesPerBand;

      // Raw-plane ring, flattened into one buffer so that a band of planesPerBand consecutive Z-planes is
      // contiguous in memory (modulo rawRingCapacity) and can be filled by a single store call. Sized
      // ringDepth + planesPerBand: at most ringDepth planes are ever counted in the zBoxSums window (the
      // eviction/entry bookkeeping below is unchanged), and at most planesPerBand - 1 additional planes can be
      // resident read-ahead-but-not-yet-box-filtered at any moment (a band read supplies up to planesPerBand
      // planes at once, but only the one immediately due is box-filtered/folded per iteration; the rest wait
      // for their own iteration). ringDepth + planesPerBand slots therefore always exceeds the maximum
      // concurrently-live plane span, so two distinct live planes never alias the same slot.
      const usize rawRingCapacity = ringDepth + planesPerBand;
      std::vector<uint8> sourcePlanes(rawRingCapacity * sliceValues);
      std::vector<std::vector<uint64>> xyBoxSums(ringDepth, std::vector<uint64>(sliceValues));
      std::vector<uint64> horizontalSums(sliceValues);
      std::vector<uint64> zBoxSums(sliceValues, 0);

      // Reads planes [startZ, startZ + count) directly into their final ring slots in one store call (or two,
      // split at the point where slot index startZ % rawRingCapacity wraps back to 0). There is no intermediate
      // staging buffer: the store call writes straight into the memory the box filter and, later, the
      // center-value read will consume, so batching the read adds no extra per-plane copy.
      auto readPlaneBand = [&](usize startZ, usize count) -> Result<> {
        const usize startSlot = startZ % rawRingCapacity;
        const usize firstPart = std::min(count, rawRingCapacity - startSlot);
        if(Result<> r = in.copyIntoBuffer(startZ * sliceValues, nonstd::span<uint8>(sourcePlanes.data() + startSlot * sliceValues, firstPart * sliceValues)); r.invalid())
        {
          return r;
        }
        const usize remaining = count - firstPart;
        if(remaining > 0)
        {
          if(Result<> r = in.copyIntoBuffer((startZ + firstPart) * sliceValues, nonstd::span<uint8>(sourcePlanes.data(), remaining * sliceValues)); r.invalid())
          {
            return r;
          }
        }
        return {};
      };

      // Ensures every plane up to and including neededZ has been read, issuing one band read per call. Reading
      // a plane before its own iteration is always safe: it only moves bytes into memory earlier, and does not
      // change any value the ring computes or the order zBoxSums is folded in.
      usize nextReadZ = 0;
      auto ensureRead = [&](usize neededZ) -> Result<> {
        while(nextReadZ <= neededZ)
        {
          const usize count = std::min(planesPerBand, dimZ - nextReadZ);
          if(Result<> r = readPlaneBand(nextReadZ, count); r.invalid())
          {
            return r;
          }
          nextReadZ += count;
        }
        return {};
      };

      // Box-filters the already-resident plane sourceZ (see ensureRead) into xyBoxSums[xyBoxSumSlot]. Identical
      // arithmetic to the pre-existing per-plane form; only the raw-byte source moved from a per-slot vector to
      // the flattened band ring, indexed by sourceZ's own position rather than by the caller's xyBoxSums slot.
      auto boxFilterPlane = [&](usize sourceZ, usize xyBoxSumSlot) {
        const uint8* plane = sourcePlanes.data() + (sourceZ % rawRingCapacity) * sliceValues;

        auto filterRows = [&](const Range& range) {
          for(usize y = range.min(); y < range.max(); ++y)
          {
            const usize row = y * dimX;
            uint64 sum = 0;
            const usize initialXHi = std::min(rx, dimX - 1);
            for(usize x = 0; x <= initialXHi; ++x)
            {
              sum += plane[row + x];
            }
            for(usize x = 0; x < dimX; ++x)
            {
              horizontalSums[row + x] = sum;
              if(x >= rx)
              {
                sum -= plane[row + x - rx];
              }
              if(rx < dimX - x - 1)
              {
                sum += plane[row + x + rx + 1];
              }
            }
          }
        };
        ParallelDataAlgorithm rowAlgorithm;
        rowAlgorithm.setRange(0, dimY);
        rowAlgorithm.execute(filterRows);

        auto filterColumns = [&](const Range& range) {
          for(usize x = range.min(); x < range.max(); ++x)
          {
            uint64 sum = 0;
            const usize initialYHi = std::min(ry, dimY - 1);
            for(usize y = 0; y <= initialYHi; ++y)
            {
              sum += horizontalSums[y * dimX + x];
            }
            for(usize y = 0; y < dimY; ++y)
            {
              xyBoxSums[xyBoxSumSlot][y * dimX + x] = sum;
              if(y >= ry)
              {
                sum -= horizontalSums[(y - ry) * dimX + x];
              }
              if(ry < dimY - y - 1)
              {
                sum += horizontalSums[(y + ry + 1) * dimX + x];
              }
            }
          }
        };
        ParallelDataAlgorithm columnAlgorithm;
        columnAlgorithm.setRange(0, dimX);
        columnAlgorithm.execute(filterColumns);
      };

      usize firstSlot = 0;
      usize activePlanes = std::min(rz, dimZ - 1) + 1;
      for(usize sourceZ = 0; sourceZ < activePlanes; ++sourceZ)
      {
        if(shouldCancel)
        {
          return {};
        }
        if(Result<> r = ensureRead(sourceZ); r.invalid())
        {
          return r;
        }
        boxFilterPlane(sourceZ, sourceZ);
        for(usize i = 0; i < sliceValues; ++i)
        {
          zBoxSums[i] += xyBoxSums[sourceZ][i];
        }
      }

      // Output write-behind band: calculateOutput below writes plane z's result into outputBand at offset
      // (z - bandBaseZ); the band is flushed with one copyFromBuffer call once it holds planesPerBand planes or
      // the loop reaches the last plane. out is never read back anywhere in this function, so buffering the
      // write is always safe. A flush also runs immediately before any cancellation return, so every plane
      // whose write point was already reached (i.e. every plane counted in bandFilledCount) stays durably
      // committed -- matching the pre-existing per-plane form, where only the one plane in flight at the moment
      // of cancellation is ever skipped.
      std::vector<uint8> outputBand(planesPerBand * sliceValues);
      usize bandBaseZ = 0;
      usize bandFilledCount = 0;
      auto flushOutputBand = [&]() -> Result<> {
        if(bandFilledCount == 0)
        {
          return {};
        }
        if(Result<> r = out.copyFromBuffer(bandBaseZ * sliceValues, nonstd::span<const uint8>(outputBand.data(), bandFilledCount * sliceValues)); r.invalid())
        {
          return r;
        }
        bandBaseZ += bandFilledCount;
        bandFilledCount = 0;
        return {};
      };

      for(usize z = 0; z < dimZ; ++z)
      {
        if(shouldCancel)
        {
          if(Result<> r = flushOutputBand(); r.invalid())
          {
            return r;
          }
          return {};
        }
        const usize zLo = z >= rz ? z - rz : 0;
        const usize zHi = rz >= dimZ - z - 1 ? dimZ - 1 : z + rz;
        const usize zCount = zHi - zLo + 1;
        // Use the already-built LUT's unit-difference value so the closed form retains the same float
        // coefficient rounding as the general integer path.
        const float unitDifferenceTerm = gLut[static_cast<usize>(1 - dMin)];

        const uint8* centerPlane = sourcePlanes.data() + (z % rawRingCapacity) * sliceValues;
        uint8* outputSlot = outputBand.data() + (z - bandBaseZ) * sliceValues;
        auto calculateOutput = [&](const Range& range) {
          for(usize y = range.min(); y < range.max(); ++y)
          {
            if(shouldCancel)
            {
              return;
            }
            const usize yLo = y >= ry ? y - ry : 0;
            const usize yHi = ry >= dimY - y - 1 ? dimY - 1 : y + ry;
            const usize yCount = yHi - yLo + 1;
            for(usize x = 0; x < dimX; ++x)
            {
              const usize xLo = x >= rx ? x - rx : 0;
              const usize xHi = rx >= dimX - x - 1 ? dimX - 1 : x + rx;
              const usize count = (xHi - xLo + 1) * yCount * zCount;
              const usize index = y * dimX + x;
              const uint8 center = centerPlane[index];
              const float u = static_cast<float>((static_cast<float64>(center) - minValue) / iscale - 0.5);
              const int64 differenceSum = static_cast<int64>(center) * static_cast<int64>(count) - static_cast<int64>(zBoxSums[index]);
              const float64 localSum = static_cast<float64>(unitDifferenceTerm) * static_cast<float64>(differenceSum) + static_cast<float64>(beta * u) * count;
              const float64 result = iscale * (localSum / static_cast<float64>(count) + 0.5) + minValue;
              outputSlot[index] = static_cast<uint8>(result);
            }
          }
        };
        ParallelDataAlgorithm outputAlgorithm;
        outputAlgorithm.setRange(0, dimY);
        outputAlgorithm.execute(calculateOutput);
        if(shouldCancel)
        {
          if(Result<> r = flushOutputBand(); r.invalid())
          {
            return r;
          }
          return {};
        }
        ++bandFilledCount;
        if(bandFilledCount == planesPerBand || z + 1 == dimZ)
        {
          if(Result<> r = flushOutputBand(); r.invalid())
          {
            return r;
          }
        }
        progressMessenger.sendProgressMessage(1);

        if(z + 1 == dimZ)
        {
          continue;
        }
        const usize nextZLo = z + 1 >= rz ? z + 1 - rz : 0;
        if(nextZLo > zLo)
        {
          for(usize i = 0; i < sliceValues; ++i)
          {
            zBoxSums[i] -= xyBoxSums[firstSlot][i];
          }
          firstSlot = (firstSlot + 1) % ringDepth;
          --activePlanes;
        }
        if(rz < dimZ - z - 1)
        {
          const usize enteringZ = z + rz + 1;
          const usize slot = (firstSlot + activePlanes) % ringDepth;
          if(Result<> r = ensureRead(enteringZ); r.invalid())
          {
            return r;
          }
          boxFilterPlane(enteringZ, slot);
          for(usize i = 0; i < sliceValues; ++i)
          {
            zBoxSums[i] += xyBoxSums[slot][i];
          }
          ++activePlanes;
        }
      }
      return {};
    }
  }

  if(useBounded2D)
  {
    usize lookupBytes = 0;
    if(!detail::TryMultiplyAdaptiveHistogramSize(gLut.capacity(), sizeof(float), lookupBytes))
    {
      return MakeErrorResult(-8545, fmt::format("Adaptive histogram equalization lookup-table capacity ({}) overflows its byte count.", gLut.capacity()));
    }
    const bool preferFixedOutput = out.getStoreType() == IDataStore::StoreType::OutOfCore;
    auto planResult = detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, rx, ry, sizeof(T), target2DBytes, lookupBytes, /*useHorizontalSums=*/false, preferFixedOutput);
    if(planResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(planResult));
    }
    return detail::ExecuteAdaptiveHistogramEqualization2D<T>(
        in, out, dims, radius, planResult.value(), shouldCancel,
        [&](const T* inputBlock, T* outputBlock, uint64*, usize inputXBegin, usize inputYBegin, usize inputWidth, usize, usize outputXBegin, usize outputYBegin, usize outputWidth, usize outputRows) {
          ParallelDataAlgorithm parallelAlgorithm;
          parallelAlgorithm.setRange(0, outputRows * outputWidth);
          bool executed = false;
          if constexpr(std::is_integral_v<T>)
          {
            if(!gLut.empty())
            {
              parallelAlgorithm.execute(detail::AdaptiveHistogramEqualizationLut2DBlockBody<T>{.input = inputBlock,
                                                                                               .output = outputBlock,
                                                                                               .dimX = dimX,
                                                                                               .dimY = dimY,
                                                                                               .radiusX = rx,
                                                                                               .radiusY = ry,
                                                                                               .inputXBegin = inputXBegin,
                                                                                               .inputYBegin = inputYBegin,
                                                                                               .inputWidth = inputWidth,
                                                                                               .outputXBegin = outputXBegin,
                                                                                               .outputYBegin = outputYBegin,
                                                                                               .outputWidth = outputWidth,
                                                                                               .minValue = minValue,
                                                                                               .iscale = iscale,
                                                                                               .beta = beta,
                                                                                               .lookupTable = gLut.data(),
                                                                                               .differenceMinimum = dMin,
                                                                                               .lookupTableSize = gLut.size(),
                                                                                               .shouldCancel = shouldCancel});
              executed = true;
            }
          }
          if(!executed)
          {
            parallelAlgorithm.execute(detail::AdaptiveHistogramEqualization2DBlockBody<T>{.input = inputBlock,
                                                                                          .output = outputBlock,
                                                                                          .dimX = dimX,
                                                                                          .dimY = dimY,
                                                                                          .radiusX = rx,
                                                                                          .radiusY = ry,
                                                                                          .inputXBegin = inputXBegin,
                                                                                          .inputYBegin = inputYBegin,
                                                                                          .inputWidth = inputWidth,
                                                                                          .outputXBegin = outputXBegin,
                                                                                          .outputYBegin = outputYBegin,
                                                                                          .outputWidth = outputWidth,
                                                                                          .minValue = minValue,
                                                                                          .iscale = iscale,
                                                                                          .alpha = alpha,
                                                                                          .beta = beta,
                                                                                          .shouldCancel = shouldCancel});
          }
        });
  }

  // Allocate the slab once at its max depth and reuse it across every Z-plane (see MorphScanline). Bounded at
  // O(min(2rz+1, dimZ) * dimX * dimY), never O(volume).
  const usize maxSlabDepth = rz >= dimZ / 2 ? dimZ : 2 * rz + 1;
  auto slab = std::make_unique<T[]>(maxSlabDepth * sliceValues);

  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize zLo = (z >= rz) ? (z - rz) : 0;
    const usize zHi = rz >= dimZ - z - 1 ? dimZ - 1 : z + rz;
    const usize slabDepth = zHi - zLo + 1;
    if(Result<> r = in.copyIntoBuffer(zLo * sliceValues, nonstd::span<T>(slab.get(), slabDepth * sliceValues)); r.invalid())
    {
      return r;
    }

    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, sliceValues);

    // Integer bounded-range LUT fast path when active; otherwise the exact per-neighbor path (float T, or an
    // integer span above the cap). The if constexpr keeps the LUT body from being instantiated for float T.
    bool executed = false;
    if constexpr(std::is_integral_v<T>)
    {
      if(!gLut.empty())
      {
        parallelAlgorithm.execute(detail::AdaptiveHistogramEqualizationLutPlaneBody<T>{.slab = slab.get(),
                                                                                       .outPlane = outPlane.get(),
                                                                                       .dimX = dimX,
                                                                                       .dimY = dimY,
                                                                                       .dimZ = dimZ,
                                                                                       .rx = rx,
                                                                                       .ry = ry,
                                                                                       .rz = rz,
                                                                                       .zLo = zLo,
                                                                                       .z = z,
                                                                                       .minValue = minValue,
                                                                                       .iscale = iscale,
                                                                                       .beta = beta,
                                                                                       .gLut = gLut.data(),
                                                                                       .dMin = dMin,
                                                                                       .lutSize = gLut.size()});
        executed = true;
      }
    }
    if(!executed)
    {
      parallelAlgorithm.execute(detail::AdaptiveHistogramEqualizationPlaneBody<T>{.slab = slab.get(),
                                                                                  .outPlane = outPlane.get(),
                                                                                  .dimX = dimX,
                                                                                  .dimY = dimY,
                                                                                  .dimZ = dimZ,
                                                                                  .rx = rx,
                                                                                  .ry = ry,
                                                                                  .rz = rz,
                                                                                  .zLo = zLo,
                                                                                  .z = z,
                                                                                  .minValue = minValue,
                                                                                  .iscale = iscale,
                                                                                  .alpha = alpha,
                                                                                  .beta = beta});
    }

    if(Result<> r = out.copyFromBuffer(z * sliceValues, nonstd::span<const T>(outPlane.get(), sliceValues)); r.invalid())
    {
      return r;
    }
    progressMessenger.sendProgressMessage(1);
  }
  return {};
}
} // namespace nx::core::ImageProcessing
