#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
constexpr usize k_Chamfer2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
constexpr usize k_Chamfer2DFixedStateBytes = 4096;
constexpr usize k_Chamfer2DMaxTileRecords = 5;
inline constexpr usize k_ChamferRowGroupRows = 8;

struct Chamfer2DBufferPlan
{
  usize coreRows = 0;
  usize coreCols = 0;
  usize residentBytes = 0;
  bool valid = false;
  bool overflow = false;
};

inline bool ChamferCheckedAdd(usize left, usize right, usize& result)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  result = left + right;
  return true;
}

inline bool ChamferCheckedMultiply(usize left, usize right, usize& result)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  result = left * right;
  return true;
}

struct FastChamferResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseFastChamferResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

inline Result<usize> CalculateFastChamferResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  // The resident route propagates in place on one float32 copy of the volume, so that copy is the whole working state.
  usize sliceValues = 0;
  usize volumeValues = 0;
  usize requiredBytes = 0;
  if(!ChamferCheckedMultiply(dims[0], dims[1], sliceValues) || !ChamferCheckedMultiply(sliceValues, dims[2], volumeValues) || !ChamferCheckedMultiply(volumeValues, sizeof(float32), requiredBytes))
  {
    return MakeErrorResult<usize>(-8676, fmt::format("Fast chamfer distance dimensions ({}) overflow while sizing the resident float32 image.", StringUtilities::formatDimensions3D(dims)));
  }
  return {requiredBytes};
}

inline Result<FastChamferResidentMemoryAllocation> ReserveFastChamferResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateFastChamferResidentWorkingMemoryBytes(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<FastChamferResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {FastChamferResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

inline bool Chamfer2DFullWidthPeak(usize columns, usize rows, usize& peak)
{
  usize rowsWithHalo = 0;
  usize cells = 0;
  usize valuesBytes = 0;
  return ChamferCheckedAdd(rows, 1, rowsWithHalo) && ChamferCheckedMultiply(columns, rowsWithHalo, cells) && ChamferCheckedMultiply(cells, sizeof(float32), valuesBytes) &&
         ChamferCheckedAdd(valuesBytes, k_Chamfer2DFixedStateBytes, peak);
}

inline bool Chamfer2DTiledPeak(usize columns, usize& peak)
{
  usize values = 0;
  usize valuesBytes = 0;
  return ChamferCheckedMultiply(columns, k_Chamfer2DMaxTileRecords, values) && ChamferCheckedMultiply(values, sizeof(float32), valuesBytes) &&
         ChamferCheckedAdd(valuesBytes, k_Chamfer2DFixedStateBytes, peak);
}

inline Chamfer2DBufferPlan BuildChamfer2DBufferPlan(usize nx, usize ny, usize residentLimit = k_Chamfer2DResidentLimit)
{
  Chamfer2DBufferPlan plan;
  usize cellCount = 0;
  if(nx == 0 || ny == 0 || residentLimit == 0 || nx > static_cast<usize>(std::numeric_limits<int64>::max()) || ny > static_cast<usize>(std::numeric_limits<int64>::max()) ||
     !ChamferCheckedMultiply(nx, ny, cellCount))
  {
    plan.overflow = true;
    return plan;
  }

  usize minimumPeak = 0;
  if(!Chamfer2DTiledPeak(1, minimumPeak))
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

  usize oneRowPeak = 0;
  if(Chamfer2DFullWidthPeak(nx, 1, oneRowPeak) && oneRowPeak <= residentLimit)
  {
    plan.coreCols = nx;
    plan.coreRows = largestFitting(ny, [nx](usize rows, usize& peak) { return Chamfer2DFullWidthPeak(nx, rows, peak); });
    if(!Chamfer2DFullWidthPeak(nx, plan.coreRows, plan.residentBytes))
    {
      plan.overflow = true;
      return plan;
    }
  }
  else
  {
    plan.coreCols = largestFitting(nx, [](usize columns, usize& peak) { return Chamfer2DTiledPeak(columns, peak); });
    plan.coreRows = 1;
    if(!Chamfer2DTiledPeak(plan.coreCols, plan.residentBytes))
    {
      plan.overflow = true;
      return plan;
    }
  }
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}

// Optimized chamfer weights (itk::FastChamferDistanceImageFilter defaults) indexed by neighbor "diagonality"
// nt = (|dx|+|dy|+|dz|)-1: axis, face-diagonal, cube-diagonal. This array is the single source of the unitless weights.
inline constexpr std::array<float32, 3> k_ChamferWeights = {0.92644f, 1.34065f, 1.65849f};
} // namespace detail

/**
 * @class FastChamferDistance
 * @brief Propagates a narrow signed band into an approximate signed distance map with the ITK two-pass chamfer recurrence.
 *
 * The forward scan pushes each source to the 13 neighbors whose unit offsets satisfy `9dz + 3dy + dx > 0`.
 * The backward scan uses the negated offsets. Each scan preserves the serial source order for every target voxel.
 * Values at or beyond the positive or negative maximum distance do not propagate. Pushed values are not clamped.
 *
 * Each 3-D kernel partitions a plane into full-width groups of `g` rows. Forward group `(Y,z)` has level `3z + Y`.
 * Cross-group pushes reach levels `L+1`, `L+2`, `L+3`, or `L+4`. Thus, each group receives all pushes before it runs.
 * Same-level groups `(Y,z)` and `(Y-3,z+1)` have disjoint source and target rows.
 * The first group targets `Y` and `Y+1` in plane `z`. It targets `Y-1` through `Y+1` in plane `z+1`.
 * The second group targets `Y-3` and `Y-2` in plane `z+1`. It targets `Y-4` through `Y-2` in plane `z+2`.
 * The groups cannot be a source or target for each other, so the parallel work has no data races.
 *
 * For a target voxel, plane `z-1` sources arrive from groups `Y-1`, `Y`, and `Y+1` at levels `L-4` through `L-2`.
 * Plane `z` sources then arrive from groups `Y-1` and `Y` at levels `L-1` and `L`.
 * Rows and columns keep raster order inside each group. Each float32 minimum or maximum fold is bit-identical to the serial scan.
 * The backward scan mirrors the plane and group coordinates and processes rows and columns in descending order.
 *
 * Full-width groups prevent cyclic dependencies between x partitions. A `(-1,+1,0)` push can enter the left partition from a first column.
 * That partition can push back with `(+1,0,0)`. Therefore, an arbitrary 3-D block is not a safe parallel work unit.
 *
 * A resident DataStore supplies a direct span, so the kernels update the array without plane copies.
 * Other in-memory stores use one resident copy because they do not supply a direct span.
 * The bounded 3-D route uses the same kernels in 16-plane blocks and keeps one target-only halo plane.
 * The 2-D route uses bounded full-width row blocks or a five-record tile window.
 * Sign inversion occurs after each backward source group is final. Halo planes are not negated.
 * The algorithm checks cancellation between wavefront levels and bounded blocks.
 */
class FastChamferDistance
{
public:
  /**
   * @brief Creates an in-place chamfer propagator.
   * @param store Stores the float32 signed band and receives the distance map.
   * @param dims Gives the image dimensions in X, Y, Z order.
   * @param maxDist Freezes source values at or beyond this positive or negative magnitude.
   * @param shouldCancel Requests a valid early return when set.
   * @param messageHandler Receives filter messages.
   * @param residentLimit2D Limits resident memory for bounded 2-D processing, in bytes.
   * @param negateOutput Selects sign inversion during final backward processing.
   * @param rowGroupRows Sets the row count in each full-width wavefront group. Values less than one select one row.
   */
  FastChamferDistance(AbstractDataStore<float32>& store, SizeVec3 dims, float32 maxDist, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                      usize residentLimit2D = detail::k_Chamfer2DResidentLimit, bool negateOutput = false, usize rowGroupRows = detail::k_ChamferRowGroupRows)
  : m_Store(store)
  , m_Dims(dims)
  , m_MaxDist(maxDist)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_ResidentLimit2D(residentLimit2D)
  , m_NegateOutput(negateOutput)
  , m_RowGroupRows(std::max<usize>(usize{1}, rowGroupRows))
  {
  }
  ~FastChamferDistance() = default;
  FastChamferDistance(const FastChamferDistance&) = delete;
  FastChamferDistance(FastChamferDistance&&) noexcept = delete;
  FastChamferDistance& operator=(const FastChamferDistance&) = delete;
  FastChamferDistance& operator=(FastChamferDistance&&) noexcept = delete;

  /**
   * @brief Runs the two chamfer scans on the selected storage route.
   * @return An error for invalid storage sizes, failed transfers, or failed allocations. Cancellation returns a valid result.
   */
  Result<> operator()()
  {
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize dimZ = m_Dims[2];
    if(dimX == 0 || dimY == 0 || dimZ == 0)
    {
      return {};
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    if(m_Store.getStoreType() == IDataStore::StoreType::OutOfCore)
    {
      if(dimZ == 1)
      {
        return RunOutOfCore2D(dimX, dimY);
      }

      if(detail::ShouldUseFastChamferResidentState(m_Dims))
      {
        auto allocationResult = detail::ReserveFastChamferResidentWorkingMemory(m_Dims);
        if(allocationResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(allocationResult));
        }
        auto allocation = std::move(allocationResult.value());
        if(allocation.holdsCompleteState())
        {
          try
          {
            DataStore<float32> residentStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, std::nullopt);
            nonstd::span<float32> residentValues = residentStore.createSpan();
            if(Result<> result = m_Store.copyIntoBuffer(0, residentValues); result.invalid())
            {
              return result;
            }
            FastChamferDistance residentEngine(residentStore, m_Dims, m_MaxDist, m_ShouldCancel, m_MessageHandler, m_ResidentLimit2D, m_NegateOutput, m_RowGroupRows);
            if(Result<> result = residentEngine(); result.invalid())
            {
              return result;
            }
            if(m_ShouldCancel)
            {
              return {};
            }
            return m_Store.copyFromBuffer(0, nonstd::span<const float32>(residentValues.data(), residentValues.size()));
          } catch(const std::bad_alloc&)
          {
            // Release the complete-state reservation before entering the bounded fallback.
          }
        }
      }
      return RunOutOfCore3D(dimX, dimY, dimZ);
    }

    usize slice = 0;
    usize volume = 0;
    if(!detail::ChamferCheckedMultiply(dimX, dimY, slice) || !detail::ChamferCheckedMultiply(slice, dimZ, volume))
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance image dimensions overflow the addressable value count: {} x {} x {}.", dimX, dimY, dimZ));
    }
    if(m_Store.getSize() != volume)
    {
      return MakeErrorResult(
          -8671, fmt::format("Fast chamfer distance store size ({}) does not match the expected image size ({}) for dimensions {} x {} x {}.", m_Store.getSize(), volume, dimX, dimY, dimZ));
    }

    if(auto* resident = dynamic_cast<DataStore<float32>*>(&m_Store); resident != nullptr)
    {
      float32* values = resident->createSpan().data();
      if(!PropagateForwardWavefront(values, dimX, dimY, dimZ, dimZ))
      {
        return {};
      }
      if(!PropagateBackwardWavefront(values, dimX, dimY, 0, dimZ, m_NegateOutput))
      {
        return {};
      }
      return {};
    }

    try
    {
      DataStore<float32> residentStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, std::nullopt);
      nonstd::span<float32> residentValues = residentStore.createSpan();
      if(Result<> result = m_Store.copyIntoBuffer(0, residentValues); result.invalid())
      {
        return result;
      }
      if(!PropagateForwardWavefront(residentValues.data(), dimX, dimY, dimZ, dimZ))
      {
        return {};
      }
      if(!PropagateBackwardWavefront(residentValues.data(), dimX, dimY, 0, dimZ, m_NegateOutput))
      {
        return {};
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      return m_Store.copyFromBuffer(0, nonstd::span<const float32>(residentValues.data(), residentValues.size()));
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-8672, fmt::format("Fast chamfer distance could not allocate a resident float32 working store for dimensions {} x {} x {}.", dimX, dimY, dimZ));
    }
  }

