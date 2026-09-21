#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/RadiusOneStencil2D.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace nx::core::ImageProcessing
{
namespace detail
{
inline bool ZeroCrossingCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool ZeroCrossingCheckedAdd(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

/// Approximate number of input values processed by one parallel dispatch of the direct 3-D route.
inline constexpr usize k_ZeroCrossingDirectGroupValues = usize{16} << 20;

/**
 * @struct ZeroCrossingSlabPlan
 * @brief Stores the core depth and buffer sizes for one 3-D slab.
 */
struct ZeroCrossingSlabPlan
{
  usize corePlanes = 0;
  usize inputPlanes = 0;
  usize inputBytes = 0;
  usize outputBytes = 0;
};

/**
 * @brief Calculates the useful memory for complete 3-D input and output buffers.
 * @tparam T Specifies the signed input value type.
 * @param dims Image dimensions in XYZ order.
 * @return Useful bytes, or error -8761 if the calculation overflows. An empty dimension returns zero.
 */
template <class T>
Result<usize> ZeroCrossingSlabUsefulBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize valueBytes = 0;
  usize usefulBytes = 0;
  if(!ZeroCrossingCheckedMultiply(dims[0], dims[1], sliceValues) || !ZeroCrossingCheckedMultiply(sliceValues, dims[2], volumeValues) || !ZeroCrossingCheckedAdd(sizeof(T), sizeof(uint8), valueBytes) ||
     !ZeroCrossingCheckedMultiply(volumeValues, valueBytes, usefulBytes))
  {
    return MakeErrorResult<usize>(-8761, fmt::format("Zero crossing dimensions ({}) and input element size ({} bytes) overflow while sizing the complete input and uint8 output buffers.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {usefulBytes};
}

/**
 * @brief Plans a 3-D slab from the shared working-memory grant.
 * @tparam T Specifies the signed input value type.
 * @param dims Image dimensions in XYZ order. The dimensions must have a valid useful-byte calculation.
 * @param grantBytes Working-memory grant in bytes.
 * @return Core depth, input depth, and buffer sizes for one slab.
 *
 * The grant controls the core depth so slab memory stays bounded by the shared budget when the minimum fits.
 * A small grant uses the minimum three input planes and one output plane, even when that footprint exceeds the grant.
 * A complete grant removes halo storage and permits one bulk read and one bulk write.
 */
template <class T>
ZeroCrossingSlabPlan PlanZeroCrossingSlab(const SizeVec3& dims, uint64 grantBytes)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }

  const usize sliceValues = dims[0] * dims[1];
  const usize inputPlaneBytes = sliceValues * sizeof(T);
  const usize outputPlaneBytes = sliceValues * sizeof(uint8);
  const usize usefulBytes = dims[2] * (inputPlaneBytes + outputPlaneBytes);
  if(grantBytes >= usefulBytes)
  {
    return {.corePlanes = dims[2], .inputPlanes = dims[2], .inputBytes = dims[2] * inputPlaneBytes, .outputBytes = dims[2] * outputPlaneBytes};
  }

  const uint64 haloBytes = uint64{2} * inputPlaneBytes;
  const uint64 corePlaneBytes = inputPlaneBytes + outputPlaneBytes;
  const usize fittingCorePlanes = grantBytes > haloBytes ? static_cast<usize>((grantBytes - haloBytes) / corePlaneBytes) : usize{0};
  const usize corePlanes = std::max<usize>(usize{1}, fittingCorePlanes);
  return {.corePlanes = corePlanes, .inputPlanes = corePlanes + 2, .inputBytes = (corePlanes + 2) * inputPlaneBytes, .outputBytes = corePlanes * outputPlaneBytes};
}

/**
 * @brief Selects the type that carries a zero-crossing magnitude for input type T.
 * @tparam T Specifies the signed input value type.
 *
 * ITK keeps T for integer types narrower than int. The minimum magnitude therefore wraps to a negative value.
 * For int and wider types, ITK compares the exact unsigned magnitude. Floating-point types keep T.
 */
template <class T>
using ZeroCrossingMagnitudeType = std::conditional_t<std::is_integral_v<T> && (sizeof(T) >= sizeof(int32)), std::make_unsigned_t<std::conditional_t<std::is_integral_v<T>, T, int32>>, T>;

/**
 * @brief Returns the magnitude of a value as ITK compares it.
 * @tparam T Specifies the signed input value type.
 * @param value Input value.
 * @return The magnitude in ZeroCrossingMagnitudeType<T>. The minimum of a narrow integer type wraps to a negative value.
 *
 * Integer negation runs in the unsigned domain, which is defined for the minimum value. Types narrower than int cast
 * the exact magnitude back to T, which wraps the minimum to itself. Wider types keep the exact unsigned magnitude.
 */
template <class T>
inline ZeroCrossingMagnitudeType<T> ZeroCrossingMagnitude(T value)
{
  if constexpr(std::is_integral_v<T>)
  {
    using Unsigned = std::make_unsigned_t<T>;
    const Unsigned magnitude = value < T{} ? static_cast<Unsigned>(Unsigned{0} - static_cast<Unsigned>(value)) : static_cast<Unsigned>(value);
    return static_cast<ZeroCrossingMagnitudeType<T>>(magnitude);
  }
  else
  {
    return value < T{} ? -value : value;
  }
}

/**
 * @brief Tests one axial neighbor for a zero crossing that the center owns.
 * @tparam T Specifies the signed input value type.
 * @param c Center value.
 * @param n Neighbor value.
 * @param positiveDirection One for the +X, +Y, and +Z neighbors, which win magnitude ties; zero otherwise.
 * @return One when the pair changes sign and the center is the closer-to-zero side, otherwise zero.
 *
 * The predicate combines 0/1 masks with bitwise operators. A voxel evaluates all six neighbors without branches.
 * The masks give the same result as the short-circuit rule, including for NaN and signed zero.
 * The magnitude uses a defined wrap, so its unconditional evaluation is safe for every value.
 */
template <class T>
inline uint32 ZeroCrossingCrosses(T c, T n, uint32 positiveDirection)
{
  const uint32 centerNegative = static_cast<uint32>(c < T{});
  const uint32 centerPositive = static_cast<uint32>(c > T{});
  const uint32 centerZero = static_cast<uint32>(c == T{});
  const uint32 neighborNegative = static_cast<uint32>(n < T{});
  const uint32 neighborPositive = static_cast<uint32>(n > T{});
  const uint32 neighborZero = static_cast<uint32>(n == T{});
  const uint32 signChange = (centerNegative & neighborPositive) | (centerPositive & neighborNegative) | (centerZero & (neighborZero ^ uint32{1})) | ((centerZero ^ uint32{1}) & neighborZero);
  const ZeroCrossingMagnitudeType<T> centerMagnitude = ZeroCrossingMagnitude(c);
  const ZeroCrossingMagnitudeType<T> neighborMagnitude = ZeroCrossingMagnitude(n);
  const uint32 magnitudeLess = static_cast<uint32>(centerMagnitude < neighborMagnitude);
  const uint32 magnitudeEqual = static_cast<uint32>(centerMagnitude == neighborMagnitude);
  return signChange & (magnitudeLess | (magnitudeEqual & positiveDirection));
}

/**
 * @brief Evaluates one voxel of a 3-D row against its six axial neighbors.
 * @tparam T Specifies the signed input value type.
 * @param previousPlaneRow Row at the negative Z neighbor.
 * @param previousRow Row at the negative Y neighbor.
 * @param row Current input row.
 * @param nextRow Row at the positive Y neighbor.
 * @param nextPlaneRow Row at the positive Z neighbor.
 * @param x Column of the voxel.
 * @param negativeX Column of the negative X neighbor, clamped at the row start.
 * @param positiveX Column of the positive X neighbor, clamped at the row end.
 * @return Nonzero when any neighbor forms a crossing that the voxel owns.
 */
template <class T>
inline uint32 ZeroCrossingHitAt(const T* previousPlaneRow, const T* previousRow, const T* row, const T* nextRow, const T* nextPlaneRow, usize x, usize negativeX, usize positiveX)
{
  const T center = row[x];
  uint32 hit = ZeroCrossingCrosses(center, row[negativeX], uint32{0});
  hit |= ZeroCrossingCrosses(center, previousRow[x], uint32{0});
  hit |= ZeroCrossingCrosses(center, previousPlaneRow[x], uint32{0});
  hit |= ZeroCrossingCrosses(center, row[positiveX], uint32{1});
  hit |= ZeroCrossingCrosses(center, nextRow[x], uint32{1});
  hit |= ZeroCrossingCrosses(center, nextPlaneRow[x], uint32{1});
  return hit;
}

/**
 * @brief Evaluates one 3-D row with the six axial neighbors.
 * @tparam T Specifies the signed input value type.
 * @param previousPlaneRow Row at the negative Z neighbor, clamped to the current row at the boundary.
 * @param previousRow Row at the negative Y neighbor, clamped to the current row at the boundary.
 * @param row Current input row.
 * @param nextRow Row at the positive Y neighbor, clamped to the current row at the boundary.
 * @param nextPlaneRow Row at the positive Z neighbor, clamped to the current row at the boundary.
 * @param output Receives the output labels for the current row.
 * @param nx Number of values in each row.
 * @param foreground Label for a detected crossing.
 * @param background Label for a value without a crossing.
 * @pre All row pointers and the output point to at least nx values.
 *
 * The border columns use clamped X neighbors. The interior loop reads the six neighbors at fixed offsets and carries
 * no per-column branches, so the compiler can vectorize it.
 */
template <class T>
inline void ZeroCrossingRow(const T* previousPlaneRow, const T* previousRow, const T* row, const T* nextRow, const T* nextPlaneRow, uint8* output, usize nx, uint8 foreground, uint8 background)
{
  if(nx < 3)
  {
    for(usize x = 0; x < nx; ++x)
    {
      const usize negativeX = x > 0 ? x - 1 : 0;
      const usize positiveX = x + 1 < nx ? x + 1 : nx - 1;
      const uint32 hit = ZeroCrossingHitAt(previousPlaneRow, previousRow, row, nextRow, nextPlaneRow, x, negativeX, positiveX);
      output[x] = hit != 0 ? foreground : background;
    }
    return;
  }

  const uint32 firstHit = ZeroCrossingHitAt(previousPlaneRow, previousRow, row, nextRow, nextPlaneRow, 0, 0, 1);
  output[0] = firstHit != 0 ? foreground : background;

  const T* const negativeZ = previousPlaneRow;
  const T* const negativeY = previousRow;
  const T* const center = row;
  const T* const positiveY = nextRow;
  const T* const positiveZ = nextPlaneRow;
  uint8* const labels = output;
  const uint8 foregroundLabel = foreground;
  const uint8 backgroundLabel = background;
  const usize lastX = nx - 1;
  for(usize x = 1; x < lastX; ++x)
  {
    const T c = center[x];
    uint32 hit = ZeroCrossingCrosses(c, center[x - 1], uint32{0});
    hit |= ZeroCrossingCrosses(c, negativeY[x], uint32{0});
    hit |= ZeroCrossingCrosses(c, negativeZ[x], uint32{0});
    hit |= ZeroCrossingCrosses(c, center[x + 1], uint32{1});
    hit |= ZeroCrossingCrosses(c, positiveY[x], uint32{1});
    hit |= ZeroCrossingCrosses(c, positiveZ[x], uint32{1});
    labels[x] = hit != 0 ? foregroundLabel : backgroundLabel;
  }

  const uint32 lastHit = ZeroCrossingHitAt(previousPlaneRow, previousRow, row, nextRow, nextPlaneRow, lastX, lastX - 1, lastX);
  output[lastX] = lastHit != 0 ? foreground : background;
}

template <class T>
struct ZeroCrossing2DBlockBody
{
  const T* input;
  uint8* output;
  usize dimX;
  usize dimY;
  usize inputXBegin;
  usize inputYBegin;
  usize inputWidth;
  usize outputXBegin;
  usize outputYBegin;
  usize outputWidth;
  uint8 foreground;
  uint8 background;

  void operator()(const Range& range) const
  {
    const int64 maxX = static_cast<int64>(dimX) - 1;
    const int64 maxY = static_cast<int64>(dimY) - 1;
    const auto clamp = [](int64 value, int64 upper) { return value < 0 ? int64{0} : (value > upper ? upper : value); };
    for(usize tuple = range.min(); tuple < range.max(); ++tuple)
    {
      const usize localX = tuple % outputWidth;
      const usize localY = tuple / outputWidth;
      const usize x = outputXBegin + localX;
      const usize y = outputYBegin + localY;
      const T center = input[(y - inputYBegin) * inputWidth + (x - inputXBegin)];
      const auto valueAt = [&](int64 neighborX, int64 neighborY) {
        const usize clampedX = static_cast<usize>(clamp(neighborX, maxX));
        const usize clampedY = static_cast<usize>(clamp(neighborY, maxY));
        return input[(clampedY - inputYBegin) * inputWidth + (clampedX - inputXBegin)];
      };
      const auto crosses = [&](T neighbor, bool positiveDirection) {
        const bool signChange = ((center < T{}) && (neighbor > T{})) || ((center > T{}) && (neighbor < T{})) || ((center == T{}) && (neighbor != T{})) || ((center != T{}) && (neighbor == T{}));
        if(!signChange)
        {
          return false;
        }
        const ZeroCrossingMagnitudeType<T> centerMagnitude = ZeroCrossingMagnitude(center);
        const ZeroCrossingMagnitudeType<T> neighborMagnitude = ZeroCrossingMagnitude(neighbor);
        return centerMagnitude < neighborMagnitude || (centerMagnitude == neighborMagnitude && positiveDirection);
      };

      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);
      const bool hit = crosses(valueAt(xi - 1, yi), false) || crosses(valueAt(xi, yi - 1), false) || crosses(valueAt(xi + 1, yi), true) || crosses(valueAt(xi, yi + 1), true);
      output[tuple] = hit ? foreground : background;
    }
  }
};
} // namespace detail

/**
 * @brief Finds axial zero crossings with storage-specific execution routes.
 * @tparam T Specifies the signed input value type.
 * @param inStore Input scalar store.
 * @param outStore Output uint8 label store.
 * @param dims Image dimensions in XYZ order.
 * @param foregroundValue Label for a detected crossing.
 * @param backgroundValue Label for a value without a crossing.
 * @param shouldCancel Stops work before 3-D processing, between direct-route plane groups, or between slabs.
 * @param messageHandler Reserved for facade consistency. This function does not send messages.
 * @param target2DBytes Target working-memory size for single-slice processing, in bytes.
 * @return An error for overflow, failed allocation, or failed transfer. Cancellation returns a valid result.
 *
 * The direct 3-D route reads and writes in-memory spans through parallel rows.
 * The slab route uses a budget-derived depth and performs one bulk read and one bulk write per slab.
 * Single-slice images use checked row blocks or X tiles with bounded buffers.
 */
template <class T>
Result<> ApplyZeroCrossing(const AbstractDataStore<T>& inStore, AbstractDataStore<uint8>& outStore, const SizeVec3& dims, uint8 foregroundValue, uint8 backgroundValue,
                           const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_RadiusOneStencil2DTargetBytes)
{
  if(dims[2] == 1)
  {
    return detail::ExecuteRadiusOneStencil2D<T, uint8>(
        inStore, outStore, dims, shouldCancel, target2DBytes, /*workerScratchBytes=*/0, /*maximumWorkers=*/0,
        [&](const T* input, uint8* output, usize inputXBegin, usize inputYBegin, usize inputWidth, usize outputXBegin, usize outputYBegin, usize outputWidth) {
          return detail::ZeroCrossing2DBlockBody<T>{.input = input,
                                                    .output = output,
                                                    .dimX = dims[0],
                                                    .dimY = dims[1],
                                                    .inputXBegin = inputXBegin,
                                                    .inputYBegin = inputYBegin,
                                                    .inputWidth = inputWidth,
                                                    .outputXBegin = outputXBegin,
                                                    .outputYBegin = outputYBegin,
                                                    .outputWidth = outputWidth,
                                                    .foreground = foregroundValue,
                                                    .background = backgroundValue};
        });
  }

  // The single-slice route above is one shared implementation, so only the 3-D routes record an algorithm path.
  const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  const auto* inMemoryInput = dynamic_cast<const DataStore<T>*>(&inStore);
  auto* inMemoryOutput = dynamic_cast<DataStore<uint8>*>(&outStore);
  const bool direct = !usesOutOfCoreStore && inMemoryInput != nullptr && inMemoryOutput != nullptr && !ForceOocAlgorithm();
  RecordAlgorithmPathExecution(direct ? AlgorithmPath::InCore : AlgorithmPath::OutOfCore, usesOutOfCoreStore);
  if(shouldCancel)
  {
    return {};
  }

  const usize nX = dims[0];
  const usize nY = dims[1];
  const usize nZ = dims[2];
  if(nX == 0 || nY == 0 || nZ == 0)
  {
    return {};
  }
  const usize slice = nX * nY;
  static_cast<void>(messageHandler);

  if(direct)
  {
    const T* values = inMemoryInput->createSpan().data();
    uint8* output = inMemoryOutput->createSpan().data();
    // Planes are dispatched in groups of about k_ZeroCrossingDirectGroupValues values. A group is large enough that
    // the parallel dispatch cost is negligible, and the cancellation check between groups stays responsive.
    const usize planesPerGroup = std::max<usize>(usize{1}, detail::k_ZeroCrossingDirectGroupValues / slice);
    for(usize zBegin = 0; zBegin < nZ; zBegin += planesPerGroup)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize zEnd = std::min(nZ, zBegin + planesPerGroup);
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, (zEnd - zBegin) * nY);
      parallelAlgorithm.execute([values, output, zBegin, nX, nY, nZ, slice, foregroundValue, backgroundValue](const Range& range) {
        for(usize index = range.min(); index < range.max(); ++index)
        {
          const usize z = zBegin + index / nY;
          const usize y = index % nY;
          const usize zPrevious = z > 0 ? z - 1 : z;
          const usize zNext = z + 1 < nZ ? z + 1 : z;
          const usize yPrevious = y > 0 ? y - 1 : y;
          const usize yNext = y + 1 < nY ? y + 1 : y;
          const T* plane = values + z * slice;
          detail::ZeroCrossingRow(values + zPrevious * slice + y * nX, plane + yPrevious * nX, plane + y * nX, plane + yNext * nX, values + zNext * slice + y * nX, output + z * slice + y * nX, nX,
                                  foregroundValue, backgroundValue);
        }
      });
    }
    return {};
  }

  auto usefulResult = detail::ZeroCrossingSlabUsefulBytes<T>(dims);
  if(usefulResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(usefulResult));
  }
  auto reservation = ReserveWorkingMemory(usefulResult.value(), usefulResult.value());
  const detail::ZeroCrossingSlabPlan plan = detail::PlanZeroCrossingSlab<T>(dims, reservation.sizeBytes());

  std::unique_ptr<T[]> input;
  std::unique_ptr<uint8[]> output;
  try
  {
    input = std::make_unique_for_overwrite<T[]>(plan.inputPlanes * slice);
    output = std::make_unique_for_overwrite<uint8[]>(plan.corePlanes * slice);
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(
        -8762,
        fmt::format("Zero crossing could not allocate the planned slab with {} input planes ({} bytes) and {} output planes ({} bytes). The working-memory grant is {} bytes for dimensions ({}).",
                    plan.inputPlanes, plan.inputBytes, plan.corePlanes, plan.outputBytes, reservation.sizeBytes(), StringUtilities::formatDimensions3D(dims)));
  }

  for(usize z0 = 0; z0 < nZ; z0 += plan.corePlanes)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize zLo = z0 > 0 ? z0 - 1 : 0;
    const usize count = std::min(nZ, z0 + plan.corePlanes);
    const usize zHi = std::min(nZ, count + 1);
    const usize inputValueCount = (zHi - zLo) * slice;
    if(Result<> result = inStore.copyIntoBuffer(zLo * slice, nonstd::span<T>(input.get(), inputValueCount)); result.invalid())
    {
      return result;
    }

    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, (count - z0) * nY);
    parallelAlgorithm.execute([inputValues = input.get(), outputValues = output.get(), z0, zLo, zHi, nX, nY, slice, foregroundValue, backgroundValue](const Range& range) {
      for(usize index = range.min(); index < range.max(); ++index)
      {
        const usize z = z0 + index / nY;
        const usize y = index % nY;
        const usize zPrevious = z > zLo ? z - 1 : zLo;
        const usize zNext = z + 1 < zHi ? z + 1 : zHi - 1;
        const usize yPrevious = y > 0 ? y - 1 : y;
        const usize yNext = y + 1 < nY ? y + 1 : y;
        const T* row = inputValues + (z - zLo) * slice + y * nX;
        const T* previousPlaneRow = inputValues + (zPrevious - zLo) * slice + y * nX;
        const T* previousRow = inputValues + (z - zLo) * slice + yPrevious * nX;
        const T* nextRow = inputValues + (z - zLo) * slice + yNext * nX;
        const T* nextPlaneRow = inputValues + (zNext - zLo) * slice + y * nX;
        detail::ZeroCrossingRow(previousPlaneRow, previousRow, row, nextRow, nextPlaneRow, outputValues + (z - z0) * slice + y * nX, nX, foregroundValue, backgroundValue);
      }
    });

    const usize outputValueCount = (count - z0) * slice;
    if(Result<> result = outStore.copyFromBuffer(z0 * slice, nonstd::span<const uint8>(output.get(), outputValueCount)); result.invalid())
    {
      return result;
    }
  }
  return {};
}
} // namespace nx::core::ImageProcessing
