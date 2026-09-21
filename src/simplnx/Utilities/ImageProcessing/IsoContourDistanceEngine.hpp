#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
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
constexpr usize k_IsoContour2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
constexpr usize k_IsoContour2DFixedStateBytes = 4096;

/**
 * @brief Selects the real type that ITK uses for the input type.
 * @tparam T Specifies the input value type.
 */
template <class T>
using IsoRealType = std::conditional_t<std::is_same_v<T, float32>, float32, float64>;

struct IsoContour2DBufferPlan
{
  usize coreRows = 0;
  usize coreCols = 0;
  usize residentBytes = 0;
  bool valid = false;
  bool overflow = false;
};

inline bool IsoContourCheckedAdd(usize left, usize right, usize& result)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  result = left + right;
  return true;
}

inline bool IsoContourCheckedMultiply(usize left, usize right, usize& result)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  result = left * right;
  return true;
}

/**
 * @struct IsoContourSlabPlan
 * @brief Specifies the input halo and output core buffers for one 3-D slab.
 *
 * The working-memory grant bounds each slab when the grant can hold the minimum plan.
 * The standard minimum plan contains five input planes and one output plane, even when the grant is smaller.
 * Thin volumes cap the input depth at the volume depth.
 * A radius-2 halo adds two input planes on each side of the output core.
 * A complete grant processes the volume with one input read and one output write.
 */
struct IsoContourSlabPlan
{
  usize corePlanes = 0;
  usize inputPlanes = 0;
  usize inputBytes = 0;
  usize outputBytes = 0;
};

/**
 * @brief Computes the useful bytes for complete input and float32 output buffers.
 * @tparam T Specifies the input value type.
 * @param dims Gives the image dimensions in X, Y, Z order.
 * @return The useful byte count, or error -8626 if the calculation overflows.
 */