private:
  template <class T>
  static std::string DescribeResultError(const Result<T>& result)
  {
    if(result.errors().empty())
    {
      return "provider returned an unspecified error";
    }
    const Error& error = result.errors().front();
    return fmt::format("{} (provider code {})", error.message, error.code);
  }

  static nonstd::span<std::byte> AsWritableBytes(nonstd::span<float32> values)
  {
    return {reinterpret_cast<std::byte*>(values.data()), values.size() * sizeof(float32)};
  }

  static nonstd::span<const std::byte> AsBytes(nonstd::span<const float32> values)
  {
    return {reinterpret_cast<const std::byte*>(values.data()), values.size() * sizeof(float32)};
  }

  void MaybeNegateValues(nonstd::span<float32> values) const
  {
    if(!m_NegateOutput)
    {
      return;
    }
    for(float32& value : values)
    {
      value = -value;
    }
  }

  Result<std::unique_ptr<ITemporaryRecordStore>> CreateScratchStore(const TemporaryRecordStoreConfig& config, std::string_view context)
  {
    try
    {
      auto storeResult = DataStoreUtilities::CreateTemporaryRecordStore(config);
      if(storeResult.invalid())
      {
        return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8673, fmt::format("Fast chamfer distance failed to create {}: {}", context, DescribeResultError(storeResult)));
      }
      std::unique_ptr<ITemporaryRecordStore> store = std::move(storeResult.value());
      if(store == nullptr)
      {
        return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8673, fmt::format("Fast chamfer distance failed to create {}: provider returned a null store", context));
      }
      return {std::move(store)};
    } catch(const std::bad_alloc& exception)
    {
      return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8673, fmt::format("Fast chamfer distance failed to create {}: {}", context, exception.what()));
    } catch(const std::exception& exception)
    {
      return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-8673, fmt::format("Fast chamfer distance failed to create {}: {}", context, exception.what()));
    }
  }

  Result<> ReadScratchRecords(const ITemporaryRecordStore& store, uint64 recordOffset, uint64 recordCount, nonstd::span<float32> values, std::string_view context)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      auto result = store.read(recordOffset, recordCount, AsWritableBytes(values), m_ShouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult(-8674, fmt::format("Fast chamfer distance scratch read failed for {}: {}", context, DescribeResultError(result)));
      }
      if(result.value() != recordCount)
      {
        return MakeErrorResult(-8674, fmt::format("Fast chamfer distance scratch read failed for {}: returned {} of {} records", context, result.value(), recordCount));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8674, fmt::format("Fast chamfer distance scratch read failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  Result<> WriteScratchRecords(ITemporaryRecordStore& store, uint64 recordOffset, uint64 recordCount, nonstd::span<const float32> values, std::string_view context)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      Result<> result = store.write(recordOffset, recordCount, AsBytes(values), m_ShouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult(-8674, fmt::format("Fast chamfer distance scratch write failed for {}: {}", context, DescribeResultError(result)));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8674, fmt::format("Fast chamfer distance scratch write failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  Result<> ReadPrimaryValues(usize valueOffset, nonstd::span<float32> values, std::string_view context)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      Result<> result = m_Store.copyIntoBuffer(valueOffset, values);
      if(result.invalid())
      {
        return MakeErrorResult(-8675, fmt::format("Fast chamfer distance bulk input transfer failed for {}: {}", context, DescribeResultError(result)));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8675, fmt::format("Fast chamfer distance bulk input transfer failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  Result<> WritePrimaryValues(usize valueOffset, nonstd::span<const float32> values, std::string_view context)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      Result<> result = m_Store.copyFromBuffer(valueOffset, values);
      if(result.invalid())
      {
        return MakeErrorResult(-8675, fmt::format("Fast chamfer distance bulk output transfer failed for {}: {}", context, DescribeResultError(result)));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8675, fmt::format("Fast chamfer distance bulk output transfer failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  void Propagate2DForward(float32* block, usize nx, usize sourceRows, usize loadedRows)
  {
    const float32 w0 = detail::k_ChamferWeights[0];
    const float32 w1 = detail::k_ChamferWeights[1];
    for(usize row = 0; row < sourceRows; ++row)
    {
      float32* current = block + row * nx;
      float32* next = row + 1 < loadedRows ? current + nx : nullptr;
      for(usize x = 0; x < nx; ++x)
      {
        const float32 value = current[x];
        if(value >= m_MaxDist || value <= -m_MaxDist)
        {
          continue;
        }
        const bool doPos = value > -w0;
        const bool doNeg = value < w0;
        if(x + 1 < nx)
        {
          PushChamfer(current[x + 1], value, w0, doPos, doNeg);
        }
        if(next != nullptr)
        {
          if(x > 0)
          {
            PushChamfer(next[x - 1], value, w1, doPos, doNeg);
          }
          PushChamfer(next[x], value, w0, doPos, doNeg);
          if(x + 1 < nx)
          {
            PushChamfer(next[x + 1], value, w1, doPos, doNeg);
          }
        }
      }
    }
  }

  void Propagate2DBackward(float32* block, usize nx, usize firstSourceRow, usize loadedRows)
  {
    const float32 w0 = detail::k_ChamferWeights[0];
    const float32 w1 = detail::k_ChamferWeights[1];
    for(usize row = loadedRows; row-- > firstSourceRow;)
    {
      float32* current = block + row * nx;
      float32* previous = row > 0 ? current - nx : nullptr;
      for(usize x = nx; x-- > 0;)
      {
        const float32 value = current[x];
        if(value >= m_MaxDist || value <= -m_MaxDist)
        {
          continue;
        }
        const bool doPos = value > -w0;
        const bool doNeg = value < w0;
        if(previous != nullptr)
        {
          if(x > 0)
          {
            PushChamfer(previous[x - 1], value, w1, doPos, doNeg);
          }
          PushChamfer(previous[x], value, w0, doPos, doNeg);
          if(x + 1 < nx)
          {
            PushChamfer(previous[x + 1], value, w1, doPos, doNeg);
          }
        }
        if(x > 0)
        {
          PushChamfer(current[x - 1], value, w0, doPos, doNeg);
        }
      }
    }
  }

  Result<> RunFullWidth2D(ITemporaryRecordStore& scratch, usize nx, usize ny, usize coreRows)
  {
    const usize capacityRows = std::min(ny, coreRows + 1);
    usize capacityValues = 0;
    if(!detail::ChamferCheckedMultiply(capacityRows, nx, capacityValues))
    {
      return MakeErrorResult(-8672, fmt::format("Fast chamfer distance 2D row-block layout overflows for dimensions {} x {}.", nx, ny));
    }
    std::vector<float32> block(capacityValues);

    bool firstBlock = true;
    for(usize rowBegin = 0; rowBegin < ny; rowBegin += coreRows)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize sourceRows = std::min(coreRows, ny - rowBegin);
      const usize loadedRows = sourceRows + static_cast<usize>(rowBegin + sourceRows < ny);
      if(firstBlock)
      {
        if(Result<> result = ReadPrimaryValues(rowBegin * nx, nonstd::span<float32>(block.data(), loadedRows * nx), "2D forward input block"); result.invalid())
        {
          return result;
        }
      }
      else
      {
        const usize rowsToRead = loadedRows - 1;
        if(rowsToRead > 0)
        {
          if(Result<> result = ReadPrimaryValues((rowBegin + 1) * nx, nonstd::span<float32>(block.data() + nx, rowsToRead * nx), "2D forward input block"); result.invalid())
          {
            return result;
          }
        }
      }
      Propagate2DForward(block.data(), nx, sourceRows, loadedRows);
      if(Result<> result = WriteScratchRecords(scratch, rowBegin, sourceRows, nonstd::span<const float32>(block.data(), sourceRows * nx), "2D forward output block"); result.invalid())
      {
        return result;
      }
      if(loadedRows > sourceRows)
      {
        std::copy_n(block.data() + sourceRows * nx, nx, block.data());
      }
      firstBlock = false;
    }

    firstBlock = true;
    for(usize rowEnd = ny; rowEnd > 0;)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize sourceRows = std::min(coreRows, rowEnd);
      const usize sourceBegin = rowEnd - sourceRows;
      const bool hasHalo = sourceBegin > 0;
      const usize loadBegin = hasHalo ? sourceBegin - 1 : sourceBegin;
      const usize loadedRows = sourceRows + static_cast<usize>(hasHalo);
      if(firstBlock)
      {
        if(Result<> result = ReadScratchRecords(scratch, loadBegin, loadedRows, nonstd::span<float32>(block.data(), loadedRows * nx), "2D backward input block"); result.invalid())
        {
          return result;
        }
      }
      else
      {
        std::copy_n(block.data(), nx, block.data() + (loadedRows - 1) * nx);
        const usize rowsToRead = loadedRows - 1;
        if(rowsToRead > 0)
        {
          if(Result<> result = ReadScratchRecords(scratch, loadBegin, rowsToRead, nonstd::span<float32>(block.data(), rowsToRead * nx), "2D backward input block"); result.invalid())
          {
            return result;
          }
        }
      }
      Propagate2DBackward(block.data(), nx, static_cast<usize>(hasHalo), loadedRows);
      const usize sourceOffset = static_cast<usize>(hasHalo) * nx;
      MaybeNegateValues(nonstd::span<float32>(block.data() + sourceOffset, sourceRows * nx));
      if(Result<> result = WritePrimaryValues(sourceBegin * nx, nonstd::span<const float32>(block.data() + sourceOffset, sourceRows * nx), "2D backward output block"); result.invalid())
      {
        return result;
      }
      rowEnd = sourceBegin;
      firstBlock = false;
    }
    return {};
  }

  Result<> ProcessTiled2DPass(ITemporaryRecordStore& scratch, usize nx, usize ny, usize coreCols, usize tileCount, bool forward)
  {
    std::array<std::vector<float32>, detail::k_Chamfer2DMaxTileRecords> tileValues;
    for(auto& values : tileValues)
    {
      values.resize(coreCols);
    }
    std::array<uint64, detail::k_Chamfer2DMaxTileRecords> recordIndices{};
    std::array<bool, detail::k_Chamfer2DMaxTileRecords> dirty{};
    const float32 w0 = detail::k_ChamferWeights[0];
    const float32 w1 = detail::k_ChamferWeights[1];

    const auto processTile = [&](usize row, usize tile) -> Result<> {
      if(m_ShouldCancel)
      {
        return {};
      }
      usize usedRecords = 0;
      dirty.fill(false);
      auto loadRecord = [&](usize targetRow, usize targetTile) -> Result<> {
        const uint64 recordIndex = static_cast<uint64>(targetRow) * static_cast<uint64>(tileCount) + static_cast<uint64>(targetTile);
        for(usize index = 0; index < usedRecords; ++index)
        {
          if(recordIndices[index] == recordIndex)
          {
            return {};
          }
        }
        if(usedRecords >= tileValues.size())
        {
          return MakeErrorResult(-8672, "Fast chamfer distance 2D tile working set exceeded its fixed five-record bound.");
        }
        nonstd::span<float32> values(tileValues[usedRecords].data(), coreCols);
        if(Result<> result = ReadScratchRecords(scratch, recordIndex, 1, values, "2D tile working set"); result.invalid())
        {
          return result;
        }
        recordIndices[usedRecords] = recordIndex;
        ++usedRecords;
        return {};
      };
      auto findRecord = [&](usize targetRow, usize targetTile) {
        const uint64 recordIndex = static_cast<uint64>(targetRow) * static_cast<uint64>(tileCount) + static_cast<uint64>(targetTile);
        for(usize index = 0; index < usedRecords; ++index)
        {
          if(recordIndices[index] == recordIndex)
          {
            return index;
          }
        }
        return usedRecords;
      };

      if(Result<> result = loadRecord(row, tile); result.invalid())
      {
        return result;
      }
      if(forward)
      {
        if(tile + 1 < tileCount)
        {
          if(Result<> result = loadRecord(row, tile + 1); result.invalid())
          {
            return result;
          }
        }
        if(row + 1 < ny)
        {
          if(tile > 0)
          {
            if(Result<> result = loadRecord(row + 1, tile - 1); result.invalid())
            {
              return result;
            }
          }
          if(Result<> result = loadRecord(row + 1, tile); result.invalid())
          {
            return result;
          }
          if(tile + 1 < tileCount)
          {
            if(Result<> result = loadRecord(row + 1, tile + 1); result.invalid())
            {
              return result;
            }
          }
        }
      }
      else
      {
        if(tile > 0)
        {
          if(Result<> result = loadRecord(row, tile - 1); result.invalid())
          {
            return result;
          }
        }
        if(row > 0)
        {
          if(tile > 0)
          {
            if(Result<> result = loadRecord(row - 1, tile - 1); result.invalid())
            {
              return result;
            }
          }
          if(Result<> result = loadRecord(row - 1, tile); result.invalid())
          {
            return result;
          }
          if(tile + 1 < tileCount)
          {
            if(Result<> result = loadRecord(row - 1, tile + 1); result.invalid())
            {
              return result;
            }
          }
        }
      }

      auto pushTarget = [&](usize targetRow, usize targetX, float32 source, float32 weight, bool doPos, bool doNeg) {
        const usize targetTile = targetX / coreCols;
        const usize targetOffset = targetX - targetTile * coreCols;
        const usize recordSlot = findRecord(targetRow, targetTile);
        if(recordSlot == usedRecords)
        {
          return false;
        }
        PushChamfer(tileValues[recordSlot][targetOffset], source, weight, doPos, doNeg);
        dirty[recordSlot] = true;
        return true;
      };

      const usize xBegin = tile * coreCols;
      const usize validColumns = std::min(coreCols, nx - xBegin);
      float32* sourceValues = tileValues[0].data();
      if(forward)
      {
        for(usize localX = 0; localX < validColumns; ++localX)
        {
          const usize x = xBegin + localX;
          const float32 value = sourceValues[localX];
          if(value >= m_MaxDist || value <= -m_MaxDist)
          {
            continue;
          }
          const bool doPos = value > -w0;
          const bool doNeg = value < w0;
          if(x + 1 < nx && !pushTarget(row, x + 1, value, w0, doPos, doNeg))
          {
            return MakeErrorResult(-8672, "Fast chamfer distance 2D forward tile omitted a required target record.");
          }
          if(row + 1 < ny)
          {
            if(x > 0 && !pushTarget(row + 1, x - 1, value, w1, doPos, doNeg))
            {
              return MakeErrorResult(-8672, "Fast chamfer distance 2D forward tile omitted a required target record.");
            }
            if(!pushTarget(row + 1, x, value, w0, doPos, doNeg))
            {
              return MakeErrorResult(-8672, "Fast chamfer distance 2D forward tile omitted a required target record.");
            }
            if(x + 1 < nx && !pushTarget(row + 1, x + 1, value, w1, doPos, doNeg))
            {
              return MakeErrorResult(-8672, "Fast chamfer distance 2D forward tile omitted a required target record.");
            }
          }
        }
      }
      else
      {
        for(usize localX = validColumns; localX-- > 0;)
        {
          const usize x = xBegin + localX;
          const float32 value = sourceValues[localX];
          if(value >= m_MaxDist || value <= -m_MaxDist)
          {
            continue;
          }
          const bool doPos = value > -w0;
          const bool doNeg = value < w0;
          if(row > 0)
          {
            if(x > 0 && !pushTarget(row - 1, x - 1, value, w1, doPos, doNeg))
            {
              return MakeErrorResult(-8672, "Fast chamfer distance 2D backward tile omitted a required target record.");
            }
            if(!pushTarget(row - 1, x, value, w0, doPos, doNeg))
            {
              return MakeErrorResult(-8672, "Fast chamfer distance 2D backward tile omitted a required target record.");
            }
            if(x + 1 < nx && !pushTarget(row - 1, x + 1, value, w1, doPos, doNeg))
            {
              return MakeErrorResult(-8672, "Fast chamfer distance 2D backward tile omitted a required target record.");
            }
          }
          if(x > 0 && !pushTarget(row, x - 1, value, w0, doPos, doNeg))
          {
            return MakeErrorResult(-8672, "Fast chamfer distance 2D backward tile omitted a required target record.");
          }
        }
      }

      for(usize index = 0; index < usedRecords; ++index)
      {
        if(!dirty[index])
        {
          continue;
        }
        const nonstd::span<const float32> values(tileValues[index].data(), coreCols);
        if(Result<> result = WriteScratchRecords(scratch, recordIndices[index], 1, values, forward ? "2D forward tile" : "2D backward tile"); result.invalid())
        {
          return result;
        }
      }
      return {};
    };

    if(forward)
    {
      for(usize row = 0; row < ny; ++row)
      {
        for(usize tile = 0; tile < tileCount; ++tile)
        {
          if(Result<> result = processTile(row, tile); result.invalid())
          {
            return result;
          }
        }
      }
    }
    else
    {
      for(usize row = ny; row-- > 0;)
      {
        for(usize tile = tileCount; tile-- > 0;)
        {
          if(Result<> result = processTile(row, tile); result.invalid())
          {
            return result;
          }
        }
      }
    }
    return {};
  }

  Result<> RunTiled2D(ITemporaryRecordStore& scratch, usize nx, usize ny, usize coreCols, usize tileCount)
  {
    {
      std::vector<float32> record(coreCols, 0.0f);
      for(usize row = 0; row < ny; ++row)
      {
        for(usize tile = 0; tile < tileCount; ++tile)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const usize xBegin = tile * coreCols;
          const usize validColumns = std::min(coreCols, nx - xBegin);
          std::fill(record.begin(), record.end(), 0.0f);
          if(Result<> result = ReadPrimaryValues(row * nx + xBegin, nonstd::span<float32>(record.data(), validColumns), "2D tiled scratch initialization"); result.invalid())
          {
            return result;
          }
          const uint64 recordIndex = static_cast<uint64>(row) * static_cast<uint64>(tileCount) + static_cast<uint64>(tile);
          if(Result<> result = WriteScratchRecords(scratch, recordIndex, 1, nonstd::span<const float32>(record.data(), coreCols), "2D tiled scratch initialization"); result.invalid())
          {
            return result;
          }
        }
      }
    }

    if(Result<> result = ProcessTiled2DPass(scratch, nx, ny, coreCols, tileCount, true); result.invalid())
    {
      return result;
    }
    if(Result<> result = ProcessTiled2DPass(scratch, nx, ny, coreCols, tileCount, false); result.invalid())
    {
      return result;
    }

    {
      std::vector<float32> record(coreCols);
      for(usize row = 0; row < ny; ++row)
      {
        for(usize tile = 0; tile < tileCount; ++tile)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const uint64 recordIndex = static_cast<uint64>(row) * static_cast<uint64>(tileCount) + static_cast<uint64>(tile);
          if(Result<> result = ReadScratchRecords(scratch, recordIndex, 1, nonstd::span<float32>(record.data(), coreCols), "2D tiled final output"); result.invalid())
          {
            return result;
          }
          const usize xBegin = tile * coreCols;
          const usize validColumns = std::min(coreCols, nx - xBegin);
          MaybeNegateValues(nonstd::span<float32>(record.data(), validColumns));
          if(Result<> result = WritePrimaryValues(row * nx + xBegin, nonstd::span<const float32>(record.data(), validColumns), "2D tiled final output"); result.invalid())
          {
            return result;
          }
        }
      }
    }
    return {};
  }

  Result<> RunOutOfCore2D(usize nx, usize ny)
  {
    const detail::Chamfer2DBufferPlan plan = detail::BuildChamfer2DBufferPlan(nx, ny, m_ResidentLimit2D);
    if(plan.overflow)
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance 2D buffer plan cannot represent dimensions {} x {} due to arithmetic overflow.", nx, ny));
    }
    if(!plan.valid)
    {
      return MakeErrorResult(-8672, fmt::format("Fast chamfer distance 2D buffer plan exceeds the {}-byte resident limit for dimensions {} x {}.", m_ResidentLimit2D, nx, ny));
    }
    usize cellCount = 0;
    if(!detail::ChamferCheckedMultiply(nx, ny, cellCount))
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance image dimensions overflow the addressable value count: {} x {} x 1.", nx, ny));
    }
    if(m_Store.getSize() != cellCount)
    {
      return MakeErrorResult(-8671, fmt::format("Fast chamfer distance store size ({}) does not match the expected image size ({}) for dimensions {} x {} x 1.", m_Store.getSize(), cellCount, nx, ny));
    }

    const usize tileCount = 1 + (nx - 1) / plan.coreCols;
    usize recordSize = 0;
    usize recordCount = 0;
    if(!detail::ChamferCheckedMultiply(plan.coreCols, sizeof(float32), recordSize) || !detail::ChamferCheckedMultiply(ny, tileCount, recordCount))
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance 2D scratch layout cannot represent dimensions {} x {} due to arithmetic overflow.", nx, ny));
    }
    TemporaryRecordStoreConfig config;
    config.recordSize = static_cast<uint64>(recordSize);
    config.maxRecordsPerBatch = static_cast<uint64>(plan.coreCols == nx ? std::min(ny, plan.coreRows + 1) : 1);
    config.initialRecordCount = static_cast<uint64>(recordCount);
    auto scratchResult = CreateScratchStore(config, "bounded 2D fixed-record scratch store");
    if(scratchResult.invalid())
    {
      return ConvertResult(std::move(scratchResult));
    }
    std::unique_ptr<ITemporaryRecordStore> scratch = std::move(scratchResult.value());
    try
    {
      if(plan.coreCols == nx)
      {
        return RunFullWidth2D(*scratch, nx, ny, plan.coreRows);
      }
      return RunTiled2D(*scratch, nx, ny, plan.coreCols, tileCount);
    } catch(const std::bad_alloc& exception)
    {
      return MakeErrorResult(-8672, fmt::format("Fast chamfer distance could not allocate its bounded 2D working set: {}", exception.what()));
    }
  }

  /**
   * @brief Processes one row in forward raster order.
   * @param cur Source row in the current plane.
   * @param curNext Next row in the current plane, or null at the Y boundary.
   * @param nextPrev Previous row in the next plane, or null at a boundary.
   * @param nextSame Same row in the next plane, or null when the plane is not loaded.
   * @param nextNext Next row in the next plane, or null at a boundary.
   * @param nx Number of columns in each row.
   * @param maxDist Positive magnitude at which source values stop propagation.
   */
  static void ProcessRowForward(float32* cur, float32* curNext, float32* nextPrev, float32* nextSame, float32* nextNext, usize nx, float32 maxDist)
  {
    const float32 w0 = detail::k_ChamferWeights[0];
    const float32 w1 = detail::k_ChamferWeights[1];
    const float32 w2 = detail::k_ChamferWeights[2];
    const auto processGeneric = [cur, curNext, nextPrev, nextSame, nextNext, nx, maxDist, w0, w1, w2](usize x) {
      const float32 c = cur[x];
      if(c >= maxDist || c <= -maxDist)
      {
        return;
      }
      const bool doPos = c > -w0;
      const bool doNeg = c < w0;
      if(x + 1 < nx)
      {
        PushChamfer(cur[x + 1], c, w0, doPos, doNeg);
      }
      if(curNext != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(curNext[x - 1], c, w1, doPos, doNeg);
        }
        PushChamfer(curNext[x], c, w0, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(curNext[x + 1], c, w1, doPos, doNeg);
        }
      }
      if(nextPrev != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(nextPrev[x - 1], c, w2, doPos, doNeg);
        }
        PushChamfer(nextPrev[x], c, w1, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(nextPrev[x + 1], c, w2, doPos, doNeg);
        }
      }
      if(nextSame != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(nextSame[x - 1], c, w1, doPos, doNeg);
        }
        PushChamfer(nextSame[x], c, w0, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(nextSame[x + 1], c, w1, doPos, doNeg);
        }
      }
      if(nextNext != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(nextNext[x - 1], c, w2, doPos, doNeg);
        }
        PushChamfer(nextNext[x], c, w1, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(nextNext[x + 1], c, w2, doPos, doNeg);
        }
      }
    };

    const bool hasInteriorRows = curNext != nullptr && nextPrev != nullptr && nextSame != nullptr && nextNext != nullptr;
    if(!hasInteriorRows || nx < 3)
    {
      for(usize x = 0; x < nx; ++x)
      {
        processGeneric(x);
      }
      return;
    }

    processGeneric(0);
    for(usize x = 1; x + 1 < nx; ++x)
    {
      const float32 c = cur[x];
      if(c >= maxDist || c <= -maxDist)
      {
        continue;
      }
      const bool doPos = c > -w0;
      const bool doNeg = c < w0;
      PushChamfer(cur[x + 1], c, w0, doPos, doNeg);
      PushChamfer(curNext[x - 1], c, w1, doPos, doNeg);
      PushChamfer(curNext[x], c, w0, doPos, doNeg);
      PushChamfer(curNext[x + 1], c, w1, doPos, doNeg);
      PushChamfer(nextPrev[x - 1], c, w2, doPos, doNeg);
      PushChamfer(nextPrev[x], c, w1, doPos, doNeg);
      PushChamfer(nextPrev[x + 1], c, w2, doPos, doNeg);
      PushChamfer(nextSame[x - 1], c, w1, doPos, doNeg);
      PushChamfer(nextSame[x], c, w0, doPos, doNeg);
      PushChamfer(nextSame[x + 1], c, w1, doPos, doNeg);
      PushChamfer(nextNext[x - 1], c, w2, doPos, doNeg);
      PushChamfer(nextNext[x], c, w1, doPos, doNeg);
      PushChamfer(nextNext[x + 1], c, w2, doPos, doNeg);
    }
    processGeneric(nx - 1);
  }

  /**
   * @brief Processes one row in backward raster order.
   * @param cur Source row in the current plane.
   * @param curPrev Previous row in the current plane, or null at the Y boundary.
   * @param prevPrev Previous row in the previous plane, or null at a boundary.
   * @param prevSame Same row in the previous plane, or null when the plane is not loaded.
   * @param prevNext Next row in the previous plane, or null at a boundary.
   * @param nx Number of columns in each row.
   * @param maxDist Positive magnitude at which source values stop propagation.
   */
  static void ProcessRowBackward(float32* cur, float32* curPrev, float32* prevPrev, float32* prevSame, float32* prevNext, usize nx, float32 maxDist)
  {
    const float32 w0 = detail::k_ChamferWeights[0];
    const float32 w1 = detail::k_ChamferWeights[1];
    const float32 w2 = detail::k_ChamferWeights[2];
    const auto processGeneric = [cur, curPrev, prevPrev, prevSame, prevNext, nx, maxDist, w0, w1, w2](usize x) {
      const float32 c = cur[x];
      if(c >= maxDist || c <= -maxDist)
      {
        return;
      }
      const bool doPos = c > -w0;
      const bool doNeg = c < w0;
      if(x > 0)
      {
        PushChamfer(cur[x - 1], c, w0, doPos, doNeg);
      }
      if(curPrev != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(curPrev[x - 1], c, w1, doPos, doNeg);
        }
        PushChamfer(curPrev[x], c, w0, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(curPrev[x + 1], c, w1, doPos, doNeg);
        }
      }
      if(prevPrev != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(prevPrev[x - 1], c, w2, doPos, doNeg);
        }
        PushChamfer(prevPrev[x], c, w1, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(prevPrev[x + 1], c, w2, doPos, doNeg);
        }
      }
      if(prevSame != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(prevSame[x - 1], c, w1, doPos, doNeg);
        }
        PushChamfer(prevSame[x], c, w0, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(prevSame[x + 1], c, w1, doPos, doNeg);
        }
      }
      if(prevNext != nullptr)
      {
        if(x > 0)
        {
          PushChamfer(prevNext[x - 1], c, w2, doPos, doNeg);
        }
        PushChamfer(prevNext[x], c, w1, doPos, doNeg);
        if(x + 1 < nx)
        {
          PushChamfer(prevNext[x + 1], c, w2, doPos, doNeg);
        }
      }
    };

    const bool hasInteriorRows = curPrev != nullptr && prevPrev != nullptr && prevSame != nullptr && prevNext != nullptr;
    if(!hasInteriorRows || nx < 3)
    {
      for(usize x = nx; x-- > 0;)
      {
        processGeneric(x);
      }
      return;
    }

    processGeneric(nx - 1);
    for(usize x = nx - 1; x-- > 1;)
    {
      const float32 c = cur[x];
      if(c >= maxDist || c <= -maxDist)
      {
        continue;
      }
      const bool doPos = c > -w0;
      const bool doNeg = c < w0;
      PushChamfer(cur[x - 1], c, w0, doPos, doNeg);
      PushChamfer(curPrev[x - 1], c, w1, doPos, doNeg);
      PushChamfer(curPrev[x], c, w0, doPos, doNeg);
      PushChamfer(curPrev[x + 1], c, w1, doPos, doNeg);
      PushChamfer(prevPrev[x - 1], c, w2, doPos, doNeg);
      PushChamfer(prevPrev[x], c, w1, doPos, doNeg);
      PushChamfer(prevPrev[x + 1], c, w2, doPos, doNeg);
      PushChamfer(prevSame[x - 1], c, w1, doPos, doNeg);
      PushChamfer(prevSame[x], c, w0, doPos, doNeg);
      PushChamfer(prevSame[x + 1], c, w1, doPos, doNeg);
      PushChamfer(prevNext[x - 1], c, w2, doPos, doNeg);
      PushChamfer(prevNext[x], c, w1, doPos, doNeg);
      PushChamfer(prevNext[x + 1], c, w2, doPos, doNeg);
    }
    processGeneric(0);
  }

  /**
   * @brief Applies the forward row-group wavefront to source planes and an optional target-only halo.
   * @param block Points to plane zero of the contiguous loaded window.
   * @param nx Number of columns in each plane.
   * @param ny Number of rows in each plane.
   * @param sourcePlanes Number of leading planes that act as sources.
   * @param loadedPlanes Total number of planes, including an optional trailing halo.
   * @return True after all levels complete. False when cancellation occurs between levels.
   *
   * Levels with several groups run under the parallel data algorithm. Levels with one group run on the calling thread,
   * so a thin volume with one plane or one row group performs a plain serial scan.
   */
  bool PropagateForwardWavefront(float32* block, usize nx, usize ny, usize sourcePlanes, usize loadedPlanes)
  {
    if(sourcePlanes == 0 || nx == 0 || ny == 0)
    {
      return true;
    }
    const usize slice = nx * ny;
    const usize groupRows = m_RowGroupRows;
    const usize groupCount = 1 + (ny - 1) / groupRows;
    const usize lastLevel = 3 * (sourcePlanes - 1) + (groupCount - 1);
    const float32 maxDist = m_MaxDist;
    for(usize level = 0; level <= lastLevel; ++level)
    {
      if(m_ShouldCancel)
      {
        return false;
      }
      const usize zMin = level > groupCount - 1 ? (level - (groupCount - 1) + 2) / 3 : 0;
      const usize zMax = std::min(sourcePlanes - 1, level / 3);
      // A level holds the groups (Y, z) where Y = level - 3z. With fewer than three row groups, some levels are empty.
      // A nonempty level has one group when the volume has one plane or one row group.
      const usize groupsInLevel = zMax - zMin + 1;
      const auto processGroup = [block, nx, ny, loadedPlanes, slice, groupRows, level, zMin, maxDist](usize index) {
        const usize plane = zMin + index;
        const usize group = level - 3 * plane;
        const usize rowBegin = group * groupRows;
        const usize rowEnd = rowBegin + std::min(groupRows, ny - rowBegin);
        float32* currentPlane = block + plane * slice;
        float32* nextPlane = plane + 1 < loadedPlanes ? currentPlane + slice : nullptr;
        for(usize y = rowBegin; y < rowEnd; ++y)
        {
          float32* cur = currentPlane + y * nx;
          float32* curNext = y + 1 < ny ? cur + nx : nullptr;
          float32* nextPrev = nextPlane != nullptr && y > 0 ? nextPlane + (y - 1) * nx : nullptr;
          float32* nextSame = nextPlane != nullptr ? nextPlane + y * nx : nullptr;
          float32* nextNext = nextPlane != nullptr && y + 1 < ny ? nextPlane + (y + 1) * nx : nullptr;
          FastChamferDistance::ProcessRowForward(cur, curNext, nextPrev, nextSame, nextNext, nx, maxDist);
        }
      };
      if(groupsInLevel == 0)
      {
        continue;
      }
      if(groupsInLevel == 1)
      {
        // Single-group levels run on the calling thread. This avoids a task-scheduler launch when the level has no
        // parallel work.
        processGroup(0);
        continue;
      }
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, groupsInLevel);
      parallelAlgorithm.execute([&processGroup](const Range& range) {
        for(usize index = range.min(); index < range.max(); ++index)
        {
          processGroup(index);
        }
      });
    }
    return true;
  }

  /**
   * @brief Applies the backward row-group wavefront to source planes and an optional target-only halo.
   * @param block Points to plane zero of the contiguous loaded window.
   * @param nx Number of columns in each plane.
   * @param ny Number of rows in each plane.
   * @param firstSourcePlane Index of the first source plane after an optional leading halo.
   * @param loadedPlanes Total number of planes in the window.
   * @param negateSourcePlanes Negates each source group after its backward updates are final.
   * @return True after all levels complete. False when cancellation occurs between levels.
   *
   * Level scheduling matches the forward kernel: multi-group levels run in parallel, single-group levels run inline.
   */
  bool PropagateBackwardWavefront(float32* block, usize nx, usize ny, usize firstSourcePlane, usize loadedPlanes, bool negateSourcePlanes)
  {
    if(firstSourcePlane >= loadedPlanes || nx == 0 || ny == 0)
    {
      return true;
    }
    const usize slice = nx * ny;
    const usize sourcePlanes = loadedPlanes - firstSourcePlane;
    const usize groupRows = m_RowGroupRows;
    const usize groupCount = 1 + (ny - 1) / groupRows;
    const usize lastLevel = 3 * (sourcePlanes - 1) + (groupCount - 1);
    const float32 maxDist = m_MaxDist;
    for(usize level = 0; level <= lastLevel; ++level)
    {
      if(m_ShouldCancel)
      {
        return false;
      }
      const usize zMin = level > groupCount - 1 ? (level - (groupCount - 1) + 2) / 3 : 0;
      const usize zMax = std::min(sourcePlanes - 1, level / 3);
      // Same level structure as the forward kernel, in the mirrored plane and group coordinates.
      const usize groupsInLevel = zMax - zMin + 1;
      const auto processGroup = [block, nx, ny, loadedPlanes, slice, groupRows, groupCount, level, zMin, maxDist, negateSourcePlanes](usize index) {
        const usize reversedPlane = zMin + index;
        const usize reversedGroup = level - 3 * reversedPlane;
        const usize plane = loadedPlanes - 1 - reversedPlane;
        const usize group = groupCount - 1 - reversedGroup;
        const usize rowBegin = group * groupRows;
        const usize rowEnd = rowBegin + std::min(groupRows, ny - rowBegin);
        float32* currentPlane = block + plane * slice;
        float32* previousPlane = plane > 0 ? currentPlane - slice : nullptr;
        for(usize y = rowEnd; y-- > rowBegin;)
        {
          float32* cur = currentPlane + y * nx;
          float32* curPrev = y > 0 ? cur - nx : nullptr;
          float32* prevPrev = previousPlane != nullptr && y > 0 ? previousPlane + (y - 1) * nx : nullptr;
          float32* prevSame = previousPlane != nullptr ? previousPlane + y * nx : nullptr;
          float32* prevNext = previousPlane != nullptr && y + 1 < ny ? previousPlane + (y + 1) * nx : nullptr;
          FastChamferDistance::ProcessRowBackward(cur, curPrev, prevPrev, prevSame, prevNext, nx, maxDist);
        }
        if(negateSourcePlanes)
        {
          for(usize y = rowBegin; y < rowEnd; ++y)
          {
            float32* row = currentPlane + y * nx;
            for(usize x = 0; x < nx; ++x)
            {
              row[x] = -row[x];
            }
          }
        }
      };
      if(groupsInLevel == 0)
      {
        continue;
      }
      if(groupsInLevel == 1)
      {
        processGroup(0);
        continue;
      }
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, groupsInLevel);
      parallelAlgorithm.execute([&processGroup](const Range& range) {
        for(usize index = range.min(); index < range.max(); ++index)
        {
          processGroup(index);
        }
      });
    }
    return true;
  }

  Result<> RunOutOfCore3D(usize nx, usize ny, usize nz)
  {
    if(nx > static_cast<usize>(std::numeric_limits<int64>::max()) || ny > static_cast<usize>(std::numeric_limits<int64>::max()) || nz > static_cast<usize>(std::numeric_limits<int64>::max()))
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance dimensions exceed the supported signed coordinate range: {} x {} x {}.", nx, ny, nz));
    }
    usize slice = 0;
    usize volume = 0;
    usize planeBytes = 0;
    if(!detail::ChamferCheckedMultiply(nx, ny, slice) || !detail::ChamferCheckedMultiply(slice, nz, volume) || !detail::ChamferCheckedMultiply(slice, sizeof(float32), planeBytes))
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance image dimensions overflow the addressable value count: {} x {} x {}.", nx, ny, nz));
    }
    if(m_Store.getSize() != volume)
    {
      return MakeErrorResult(-8671,
                             fmt::format("Fast chamfer distance store size ({}) does not match the expected image size ({}) for dimensions {} x {} x {}.", m_Store.getSize(), volume, nx, ny, nz));
    }

    constexpr usize kMaxCorePlanes = 16;
    const usize availableBytes = detail::k_Chamfer2DResidentLimit - detail::k_Chamfer2DFixedStateBytes;
    const usize planesFit = planeBytes == 0 ? 0 : availableBytes / planeBytes;
    usize corePlanes = planesFit > 1 ? planesFit - 1 : 1;
    corePlanes = std::min({corePlanes, kMaxCorePlanes, nz});
    const usize capacityPlanes = corePlanes + static_cast<usize>(corePlanes < nz);
    usize capacityValues = 0;
    if(!detail::ChamferCheckedMultiply(capacityPlanes, slice, capacityValues))
    {
      return MakeErrorResult(-8670, fmt::format("Fast chamfer distance 3D plane-block layout overflows for dimensions {} x {} x {}.", nx, ny, nz));
    }

    TemporaryRecordStoreConfig config;
    config.recordSize = static_cast<uint64>(planeBytes);
    config.maxRecordsPerBatch = static_cast<uint64>(capacityPlanes);
    config.initialRecordCount = static_cast<uint64>(nz);
    auto scratchResult = CreateScratchStore(config, "bounded 3D fixed-record scratch store");
    if(scratchResult.invalid())
    {
      return ConvertResult(std::move(scratchResult));
    }
    std::unique_ptr<ITemporaryRecordStore> scratch = std::move(scratchResult.value());

    try
    {
      std::vector<float32> block(capacityValues);
      bool firstBlock = true;
      for(usize planeBegin = 0; planeBegin < nz; planeBegin += corePlanes)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize sourcePlanes = std::min(corePlanes, nz - planeBegin);
        const usize loadedPlanes = sourcePlanes + static_cast<usize>(planeBegin + sourcePlanes < nz);
        if(firstBlock)
        {
          if(Result<> result = ReadPrimaryValues(planeBegin * slice, nonstd::span<float32>(block.data(), loadedPlanes * slice), "3D forward input block"); result.invalid())
          {
            return result;
          }
        }
        else
        {
          const usize planesToRead = loadedPlanes - 1;
          if(planesToRead > 0)
          {
            if(Result<> result = ReadPrimaryValues((planeBegin + 1) * slice, nonstd::span<float32>(block.data() + slice, planesToRead * slice), "3D forward input block"); result.invalid())
            {
              return result;
            }
          }
        }
        if(!PropagateForwardWavefront(block.data(), nx, ny, sourcePlanes, loadedPlanes))
        {
          return {};
        }
        if(Result<> result = WriteScratchRecords(*scratch, planeBegin, sourcePlanes, nonstd::span<const float32>(block.data(), sourcePlanes * slice), "3D forward output block"); result.invalid())
        {
          return result;
        }
        if(loadedPlanes > sourcePlanes)
        {
          std::copy_n(block.data() + sourcePlanes * slice, slice, block.data());
        }
        firstBlock = false;
      }

      firstBlock = true;
      for(usize planeEnd = nz; planeEnd > 0;)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize sourcePlanes = std::min(corePlanes, planeEnd);
        const usize sourceBegin = planeEnd - sourcePlanes;
        const bool hasHalo = sourceBegin > 0;
        const usize loadBegin = hasHalo ? sourceBegin - 1 : sourceBegin;
        const usize loadedPlanes = sourcePlanes + static_cast<usize>(hasHalo);
        if(firstBlock)
        {
          if(Result<> result = ReadScratchRecords(*scratch, loadBegin, loadedPlanes, nonstd::span<float32>(block.data(), loadedPlanes * slice), "3D backward input block"); result.invalid())
          {
            return result;
          }
        }
        else
        {
          std::copy_n(block.data(), slice, block.data() + (loadedPlanes - 1) * slice);
          const usize planesToRead = loadedPlanes - 1;
          if(planesToRead > 0)
          {
            if(Result<> result = ReadScratchRecords(*scratch, loadBegin, planesToRead, nonstd::span<float32>(block.data(), planesToRead * slice), "3D backward input block"); result.invalid())
            {
              return result;
            }
          }
        }
        const usize sourceOffset = static_cast<usize>(hasHalo) * slice;
        if(!PropagateBackwardWavefront(block.data(), nx, ny, static_cast<usize>(hasHalo), loadedPlanes, m_NegateOutput))
        {
          return {};
        }
        if(Result<> result = WritePrimaryValues(sourceBegin * slice, nonstd::span<const float32>(block.data() + sourceOffset, sourcePlanes * slice), "3D backward output block"); result.invalid())
        {
          return result;
        }
        planeEnd = sourceBegin;
        firstBlock = false;
      }
    } catch(const std::bad_alloc& exception)
    {
      return MakeErrorResult(-8672, fmt::format("Fast chamfer distance could not allocate its bounded 3D plane working set: {}", exception.what()));
    }
    return {};
  }

  // itk push: if c > -w0 and c + w < neighbor -> neighbor = c + w (min, positive side); if c < w0 and c - w > neighbor
  // -> neighbor = c - w (max, negative side). No clamp on the written value.
  static void PushChamfer(float32& neighbor, float32 c, float32 w, bool doPos, bool doNeg)
  {
    if(doPos)
    {
      const float32 cand = c + w;
      if(cand < neighbor)
      {
        neighbor = cand;
      }
    }
    if(doNeg)
    {
      const float32 cand = c - w;
      if(cand > neighbor)
      {
        neighbor = cand;
      }
    }
  }

  AbstractDataStore<float32>& m_Store;
  SizeVec3 m_Dims;
  float32 m_MaxDist;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_ResidentLimit2D;
  bool m_NegateOutput = false;
  usize m_RowGroupRows = detail::k_ChamferRowGroupRows;
};