template <class T>
inline Result<usize> IsoContourSlabUsefulBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize bytesPerValue = 0;
  usize requiredBytes = 0;
  if(!IsoContourCheckedMultiply(dims[0], dims[1], sliceValues) || !IsoContourCheckedMultiply(sliceValues, dims[2], volumeValues) || !IsoContourCheckedAdd(sizeof(T), sizeof(float32), bytesPerValue) ||
     !IsoContourCheckedMultiply(volumeValues, bytesPerValue, requiredBytes))
  {
    return MakeErrorResult<usize>(-8626, fmt::format("Iso-contour distance dimensions ({}) and input element size ({} bytes) overflow while sizing complete input and float32 output buffers.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

/**
 * @brief Plans a 3-D slab from the available working-memory grant.
 * @tparam T Specifies the input value type.
 * @param dims Gives the image dimensions in X, Y, Z order.
 * @param grantBytes Gives the working-memory grant in bytes.
 * @return The output core depth and the related input and output buffer sizes.
 * @pre The image depth is greater than one. The 2-D route handles a depth of one.
 *
 * The input depth includes a radius-2 halo on each side of the output core.
 * The standard minimum plan uses five input planes and one output plane, even when this plan exceeds the grant.
 * The input depth never exceeds the volume depth.
 * A grant for the complete buffers produces one slab and permits one input read and one output write.
 */
template <class T>
inline IsoContourSlabPlan PlanIsoContourSlab(const SizeVec3& dims, uint64 grantBytes)
{
  const usize sliceValues = dims[0] * dims[1];
  const usize planeInputBytes = sliceValues * sizeof(T);
  const usize planeOutputBytes = sliceValues * sizeof(float32);
  const usize usefulBytes = dims[2] * (planeInputBytes + planeOutputBytes);
  if(grantBytes >= usefulBytes)
  {
    return {dims[2], dims[2], dims[2] * planeInputBytes, dims[2] * planeOutputBytes};
  }

  const uint64 haloBytes = uint64{4} * planeInputBytes;
  const uint64 bytesPerCorePlane = planeInputBytes + planeOutputBytes;
  const usize quotient = grantBytes > haloBytes ? static_cast<usize>((grantBytes - haloBytes) / bytesPerCorePlane) : 0;
  const usize corePlanes = std::clamp(std::max<usize>(1, quotient), usize{1}, dims[2] - 1);
  const usize inputPlanes = std::min(dims[2], corePlanes + 4);
  return {corePlanes, inputPlanes, inputPlanes * planeInputBytes, corePlanes * planeOutputBytes};
}

/** @brief Gives the maximum value count in one direct-route plane group. */
inline constexpr usize k_IsoContourDirectGroupValues = usize{16} << 20;

/**
 * @brief Classifies one row and evaluates only voxels adjacent to an iso-contour crossing.
 * @tparam T Specifies the input value type.
 * @tparam Gather Specifies the callable that evaluates the complete 3-D kernel.
 * @param previousPlaneRow Gives the row at the negative Z neighbor.
 * @param previousRow Gives the row at the negative Y neighbor.
 * @param row Gives the center row.
 * @param nextRow Gives the row at the positive Y neighbor.
 * @param nextPlaneRow Gives the row at the positive Z neighbor.
 * @param output Receives one output row.
 * @param nx Gives the number of values in each row.
 * @param levelSet Gives the iso-contour value in the selected real type.
 * @param farValue Gives the positive value for voxels above the level set.
 * @param negativeFar Gives the negative value for voxels below the level set.
 * @param gather Evaluates the complete kernel for one X coordinate.
 *
 * Each edge test returns before it reads the candidate minimum when both endpoints share a class.
 * Edges outside the volume are not evaluated.
 * A clamped neighbor is therefore equivalent to the center for this classification.
 */
template <class T, class Gather>
inline void IsoContourRow(const T* previousPlaneRow, const T* previousRow, const T* row, const T* nextRow, const T* nextPlaneRow, float32* output, usize nx, IsoRealType<T> levelSet, float32 farValue,
                          float32 negativeFar, Gather&& gather)
{
  using RealT = IsoRealType<T>;
  const usize lastX = nx - 1;
  const auto classify = [levelSet](T value) -> uint32 { return static_cast<uint32>((static_cast<RealT>(value) - levelSet) > RealT{0}); };
  for(usize x = 0; x < nx; ++x)
  {
    const usize previousX = x > 0 ? x - 1 : 0;
    const usize nextX = x < lastX ? x + 1 : lastX;
    const RealT centerValue = static_cast<RealT>(row[x]);
    const uint32 centerClass = classify(row[x]);
    uint32 classDifference = centerClass ^ classify(row[previousX]);
    classDifference |= centerClass ^ classify(row[nextX]);
    classDifference |= centerClass ^ classify(previousRow[x]);
    classDifference |= centerClass ^ classify(nextRow[x]);
    classDifference |= centerClass ^ classify(previousPlaneRow[x]);
    classDifference |= centerClass ^ classify(nextPlaneRow[x]);
    if(classDifference == 0)
    {
      output[x] = centerValue > levelSet ? farValue : (centerValue < levelSet ? negativeFar : 0.0f);
    }
    else
    {
      output[x] = gather(x);
    }
  }
}

inline bool IsoContour2DFullWidthPeak(usize columns, usize rows, usize inputBytes, usize& peak)
{
  usize inputRows = 0;
  usize inputValues = 0;
  usize outputValues = 0;
  usize input = 0;
  usize output = 0;
  usize total = 0;
  if(!IsoContourCheckedAdd(rows, 4, inputRows) || !IsoContourCheckedMultiply(inputRows, columns, inputValues) || !IsoContourCheckedMultiply(rows, columns, outputValues) ||
     !IsoContourCheckedMultiply(inputValues, inputBytes, input) || !IsoContourCheckedMultiply(outputValues, sizeof(float32), output) || !IsoContourCheckedAdd(input, output, total) ||
     !IsoContourCheckedAdd(total, k_IsoContour2DFixedStateBytes, peak))
  {
    return false;
  }
  return true;
}

inline bool IsoContour2DTiledPeak(usize columns, usize inputBytes, usize& peak)
{
  usize haloColumns = 0;
  usize inputValues = 0;
  usize input = 0;
  usize output = 0;
  usize total = 0;
  if(!IsoContourCheckedAdd(columns, 4, haloColumns) || !IsoContourCheckedMultiply(haloColumns, 5, inputValues) || !IsoContourCheckedMultiply(columns, sizeof(float32), output) ||
     !IsoContourCheckedMultiply(inputValues, inputBytes, input) || !IsoContourCheckedAdd(input, output, total) || !IsoContourCheckedAdd(total, k_IsoContour2DFixedStateBytes, peak))
  {
    return false;
  }
  return true;
}

inline IsoContour2DBufferPlan BuildIsoContour2DBufferPlan(usize nx, usize ny, usize inputBytes, usize residentLimit = k_IsoContour2DResidentLimit)
{
  IsoContour2DBufferPlan plan;
  usize cellCount = 0;
  if(nx == 0 || ny == 0 || inputBytes == 0 || residentLimit == 0 || !IsoContourCheckedMultiply(nx, ny, cellCount))
  {
    plan.overflow = true;
    return plan;
  }

  usize minimumPeak = 0;
  if(!IsoContour2DTiledPeak(1, inputBytes, minimumPeak))
  {
    plan.overflow = true;
    return plan;
  }
  if(minimumPeak > residentLimit)
  {
    return plan;
  }
  auto largestFitting = [residentLimit](usize high, const auto& peakFunction) {
    usize low = 1;
    while(low < high)
    {
      const usize middle = low + (high - low + 1) / 2;
      usize candidatePeak = 0;
      if(peakFunction(middle, candidatePeak) && candidatePeak <= residentLimit)
      {
        low = middle;
      }
      else
      {
        high = middle - 1;
      }
    }
    return low;
  };

  usize oneFullRowPeak = 0;
  if(!IsoContour2DFullWidthPeak(nx, 1, inputBytes, oneFullRowPeak))
  {
    plan.overflow = true;
    return plan;
  }
  if(oneFullRowPeak <= residentLimit)
  {
    plan.coreCols = nx;
    plan.coreRows = largestFitting(ny, [nx, inputBytes](usize rows, usize& candidatePeak) { return IsoContour2DFullWidthPeak(nx, rows, inputBytes, candidatePeak); });
    if(!IsoContour2DFullWidthPeak(plan.coreCols, plan.coreRows, inputBytes, plan.residentBytes))
    {
      plan.overflow = true;
      return plan;
    }
  }
  else
  {
    plan.coreCols = largestFitting(nx, [inputBytes](usize columns, usize& candidatePeak) { return IsoContour2DTiledPeak(columns, inputBytes, candidatePeak); });
    plan.coreRows = 1;
    if(!IsoContour2DTiledPeak(plan.coreCols, inputBytes, plan.residentBytes))
    {
      plan.overflow = true;
      return plan;
    }
  }
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}
} // namespace detail

/**
 * @class IsoContourDistance
 * @brief Computes a narrow-band signed distance around an input iso-contour without ITK.
 * @tparam T Specifies the input value type.
 *
 * Each voxel starts at the signed far value or zero.
 * Voxels adjacent to a crossing receive the minimum-magnitude gradient-interpolated distance.
 * The kernel matches ITK edge clamping, cast order, candidate order, and input-dependent real types.
 *
 * Eligible in-memory stores process row groups directly through resident spans.
 * Other 3-D stores use working-memory slabs with a radius-2 halo and serial bulk transfers.
 * Other 2-D stores use bounded row blocks or X tiles.
 * Parallel workers access only resident buffers or in-memory spans.
 */
template <class T>
class IsoContourDistance
{
public:
  using RealT = detail::IsoRealType<T>;

  IsoContourDistance(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, float64 levelSetValue, float64 farValue, FloatVec3 spacing,
                     const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize residentLimit2D = detail::k_IsoContour2DResidentLimit)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_LevelSet(static_cast<RealT>(levelSetValue))
  , m_FarValue(static_cast<float32>(farValue))
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_ResidentLimit2D(residentLimit2D)
  {
  }
  ~IsoContourDistance() = default;
  IsoContourDistance(const IsoContourDistance&) = delete;
  IsoContourDistance(IsoContourDistance&&) noexcept = delete;
  IsoContourDistance& operator=(const IsoContourDistance&) = delete;
  IsoContourDistance& operator=(IsoContourDistance&&) noexcept = delete;

  Result<> operator()()
  {
    if(m_Dims[0] == 0 || m_Dims[1] == 0 || m_Dims[2] == 0)
    {
      return {};
    }
    if(m_Dims[0] > static_cast<usize>(std::numeric_limits<int64>::max()) || m_Dims[1] > static_cast<usize>(std::numeric_limits<int64>::max()) ||
       m_Dims[2] > static_cast<usize>(std::numeric_limits<int64>::max()))
    {
      return MakeErrorResult(-8620, fmt::format("Iso-contour distance image dimensions exceed the supported signed coordinate range. Dimensions: {} x {} x {}.", m_Dims[0], m_Dims[1], m_Dims[2]));
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
    usize vol = 0;
    if(!checkedMultiply(m_Dims[0], m_Dims[1], slice) || !checkedMultiply(slice, m_Dims[2], vol))
    {
      return MakeErrorResult(-8621, fmt::format("Iso-contour distance image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", m_Dims[0], m_Dims[1], m_Dims[2]));
    }
    if(m_In.getSize() != vol)
    {
      return MakeErrorResult(-8622, fmt::format("Iso-contour distance input store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                                m_In.getSize(), vol, m_Dims[0], m_Dims[1], m_Dims[2]));
    }
    if(m_Out.getSize() != vol)
    {
      return MakeErrorResult(-8623, fmt::format("Iso-contour distance output store size does not match the image dimensions. Actual size: {}; expected size: {}; dimensions: {} x {} x {}.",
                                                m_Out.getSize(), vol, m_Dims[0], m_Dims[1], m_Dims[2]));
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    const int64 nX = static_cast<int64>(m_Dims[0]);
    const int64 nY = static_cast<int64>(m_Dims[1]);
    const int64 nZ = static_cast<int64>(m_Dims[2]);

    // ITK stores spacing as double for every input type.
    // Double spacing preserves ITK promotion rules in the gradient denominator and distance calculation.
    // Float32 input can differ from ITK by one ULP because ITK resolves equal candidates through concurrent writes.
    const double sp[3] = {static_cast<double>(m_Spacing[0]), static_cast<double>(m_Spacing[1]), static_cast<double>(m_Spacing[2])};
    const float32 negFar = -m_FarValue;

    const auto* inMemoryInputStore = dynamic_cast<const DataStore<T>*>(&m_In);
    auto* inMemoryOutputStore = dynamic_cast<DataStore<float32>*>(&m_Out);
    const bool usesOutOfCoreStore = m_In.getStoreType() == IDataStore::StoreType::OutOfCore || m_Out.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool forceSlabs = ForceOocAlgorithm() && !ForceInCoreAlgorithm();
    const bool direct = inMemoryInputStore != nullptr && inMemoryOutputStore != nullptr && !usesOutOfCoreStore && !forceSlabs;
    RecordAlgorithmPathExecution(direct ? AlgorithmPath::InCore : AlgorithmPath::OutOfCore, usesOutOfCoreStore);
    if(direct)
    {
      return RunResident(inMemoryInputStore->createSpan(), inMemoryOutputStore->createSpan(), nX, nY, nZ, slice, sp, negFar);
    }

    if(nZ == 1)
    {
      return RunBounded2D(static_cast<usize>(nX), static_cast<usize>(nY), sp, negFar);
    }

    return RunSlabs(nX, nY, nZ, slice, sp, negFar);
  }

private:
  /**
   * @brief Processes a 3-D image in slabs sized by the shared working-memory grant.
   * @param nX Gives the image width.
   * @param nY Gives the image height.
   * @param nZ Gives the image depth.
   * @param slice Gives the number of values in one plane.
   * @param spacing Gives the physical spacing for each axis.
   * @param negativeFar Gives the negative value for voxels below the level set.
   * @return An error for invalid sizing, allocation, or bulk transfer failures.
   *
   * Each slab has a radius-2 input halo and a disjoint output core.
   * The positive Z edge reads gradient samples through Z-1 and Z+2, which requires the halo.
   * Absolute plane indices map to ring slots, which removes copies of shared halo planes.
   * Each slab uses at most two bulk reads because its new plane range can wrap once.
   * Each completed slab uses one bulk output write. A successful run reads every input plane once.
   */
  Result<> RunSlabs(int64 nX, int64 nY, int64 nZ, usize slice, const double spacing[3], float32 negativeFar)
  {
    auto usefulResult = detail::IsoContourSlabUsefulBytes<T>(m_Dims);
    if(usefulResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(usefulResult));
    }
    const usize usefulBytes = usefulResult.value();
    auto reservation = ReserveWorkingMemory(usefulBytes, usefulBytes);
    const detail::IsoContourSlabPlan plan = detail::PlanIsoContourSlab<T>(m_Dims, reservation.sizeBytes());

    std::unique_ptr<T[]> input;
    std::unique_ptr<float32[]> output;
    try
    {
      input = std::make_unique_for_overwrite<T[]>(plan.inputPlanes * slice);
      output = std::make_unique_for_overwrite<float32[]>(plan.corePlanes * slice);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-8627, fmt::format("Iso-contour distance could not allocate a slab with {} input planes ({} bytes) and {} output planes ({} bytes) from a {}-byte working-memory grant.",
                                                plan.inputPlanes, plan.inputBytes, plan.corePlanes, plan.outputBytes, reservation.sizeBytes()));
    }

    const usize xCount = static_cast<usize>(nX);
    const usize yCount = static_cast<usize>(nY);
    const usize zCount = static_cast<usize>(nZ);
    usize windowLo = 0;
    usize windowHi = 0;
    for(usize z0 = 0; z0 < zCount; z0 += plan.corePlanes)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize needLo = z0 >= 2 ? z0 - 2 : 0;
      const usize zEnd = std::min(zCount, z0 + plan.corePlanes);
      const usize needHi = std::min(zCount, zEnd + 2);
      const usize retainedLo = std::max(needLo, windowLo);
      const usize readLo = std::max(retainedLo, windowHi);
      const usize readPlanes = needHi - readLo;
      if(readPlanes > 0)
      {
        const usize firstSlot = readLo % plan.inputPlanes;
        const usize firstReadPlanes = std::min(readPlanes, plan.inputPlanes - firstSlot);
        if(Result<> result = m_In.copyIntoBuffer(readLo * slice, nonstd::span<T>(input.get() + firstSlot * slice, firstReadPlanes * slice)); result.invalid())
        {
          return result;
        }
        const usize secondReadPlanes = readPlanes - firstReadPlanes;
        if(secondReadPlanes > 0)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const usize secondReadLo = readLo + firstReadPlanes;
          if(Result<> result = m_In.copyIntoBuffer(secondReadLo * slice, nonstd::span<T>(input.get(), secondReadPlanes * slice)); result.invalid())
          {
            return result;
          }
        }
      }
      windowLo = needLo;
      windowHi = needHi;

      const usize rowCount = (zEnd - z0) * yCount;
      auto processRows = [&](const Range& range) {
        for(usize rowIndex = range.min(); rowIndex < range.max(); ++rowIndex)
        {
          // One atomic load per row keeps cancellation responsive inside a large slab or plane group.
          if(m_ShouldCancel)
          {
            return;
          }
          const usize localZ = rowIndex / yCount;
          const usize yIndex = rowIndex - localZ * yCount;
          const usize zIndex = z0 + localZ;
          const usize previousZ = zIndex > 0 ? zIndex - 1 : 0;
          const usize nextZ = std::min(zIndex + 1, zCount - 1);
          const usize previousY = yIndex > 0 ? yIndex - 1 : 0;
          const usize nextY = std::min(yIndex + 1, yCount - 1);
          const T* previousPlaneRow = input.get() + (previousZ % plan.inputPlanes) * slice + yIndex * xCount;
          const T* previousRow = input.get() + (zIndex % plan.inputPlanes) * slice + previousY * xCount;
          const T* row = input.get() + (zIndex % plan.inputPlanes) * slice + yIndex * xCount;
          const T* nextRow = input.get() + (zIndex % plan.inputPlanes) * slice + nextY * xCount;
          const T* nextPlaneRow = input.get() + (nextZ % plan.inputPlanes) * slice + yIndex * xCount;
          float32* outputRow = output.get() + localZ * slice + yIndex * xCount;
          const int64 y = static_cast<int64>(yIndex);
          const int64 z = static_cast<int64>(zIndex);
          auto gather = [&](usize xIndex) {
            const int64 x = static_cast<int64>(xIndex);
            auto relativeReal = [&](int64 deltaX, int64 deltaY, int64 deltaZ) -> RealT {
              const usize sampleX = static_cast<usize>(std::clamp<int64>(x + deltaX, 0, nX - 1));
              const usize sampleY = static_cast<usize>(std::clamp<int64>(y + deltaY, 0, nY - 1));
              const usize sampleZ = static_cast<usize>(std::clamp<int64>(z + deltaZ, 0, nZ - 1));
              return static_cast<RealT>(input[(sampleZ % plan.inputPlanes) * slice + sampleY * xCount + sampleX]);
            };
            auto relativeFloat = [&](int64 deltaX, int64 deltaY, int64 deltaZ) -> float32 {
              const usize sampleX = static_cast<usize>(std::clamp<int64>(x + deltaX, 0, nX - 1));
              const usize sampleY = static_cast<usize>(std::clamp<int64>(y + deltaY, 0, nY - 1));
              const usize sampleZ = static_cast<usize>(std::clamp<int64>(z + deltaZ, 0, nZ - 1));
              return static_cast<float32>(input[(sampleZ % plan.inputPlanes) * slice + sampleY * xCount + sampleX]);
            };
            return Evaluate3DGather(relativeReal, relativeFloat, x, y, z, nX, nY, nZ, spacing, negativeFar);
          };
          detail::IsoContourRow(previousPlaneRow, previousRow, row, nextRow, nextPlaneRow, outputRow, xCount, m_LevelSet, m_FarValue, negativeFar, gather);
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, rowCount);
      parallelAlgorithm.execute(processRows);

      if(m_ShouldCancel)
      {
        return {};
      }
      const usize outputValues = (zEnd - z0) * slice;
      if(Result<> result = m_Out.copyFromBuffer(z0 * slice, nonstd::span<const float32>(output.get(), outputValues)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  /**
   * @brief Processes resident input and output spans in parallel plane groups.
   * @param input Gives the complete input volume.
   * @param output Receives the complete output volume.
   * @param nX Gives the image width.
   * @param nY Gives the image height.
   * @param nZ Gives the image depth.
   * @param slice Gives the number of values in one plane.
   * @param spacing Gives the physical spacing for each axis.
   * @param negativeFar Gives the negative value for voxels below the level set.
   * @return An empty valid result after completion or cancellation.
   *
   * The algorithm checks cancellation between plane groups and once per row inside each group.
   * The algorithm never checks cancellation per voxel.
   */
  Result<> RunResident(nonstd::span<const T> input, nonstd::span<float32> output, int64 nX, int64 nY, int64 nZ, usize slice, const double spacing[3], float32 negativeFar)
  {
    auto clampCoordinate = [](int64 value, int64 high) { return value < 0 ? int64{0} : (value > high ? high : value); };
    auto sampleReal = [&](int64 x, int64 y, int64 z) -> RealT {
      const usize index = static_cast<usize>((clampCoordinate(z, nZ - 1) * nY + clampCoordinate(y, nY - 1)) * nX + clampCoordinate(x, nX - 1));
      return static_cast<RealT>(input[index]);
    };
    auto sampleFloat = [&](int64 x, int64 y, int64 z) -> float32 {
      const usize index = static_cast<usize>((clampCoordinate(z, nZ - 1) * nY + clampCoordinate(y, nY - 1)) * nX + clampCoordinate(x, nX - 1));
      return static_cast<float32>(input[index]);
    };
    const usize xCount = static_cast<usize>(nX);
    const usize yCount = static_cast<usize>(nY);
    const usize zCount = static_cast<usize>(nZ);
    const usize planesPerGroup = std::max<usize>(1, detail::k_IsoContourDirectGroupValues / slice);
    for(usize zBegin = 0; zBegin < zCount; zBegin += planesPerGroup)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize zEnd = std::min(zCount, zBegin + planesPerGroup);
      const usize rowCount = (zEnd - zBegin) * yCount;
      auto processRows = [&](const Range& range) {
        for(usize rowIndex = range.min(); rowIndex < range.max(); ++rowIndex)
        {
          // One atomic load per row keeps cancellation responsive inside a large slab or plane group.
          if(m_ShouldCancel)
          {
            return;
          }
          const usize localZ = rowIndex / yCount;
          const usize yIndex = rowIndex - localZ * yCount;
          const usize zIndex = zBegin + localZ;
          const usize previousZ = zIndex > 0 ? zIndex - 1 : 0;
          const usize nextZ = std::min(zIndex + 1, zCount - 1);
          const usize previousY = yIndex > 0 ? yIndex - 1 : 0;
          const usize nextY = std::min(yIndex + 1, yCount - 1);
          const T* previousPlaneRow = input.data() + previousZ * slice + yIndex * xCount;
          const T* previousRow = input.data() + zIndex * slice + previousY * xCount;
          const T* row = input.data() + zIndex * slice + yIndex * xCount;
          const T* nextRow = input.data() + zIndex * slice + nextY * xCount;
          const T* nextPlaneRow = input.data() + nextZ * slice + yIndex * xCount;
          float32* outputRow = output.data() + zIndex * slice + yIndex * xCount;
          const int64 y = static_cast<int64>(yIndex);
          const int64 z = static_cast<int64>(zIndex);
          auto gather = [&](usize xIndex) {
            const int64 x = static_cast<int64>(xIndex);
            auto relativeReal = [&](int64 deltaX, int64 deltaY, int64 deltaZ) -> RealT { return sampleReal(x + deltaX, y + deltaY, z + deltaZ); };
            auto relativeFloat = [&](int64 deltaX, int64 deltaY, int64 deltaZ) -> float32 { return sampleFloat(x + deltaX, y + deltaY, z + deltaZ); };
            return Evaluate3DGather(relativeReal, relativeFloat, x, y, z, nX, nY, nZ, spacing, negativeFar);
          };
          detail::IsoContourRow(previousPlaneRow, previousRow, row, nextRow, nextPlaneRow, outputRow, xCount, m_LevelSet, m_FarValue, negativeFar, gather);
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, rowCount);
      parallelAlgorithm.execute(processRows);
      if(m_ShouldCancel)
      {
        return {};
      }
    }
    return {};
  }

  template <class SampleReal, class SampleFloat>
  float32 Evaluate2DGather(SampleReal&& sampleReal, SampleFloat&& sampleFloat, int64 x, int64 y, int64 nx, int64 ny, const double spacing[3], float32 negativeFar) const
  {
    const RealT centerValue = sampleReal(0, 0);
    float32 best = centerValue > m_LevelSet ? m_FarValue : (centerValue < m_LevelSet ? negativeFar : 0.0f);
    auto evaluateEdge = [&](int64 edgeX, int64 edgeY, int32 axis, bool targetIsFirst) {
      const int64 neighborX = edgeX + (axis == 0 ? 1 : 0);
      const int64 neighborY = edgeY + (axis == 1 ? 1 : 0);
      const RealT val0 = sampleReal(edgeX, edgeY) - m_LevelSet;
      const RealT val1 = sampleReal(neighborX, neighborY) - m_LevelSet;
      const bool sign = val0 > RealT{0};
      if(sign == (val1 > RealT{0}))
      {
        return;
      }

      RealT grad0[3];
      RealT grad1[3];
      for(int32 gradientAxis = 0; gradientAxis < 3; ++gradientAxis)
      {
        const int64 gradientX = gradientAxis == 0 ? 1 : 0;
        const int64 gradientY = gradientAxis == 1 ? 1 : 0;
        grad0[gradientAxis] = sampleReal(edgeX + gradientX, edgeY + gradientY) - sampleReal(edgeX - gradientX, edgeY - gradientY);
        const float32 positive = sampleFloat(neighborX + gradientX, neighborY + gradientY);
        const float32 negative = sampleFloat(neighborX - gradientX, neighborY - gradientY);
        grad1[gradientAxis] = static_cast<RealT>(positive - negative);
      }

      const RealT difference = sign ? (val0 - val1) : (val1 - val0);
      if(difference < std::numeric_limits<RealT>::min())
      {
        return;
      }
      RealT gradient[3];
      RealT norm = RealT{0};
      for(int32 gradientAxis = 0; gradientAxis < 3; ++gradientAxis)
      {
        gradient[gradientAxis] = (grad0[gradientAxis] * RealT{0.5} + grad1[gradientAxis] * RealT{0.5}) / (RealT{2} * spacing[gradientAxis]);
        norm += gradient[gradientAxis] * gradient[gradientAxis];
      }
      norm = std::sqrt(norm);
      if(norm < std::numeric_limits<RealT>::min())
      {
        return;
      }
      const RealT scale = std::abs(gradient[axis]) * spacing[axis] / norm / difference;
      const RealT candidate = (targetIsFirst ? val0 : val1) * scale;
      UpdateMinimum(best, candidate);
    };

    if(y > 0)
    {
      evaluateEdge(0, -1, 1, false);
    }
    if(x > 0)
    {
      evaluateEdge(-1, 0, 0, false);
    }
    if(x + 1 < nx)
    {
      evaluateEdge(0, 0, 0, true);
    }
    if(y + 1 < ny)
    {
      evaluateEdge(0, 0, 1, true);
    }
    return best;
  }

  static void UpdateMinimum(float32& target, RealT candidate)
  {
    if(std::abs(static_cast<double>(candidate)) < std::abs(static_cast<double>(target)))
    {
      target = static_cast<float32>(candidate);
    }
  }

  /**
   * @brief Evaluates the six crossing edges for one 3-D voxel in a fixed order.
   * @tparam SampleReal Specifies the real-type sampler for offsets from the voxel.
   * @tparam SampleFloat Specifies the float32 sampler for offsets from the voxel.
   * @param sampleReal Reads one relative offset in the input-dependent real type.
   * @param sampleFloat Reads one relative offset as float32.
   * @param x Gives the voxel X coordinate.
   * @param y Gives the voxel Y coordinate.
   * @param z Gives the voxel Z coordinate.
   * @param nx Gives the image width.
   * @param ny Gives the image height.
   * @param nz Gives the image depth.
   * @param spacing Gives the physical spacing for each axis.
   * @param negativeFar Gives the negative value for voxels below the level set.
   * @return The minimum-magnitude signed candidate or the signed far value.
   *
   * The order is negative Z, Y, and X, followed by positive X, Y, and Z.
   * Strict minimum updates preserve the first candidate when magnitudes are equal.
   * Relative samplers let resident and slab routes use the same kernel.
   */
  template <class SampleReal, class SampleFloat>
  float32 Evaluate3DGather(SampleReal&& sampleReal, SampleFloat&& sampleFloat, int64 x, int64 y, int64 z, int64 nx, int64 ny, int64 nz, const double spacing[3], float32 negativeFar) const
  {
    const RealT centerValue = sampleReal(0, 0, 0);
    float32 best = centerValue > m_LevelSet ? m_FarValue : (centerValue < m_LevelSet ? negativeFar : 0.0f);

    auto evaluateEdge = [&](int64 edgeX, int64 edgeY, int64 edgeZ, int32 axis, bool targetIsFirst) {
      const int64 neighborX = edgeX + (axis == 0 ? 1 : 0);
      const int64 neighborY = edgeY + (axis == 1 ? 1 : 0);
      const int64 neighborZ = edgeZ + (axis == 2 ? 1 : 0);
      const RealT val0 = sampleReal(edgeX, edgeY, edgeZ) - m_LevelSet;
      const RealT val1 = sampleReal(neighborX, neighborY, neighborZ) - m_LevelSet;
      const bool sign = val0 > RealT{0};
      if(sign == (val1 > RealT{0}))
      {
        return;
      }

      RealT gradient0[3];
      RealT gradient1[3];
      for(int32 gradientAxis = 0; gradientAxis < 3; ++gradientAxis)
      {
        const int64 gradientX = gradientAxis == 0 ? 1 : 0;
        const int64 gradientY = gradientAxis == 1 ? 1 : 0;
        const int64 gradientZ = gradientAxis == 2 ? 1 : 0;
        gradient0[gradientAxis] = sampleReal(edgeX + gradientX, edgeY + gradientY, edgeZ + gradientZ) - sampleReal(edgeX - gradientX, edgeY - gradientY, edgeZ - gradientZ);
        const float32 positive = sampleFloat(neighborX + gradientX, neighborY + gradientY, neighborZ + gradientZ);
        const float32 negative = sampleFloat(neighborX - gradientX, neighborY - gradientY, neighborZ - gradientZ);
        gradient1[gradientAxis] = static_cast<RealT>(positive - negative);
      }

      const RealT difference = sign ? (val0 - val1) : (val1 - val0);
      if(difference < std::numeric_limits<RealT>::min())
      {
        return;
      }
      RealT gradient[3];
      RealT norm = RealT{0};
      for(int32 gradientAxis = 0; gradientAxis < 3; ++gradientAxis)
      {
        gradient[gradientAxis] = (gradient0[gradientAxis] * RealT{0.5} + gradient1[gradientAxis] * RealT{0.5}) / (RealT{2} * spacing[gradientAxis]);
        norm += gradient[gradientAxis] * gradient[gradientAxis];
      }
      norm = std::sqrt(norm);
      if(norm < std::numeric_limits<RealT>::min())
      {
        return;
      }
      const RealT scale = std::abs(gradient[axis]) * spacing[axis] / norm / difference;
      const RealT candidate = (targetIsFirst ? val0 : val1) * scale;
      UpdateMinimum(best, candidate);
    };

    if(z > 0)
    {
      evaluateEdge(0, 0, -1, 2, false);
    }
    if(y > 0)
    {
      evaluateEdge(0, -1, 0, 1, false);
    }
    if(x > 0)
    {
      evaluateEdge(-1, 0, 0, 0, false);
    }
    if(x + 1 < nx)
    {
      evaluateEdge(0, 0, 0, 0, true);
    }
    if(y + 1 < ny)
    {
      evaluateEdge(0, 0, 0, 1, true);
    }
    if(z + 1 < nz)
    {
      evaluateEdge(0, 0, 0, 2, true);
    }
    return best;
  }

  Result<> RunBounded2DFullWidth(usize nx, usize ny, usize coreRows, const double spacing[3], float32 negativeFar)
  {
    std::vector<T> inputBlock((coreRows + 4) * nx);
    std::vector<float32> outputBlock(coreRows * nx);

    for(usize yBegin = 0; yBegin < ny; yBegin += coreRows)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(coreRows, ny - yBegin);
      const usize yEnd = yBegin + rowCount;
      const usize loadBegin = yBegin > 1 ? yBegin - 2 : 0;
      const usize loadEnd = std::min(ny, yEnd + 2);
      const usize loadRows = loadEnd - loadBegin;
      if(Result<> result = m_In.copyIntoBuffer(loadBegin * nx, nonstd::span<T>(inputBlock.data(), loadRows * nx)); result.invalid())
      {
        return result;
      }

      const usize coreCellCount = rowCount * nx;
      auto processBlock = [&](const Range& range) {
        for(usize localIndex = range.min(); localIndex < range.max(); ++localIndex)
        {
          const usize localRow = localIndex / nx;
          const int64 globalY = static_cast<int64>(yBegin + localRow);
          const int64 x = static_cast<int64>(localIndex - localRow * nx);
          auto sampleReal = [&](int64 deltaX, int64 deltaY) -> RealT {
            const int64 sampleX = std::clamp<int64>(x + deltaX, 0, static_cast<int64>(nx) - 1);
            const int64 sampleY = std::clamp<int64>(globalY + deltaY, 0, static_cast<int64>(ny) - 1);
            return static_cast<RealT>(inputBlock[(static_cast<usize>(sampleY) - loadBegin) * nx + static_cast<usize>(sampleX)]);
          };
          auto sampleFloat = [&](int64 deltaX, int64 deltaY) -> float32 {
            const int64 sampleX = std::clamp<int64>(x + deltaX, 0, static_cast<int64>(nx) - 1);
            const int64 sampleY = std::clamp<int64>(globalY + deltaY, 0, static_cast<int64>(ny) - 1);
            return static_cast<float32>(inputBlock[(static_cast<usize>(sampleY) - loadBegin) * nx + static_cast<usize>(sampleX)]);
          };
          outputBlock[localIndex] = Evaluate2DGather(sampleReal, sampleFloat, x, globalY, static_cast<int64>(nx), static_cast<int64>(ny), spacing, negativeFar);
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, coreCellCount);
      parallelAlgorithm.execute(processBlock);

      if(Result<> result = m_Out.copyFromBuffer(yBegin * nx, nonstd::span<const float32>(outputBlock.data(), coreCellCount)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  Result<> RunBounded2DTiled(usize nx, usize ny, usize coreCols, const double spacing[3], float32 negativeFar)
  {
    const usize inputStride = coreCols + 4;
    std::vector<T> inputWindow(5 * inputStride);
    std::vector<float32> output(coreCols);
    for(usize y = 0; y < ny; ++y)
    {
      for(usize xBegin = 0; xBegin < nx; xBegin += coreCols)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize columnCount = std::min(coreCols, nx - xBegin);
        const usize xEnd = xBegin + columnCount;
        const usize haloBegin = xBegin > 2 ? xBegin - 2 : 0;
        const usize haloEnd = std::min(nx, xEnd + 2);
        const usize haloCount = haloEnd - haloBegin;
        for(int64 rowOffset = -2; rowOffset <= 2; ++rowOffset)
        {
          const usize sampleY = static_cast<usize>(std::clamp<int64>(static_cast<int64>(y) + rowOffset, 0, static_cast<int64>(ny) - 1));
          if(Result<> result = m_In.copyIntoBuffer(sampleY * nx + haloBegin, nonstd::span<T>(inputWindow.data() + static_cast<usize>(rowOffset + 2) * inputStride, haloCount)); result.invalid())
          {
            return result;
          }
        }

        auto processColumns = [&](const Range& range) {
          for(usize localX = range.min(); localX < range.max(); ++localX)
          {
            const int64 globalX = static_cast<int64>(xBegin + localX);
            auto sampleReal = [&](int64 deltaX, int64 deltaY) -> RealT {
              const usize sampleX = static_cast<usize>(std::clamp<int64>(globalX + deltaX, 0, static_cast<int64>(nx) - 1));
              const usize inputRow = static_cast<usize>(deltaY + 2);
              return static_cast<RealT>(inputWindow[inputRow * inputStride + sampleX - haloBegin]);
            };
            auto sampleFloat = [&](int64 deltaX, int64 deltaY) -> float32 {
              const usize sampleX = static_cast<usize>(std::clamp<int64>(globalX + deltaX, 0, static_cast<int64>(nx) - 1));
              const usize inputRow = static_cast<usize>(deltaY + 2);
              return static_cast<float32>(inputWindow[inputRow * inputStride + sampleX - haloBegin]);
            };
            output[localX] = Evaluate2DGather(sampleReal, sampleFloat, globalX, static_cast<int64>(y), static_cast<int64>(nx), static_cast<int64>(ny), spacing, negativeFar);
          }
        };
        ParallelDataAlgorithm parallelAlgorithm;
        parallelAlgorithm.setRange(0, columnCount);
        parallelAlgorithm.execute(processColumns);

        if(Result<> result = m_Out.copyFromBuffer(y * nx + xBegin, nonstd::span<const float32>(output.data(), columnCount)); result.invalid())
        {
          return result;
        }
      }
    }
    return {};
  }

  Result<> RunBounded2D(usize nx, usize ny, const double spacing[3], float32 negativeFar)
  {
    const detail::IsoContour2DBufferPlan plan = detail::BuildIsoContour2DBufferPlan(nx, ny, sizeof(T), m_ResidentLimit2D);
    if(plan.overflow)
    {
      return MakeErrorResult(-8624, fmt::format("Iso-contour distance 2D buffer plan cannot represent dimensions {} x {} due to arithmetic overflow.", nx, ny));
    }
    if(!plan.valid)
    {
      return MakeErrorResult(
          -8625, fmt::format("Iso-contour distance 2D buffer plan exceeds the {}-byte resident limit for dimensions {} x {} and {}-byte input values.", m_ResidentLimit2D, nx, ny, sizeof(T)));
    }
    if(plan.coreCols == nx)
    {
      usize blockRows = plan.coreRows;
      const std::optional<ShapeType> outputChunkShape = m_Out.getChunkShape();
      if(outputChunkShape.has_value() && outputChunkShape->size() >= 3 && (*outputChunkShape)[0] == 1 && (*outputChunkShape)[1] > 0 && (*outputChunkShape)[1] <= blockRows &&
         (*outputChunkShape)[2] == nx)
      {
        blockRows = (blockRows / (*outputChunkShape)[1]) * (*outputChunkShape)[1];
      }
      return RunBounded2DFullWidth(nx, ny, blockRows, spacing, negativeFar);
    }
    return RunBounded2DTiled(nx, ny, plan.coreCols, spacing, negativeFar);
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  RealT m_LevelSet;
  float32 m_FarValue;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_ResidentLimit2D = detail::k_IsoContour2DResidentLimit;
};

/**
 * @brief Applies the IsoContour narrow-band signed-distance algorithm.
 * @tparam T Specifies the input value type.
 * @param inStore Gives the scalar input store.
 * @param outStore Receives float32 signed distances.
 * @param dims Gives the image dimensions in X, Y, Z order.
 * @param levelSetValue Gives the iso-contour value.
 * @param farValue Gives the magnitude for voxels away from the crossing surface.
 * @param spacing Gives the physical spacing for each axis.
 * @param shouldCancel Requests cancellation between plane groups or slabs and once per row inside them.
 * @param messageHandler Receives algorithm messages.
 * @return An error for invalid input, allocation, or transfer failures.
 */
template <class T>
Result<> ApplyIsoContourDistance(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, float64 levelSetValue, float64 farValue, FloatVec3 spacing,
                                 const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  IsoContourDistance<T> engine(inStore, outStore, dims, levelSetValue, farValue, spacing, shouldCancel, messageHandler);
  return engine();
}
} // namespace nx::core::ImageProcessing