/**
 * @brief Propagates a narrow signed band in place with the ITK two-pass chamfer recurrence.
 *
 * Resident arrays use a parallel wavefront over full-width row groups without plane copies.
 * Bounded 3-D processing applies the same wavefront to 16-plane blocks with one halo plane.
 * Both routes preserve serial float32 update order for each target. Sign inversion occurs after each backward group is final.
 * The function checks cancellation between wavefront levels and bounded blocks.
 * @param store Stores the float32 signed band and receives the distance map.
 * @param dims Gives the image dimensions in X, Y, Z order.
 * @param maxDist Freezes source values at or beyond this positive or negative magnitude.
 * @param shouldCancel Requests a valid early return when set.
 * @param messageHandler Receives filter messages.
 * @param negateOutput Selects sign inversion during final backward processing.
 * @return An error for invalid storage sizes, failed transfers, or failed allocations. Cancellation returns a valid result.
 */
inline Result<> ApplyFastChamferDistance(AbstractDataStore<float32>& store, const SizeVec3& dims, float32 maxDist, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                                         bool negateOutput = false)
{
  FastChamferDistance engine(store, dims, maxDist, shouldCancel, messageHandler, detail::k_Chamfer2DResidentLimit, negateOutput);
  return engine();
}
} // namespace nx::core::ImageProcessing
