#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/task_arena.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
constexpr usize k_Maurer2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
constexpr usize k_Maurer2DMaxWorkers = 16;
constexpr usize k_Maurer2DFixedStateBytes = 4096;

template <class Body>
void ExecuteMaurer2DParallel(usize count, const Body& body)
{
  ParallelDataAlgorithm algorithm;
  algorithm.setRange(0, count);
#ifdef SIMPLNX_ENABLE_MULTICORE
  const usize hardwareWorkers = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  tbb::task_arena arena(static_cast<int>(std::min(k_Maurer2DMaxWorkers, hardwareWorkers)));
  arena.execute([&]() { algorithm.execute(body); });
#else
  algorithm.execute(body);
#endif
}

inline std::string SelectDistanceWorkingDataFormat(IDataStore::StoreType inputStoreType, const std::string& inputFormat, IDataStore::StoreType outputStoreType, const std::string& outputFormat)
{
  if(inputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return inputFormat;
  }
  if(outputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return outputFormat;
  }
  return inputFormat;
}

struct Maurer2DBufferPlan
{
  usize rowBatchRows = 0;
  usize columnBatchCols = 0;
  usize lineBlockValues = 0;
  usize residentBytes = 0;
  bool spillX = false;
  bool spillY = false;
  bool valid = false;
  bool overflow = false;
};

inline bool MaurerCheckedAdd(usize left, usize right, usize& result)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  result = left + right;
  return true;
}

inline bool MaurerCheckedMultiply(usize left, usize right, usize& result)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  result = left * right;
  return true;
}

struct MaurerResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseMaurerResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateMaurerResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize planeValues = 0;
  usize volumeValues = 0;
  usize bytesPerValue = 0;
  usize requiredBytes = 0;
  if(!MaurerCheckedMultiply(dims[0], dims[1], planeValues) || !MaurerCheckedMultiply(planeValues, dims[2], volumeValues) || !MaurerCheckedAdd(sizeof(T), sizeof(float32), bytesPerValue) ||
     !MaurerCheckedMultiply(volumeValues, bytesPerValue, requiredBytes))
  {
    return MakeErrorResult<usize>(-8359,
                                  fmt::format("Signed Maurer distance-map dimensions ({}) and input element size ({} bytes) overflow while sizing the resident input copy and float32 transform state.",
                                              StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<MaurerResidentMemoryAllocation> ReserveMaurerResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateMaurerResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<MaurerResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {MaurerResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

struct Maurer3DSlabMemoryPlan
{
  usize maxYRows = 0;
  usize workerCount = 1;
  usize stagingValues = 0;
  usize residentBytes = 0;
};

template <class T>
Result<Maurer3DSlabMemoryPlan> CreateMaurer3DSlabMemoryPlan(const SizeVec3& dims, usize targetBytes)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] <= 1)
  {
    return MakeErrorResult<Maurer3DSlabMemoryPlan>(
        -8360, fmt::format("Signed Maurer 3D slab planning requires nonzero X and Y dimensions and Z greater than one. Actual dimensions: {}.", StringUtilities::formatDimensions3D(dims)));
  }

  usize planeValues = 0;
  usize initBuffersBytes = 0;
  usize initPeakBytes = 0;
  usize maxLineValues = 0;
  usize oneWorkerScratchBytes = 0;
  usize workerScratchBytes = 0;
  const usize workerCount = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  constexpr usize k_StagingBytesPerValue = sizeof(float32);
  constexpr usize k_InitBytesPerValue = 3 * sizeof(T) + sizeof(float32);
  constexpr usize k_WorkerBytesPerLineValue = 3 * sizeof(float32);
  if(!MaurerCheckedMultiply(dims[0], dims[1], planeValues) || !MaurerCheckedMultiply(planeValues, k_InitBytesPerValue, initBuffersBytes))
  {
    return MakeErrorResult<Maurer3DSlabMemoryPlan>(
        -8361, fmt::format("Signed Maurer dimensions ({}) and input element size ({} bytes) overflow while sizing 3D slab plane buffers.", StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  maxLineValues = std::max({dims[0], dims[1], dims[2]});
  if(!MaurerCheckedMultiply(maxLineValues, k_WorkerBytesPerLineValue, oneWorkerScratchBytes) || !MaurerCheckedMultiply(oneWorkerScratchBytes, workerCount, workerScratchBytes) ||
     !MaurerCheckedAdd(initBuffersBytes, workerScratchBytes, initPeakBytes))
  {
    return MakeErrorResult<Maurer3DSlabMemoryPlan>(
        -8362, fmt::format("Signed Maurer dimensions ({}) and worker count ({}) overflow while sizing 3D slab worker scratch.", StringUtilities::formatDimensions3D(dims), workerCount));
  }

  usize valuesPerYRow = 0;
  usize bytesPerYRow = 0;
  usize twoStagingRowBytes = 0;
  if(!MaurerCheckedMultiply(dims[0], dims[2], valuesPerYRow) || !MaurerCheckedMultiply(valuesPerYRow, k_StagingBytesPerValue, bytesPerYRow) ||
     !MaurerCheckedMultiply(bytesPerYRow, 2, twoStagingRowBytes))
  {
    return MakeErrorResult<Maurer3DSlabMemoryPlan>(
        -8363, fmt::format("Signed Maurer dimensions ({}) overflow while sizing one Y row in each of the two 3D Z-pass staging buffers.", StringUtilities::formatDimensions3D(dims)));
  }
  if(targetBytes < initPeakBytes || targetBytes - workerScratchBytes < twoStagingRowBytes)
  {
    return MakeErrorResult<Maurer3DSlabMemoryPlan>(-8364, fmt::format("Signed Maurer 3D slab target ({} bytes) cannot hold the initialization peak ({} bytes) or worker scratch ({} bytes) plus one Y "
                                                                      "row in each of two Z-pass staging buffers ({} bytes per row) for dimensions {}.",
                                                                      targetBytes, initPeakBytes, workerScratchBytes, bytesPerYRow, StringUtilities::formatDimensions3D(dims)));
  }

  Maurer3DSlabMemoryPlan plan;
  plan.workerCount = workerCount;
  const usize maximumYRows = std::min(dims[1], (targetBytes - workerScratchBytes) / twoStagingRowBytes);
  const usize batchCount = 1 + (dims[1] - 1) / maximumYRows;
  plan.maxYRows = 1 + (dims[1] - 1) / batchCount;
  usize stagingBytes = 0;
  usize twoStagingBytes = 0;
  usize zPassPeakBytes = 0;
  if(!MaurerCheckedMultiply(plan.maxYRows, valuesPerYRow, plan.stagingValues) || !MaurerCheckedMultiply(plan.stagingValues, k_StagingBytesPerValue, stagingBytes) ||
     !MaurerCheckedMultiply(stagingBytes, 2, twoStagingBytes) || !MaurerCheckedAdd(workerScratchBytes, twoStagingBytes, zPassPeakBytes))
  {
    return MakeErrorResult<Maurer3DSlabMemoryPlan>(
        -8365, fmt::format("Signed Maurer dimensions ({}) and {} planned Y rows overflow while sizing two 3D Z-pass staging buffers.", StringUtilities::formatDimensions3D(dims), plan.maxYRows));
  }
  plan.residentBytes = std::max(initPeakBytes, zPassPeakBytes);
  return {plan};
}

template <class T>
Result<ShapeType> CreateMaurer3DChunkHint(const SizeVec3& dims, usize targetBytes)
{
  auto planResult = CreateMaurer3DSlabMemoryPlan<T>(dims, targetBytes);
  if(planResult.invalid())
  {
    return ConvertInvalidResult<ShapeType>(std::move(planResult));
  }
  const ShapeType hint{1, planResult.value().maxYRows, dims[0]};
  usize values = 0;
  usize bytes = 0;
  if(!MaurerCheckedMultiply(hint[0], hint[1], values) || !MaurerCheckedMultiply(values, hint[2], values) || !MaurerCheckedMultiply(values, sizeof(float32), bytes) || bytes == 0)
  {
    return MakeErrorResult<ShapeType>(-8366, fmt::format("Signed Maurer chunk hint overflows for dimensions {}.", StringUtilities::formatDimensions3D(dims)));
  }
  return {hint};
}

inline usize AlignMaurer3DYBatchRows(usize rowAllowance, usize chunkYRows)
{
  if(chunkYRows == 0 || chunkYRows > rowAllowance)
  {
    return rowAllowance;
  }
  const usize alignedRows = (rowAllowance / chunkYRows) * chunkYRows;
  return alignedRows == 0 ? rowAllowance : alignedRows;
}

inline std::optional<ShapeType> SelectMaurer3DScratchChunkHint(const ShapeType& outputChunkShape, const ShapeType& scratchTupleShape)
{
  if(outputChunkShape.size() != scratchTupleShape.size())
  {
    return std::nullopt;
  }
  for(usize index = 0; index < outputChunkShape.size(); ++index)
  {
    if(outputChunkShape[index] == 0 || outputChunkShape[index] > scratchTupleShape[index])
    {
      return std::nullopt;
    }
  }
  return outputChunkShape;
}

/**
 * @class MaurerIoWorker
 * @brief Runs one serial store-I/O sequence on a dedicated thread.
 *
 * Bulk store calls must not overlap. The worker serializes these calls off the main thread so resident transforms can proceed.
 */
class MaurerIoWorker
{
public:
  /**
   * @brief Creates an idle I/O worker.
   */
  MaurerIoWorker() = default;

  /**
   * @brief Joins the I/O thread before the worker releases its state.
   */
  ~MaurerIoWorker()
  {
    join();
  }

  MaurerIoWorker(const MaurerIoWorker&) = delete;
  MaurerIoWorker(MaurerIoWorker&&) noexcept = delete;
  MaurerIoWorker& operator=(const MaurerIoWorker&) = delete;
  MaurerIoWorker& operator=(MaurerIoWorker&&) noexcept = delete;

  /**
   * @brief Starts one store-I/O sequence.
   * @tparam Callable Store-I/O callable type.
   * @param callable Returns the result of the serial store-I/O sequence.
   */
  template <class Callable>
  void start(Callable&& callable)
  {
    m_Result = {};
    m_Thread = std::thread([this, ioSequence = std::forward<Callable>(callable)]() mutable {
      try
      {
        m_Result = ioSequence();
      } catch(const std::exception& exception)
      {
        m_Result = MakeErrorResult(-8370, fmt::format("Signed Maurer I/O worker caught a store exception: {}", exception.what()));
      } catch(...)
      {
        m_Result = MakeErrorResult(-8371, "Signed Maurer I/O worker caught an unknown store exception.");
      }
    });
    m_Started = true;
  }

  /**
   * @brief Waits for the started I/O sequence to finish.
   */
  void join()
  {
    if(m_Started && m_Thread.joinable())
    {
      m_Thread.join();
    }
  }

  /**
   * @brief Returns the completed I/O sequence result.
   * @return Valid result when no sequence started, or the completed sequence result.
   * @pre join() completed after any start().
   */
  [[nodiscard]] const Result<>& result() const noexcept
  {
    return m_Result;
  }

private:
  std::thread m_Thread;
  Result<> m_Result;
  bool m_Started = false;
};

/**
 * @brief Converts a synchronous store exception to a Signed Maurer error result.
 * @tparam Callable Store-I/O callable type.
 * @param callable Runs one store call on the calling thread.
 * @param context Identifies the transfer that failed.
 * @return The store result or an error for a thrown exception.
 */
template <class Callable>
Result<> RunMaurerIoCallSafely(Callable&& callable, std::string_view context)
{
  try
  {
    return callable();
  } catch(const std::exception& exception)
  {
    return MakeErrorResult(-8370, fmt::format("Signed Maurer store call failed during {}: {}", context, exception.what()));
  } catch(...)
  {
    return MakeErrorResult(-8371, fmt::format("Signed Maurer store call failed during {} with an unknown exception.", context));
  }
}

/**
 * @brief Writes one extent and converts provider exceptions to an error result.
 * @tparam U Store value type.
 * @param store Receives the extent values.
 * @param extent Inclusive output bounds in store dimension order.
 * @param values Values for the complete extent.
 * @param shouldCancel Skips the store call when cancellation is set.
 * @param context Identifies the transfer that failed.
 * @return An empty result on success or cancellation, or an error for a provider exception.
 */
template <class U>
Result<> WriteExtentSafely(AbstractDataStore<U>& store, const Extent& extent, nonstd::span<const U> values, const std::atomic_bool& shouldCancel, std::string_view context)
{
  if(shouldCancel)
  {
    return {};
  }
  try
  {
    Result<> writeResult = store.writeExtent(extent, values);
    if(writeResult.invalid())
    {
      return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: {}", context, writeResult.errors().front().message));
    }
  } catch(const std::exception& exception)
  {
    return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: {}", context, exception.what()));
  } catch(...)
  {
    return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {} with an unknown exception.", context));
  }
  return {};
}

inline bool Maurer2DDirectXPeak(usize nx, usize rows, usize inputBytes, usize& peak)
{
  usize haloRows = 0;
  usize input = 0;
  usize cells = 0;
  usize workAndInside = 0;
  usize envelope = 0;
  usize compute = 0;
  usize transpose = 0;
  if(!MaurerCheckedAdd(rows, 2, haloRows) || !MaurerCheckedMultiply(haloRows, nx, input) || !MaurerCheckedMultiply(input, inputBytes, input) || !MaurerCheckedMultiply(rows, nx, cells) ||
     !MaurerCheckedMultiply(cells, sizeof(float32) + sizeof(uint8), workAndInside) || !MaurerCheckedMultiply(nx, 2 * sizeof(float32) * k_Maurer2DMaxWorkers, envelope) ||
     !MaurerCheckedAdd(input, workAndInside, compute) || !MaurerCheckedAdd(compute, envelope, compute) || !MaurerCheckedMultiply(cells, 2 * (sizeof(float32) + sizeof(uint8)), transpose))
  {
    return false;
  }
  return MaurerCheckedAdd(std::max(compute, transpose), k_Maurer2DFixedStateBytes, peak);
}

inline bool Maurer2DDirectYPeak(usize ny, usize columns, usize rowBatchRows, usize& peak)
{
  usize cells = 0;
  usize workAndInside = 0;
  usize envelope = 0;
  usize batchCells = 0;
  usize batchTemporary = 0;
  usize compute = 0;
  if(!MaurerCheckedMultiply(columns, ny, cells) || !MaurerCheckedMultiply(cells, sizeof(float32) + sizeof(uint8), workAndInside) ||
     !MaurerCheckedMultiply(ny, 2 * sizeof(float32) * k_Maurer2DMaxWorkers, envelope) || !MaurerCheckedMultiply(columns, rowBatchRows, batchCells) ||
     !MaurerCheckedMultiply(batchCells, sizeof(float32) + sizeof(uint8), batchTemporary) || !MaurerCheckedAdd(workAndInside, envelope, compute) || !MaurerCheckedAdd(compute, batchTemporary, compute))
  {
    return false;
  }
  return MaurerCheckedAdd(compute, k_Maurer2DFixedStateBytes, peak);
}

inline bool Maurer2DSpillPeak(usize blockValues, usize inputBytes, usize& peak)
{
  // The final one-value remainder is merged into the previous block, so each resident buffer has block+1 capacity.
  usize capacity = 0;
  usize haloWidth = 0;
  usize oneInputHalo = 0;
  usize inputHalo = 0;
  usize sourceInsideOutput = 0;
  usize envelopeCaches = 0;
  usize transposedOutput = 0;
  usize total = 0;
  if(!MaurerCheckedAdd(blockValues, 1, capacity) || !MaurerCheckedAdd(capacity, 2, haloWidth) || !MaurerCheckedMultiply(haloWidth, inputBytes, oneInputHalo) ||
     !MaurerCheckedMultiply(oneInputHalo, 3, inputHalo) || !MaurerCheckedMultiply(capacity, sizeof(float32) + sizeof(uint8) + sizeof(float32), sourceInsideOutput) ||
     !MaurerCheckedMultiply(capacity, 4 * sizeof(float32), envelopeCaches) || !MaurerCheckedMultiply(capacity, sizeof(float32), transposedOutput) ||
     !MaurerCheckedAdd(inputHalo, sourceInsideOutput, total) || !MaurerCheckedAdd(total, envelopeCaches, total) || !MaurerCheckedAdd(total, transposedOutput, total) ||
     !MaurerCheckedAdd(total, k_Maurer2DFixedStateBytes, total))
  {
    return false;
  }
  peak = total;
  return true;
}

inline Maurer2DBufferPlan BuildMaurer2DBufferPlan(usize nx, usize ny, usize inputBytes, usize residentLimit = k_Maurer2DResidentLimit)
{
  Maurer2DBufferPlan plan;
  if(nx == 0 || ny == 0 || inputBytes == 0 || residentLimit == 0)
  {
    plan.overflow = true;
    return plan;
  }

  usize oneRowPeak = 0;
  usize oneColumnPeak = 0;
  if(!Maurer2DDirectXPeak(nx, 1, inputBytes, oneRowPeak) || !Maurer2DDirectYPeak(ny, 1, 0, oneColumnPeak))
  {
    plan.overflow = true;
    return plan;
  }
  plan.spillX = oneRowPeak > residentLimit;
  plan.spillY = oneColumnPeak > residentLimit;
  // Spill-X writes the transposed full scratch directly, so keep phase 2 on the matching external-store route too.
  plan.spillY = plan.spillY || plan.spillX;

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

  if(!plan.spillX)
  {
    plan.rowBatchRows = largestFitting(ny, [nx, inputBytes](usize rows, usize& peak) { return Maurer2DDirectXPeak(nx, rows, inputBytes, peak); });
  }
  if(!plan.spillY)
  {
    const usize rowBatchRows = plan.spillX ? 0 : plan.rowBatchRows;
    plan.columnBatchCols = largestFitting(nx, [ny, rowBatchRows](usize columns, usize& peak) { return Maurer2DDirectYPeak(ny, columns, rowBatchRows, peak); });
  }

  usize spillPeak = 0;
  if(plan.spillX || plan.spillY)
  {
    const usize maxLine = std::max(nx, ny);
    const usize minimumBlock = maxLine == 1 ? 1 : 2;
    usize minimumPeak = 0;
    if(!Maurer2DSpillPeak(minimumBlock, inputBytes, minimumPeak) || minimumPeak > residentLimit)
    {
      return plan;
    }
    usize low = minimumBlock;
    usize high = std::min<usize>(maxLine, 65536);
    while(low < high)
    {
      const usize middle = low + (high - low + 1) / 2;
      usize candidatePeak = 0;
      if(Maurer2DSpillPeak(middle, inputBytes, candidatePeak) && candidatePeak <= residentLimit)
      {
        low = middle;
      }
      else
      {
        high = middle - 1;
      }
    }
    plan.lineBlockValues = low;
    if(!Maurer2DSpillPeak(low, inputBytes, spillPeak))
    {
      plan.overflow = true;
      return plan;
    }
  }
  else
  {
    plan.lineBlockValues = std::min<usize>(std::max(nx, ny), 65536);
  }

  usize directXPeak = 0;
  usize directYPeak = 0;
  const usize rowBatchRows = plan.spillX ? 0 : plan.rowBatchRows;
  if((!plan.spillX && !Maurer2DDirectXPeak(nx, plan.rowBatchRows, inputBytes, directXPeak)) || (!plan.spillY && !Maurer2DDirectYPeak(ny, plan.columnBatchCols, rowBatchRows, directYPeak)))
  {
    plan.overflow = true;
    return plan;
  }
  plan.residentBytes = std::max({directXPeak, directYPeak, spillPeak});
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}

struct MaurerLineChunk
{
  usize begin = 0;
  usize count = 0;
};

inline MaurerLineChunk GetMaurerLineChunk(usize index, usize lineLength, usize blockValues)
{
  const usize regularRemainder = lineLength % blockValues;
  const usize regularEnd = regularRemainder == 1 && lineLength > blockValues ? lineLength - (blockValues + 1) : lineLength - regularRemainder;
  if(index >= regularEnd)
  {
    return {regularEnd, lineLength - regularEnd};
  }
  return {(index / blockValues) * blockValues, blockValues};
}

// itk::SignedMaurerDistanceMapImageFilter::Remove -- the lower-envelope domination test. Parabola (x2,d2) hides the
// middle one (x1,d1) relative to the incoming (xf,df) iff the returned value is > 0. float32 to match ITK exactly.
inline bool MaurerRemove(float32 d1, float32 d2, float32 df, float32 x1, float32 x2, float32 xf)
{
  const float32 a = x2 - x1;
  const float32 b = xf - x2;
  const float32 c = xf - x1;
  const float32 value = (c * std::abs(d2) - b * std::abs(d1) - a * std::abs(df) - a * b * c);
  return value > 0.0f;
}

/**
 * @brief Runs one Maurer Voronoi pass along a line.
 * @tparam HasEncodedInsideSign Selects whether each line value contains its final sign.
 * @param line Contains signed partial squared distances. Encoded mode permits +FLT_MAX and -FLT_MAX for unreached values.
 * @param inside Contains original inside states in explicit mode. Encoded mode accepts an empty span.
 * @param nd Number of values in the line.
 * @param insideIsPositive Selects a positive result for inside voxels in explicit mode.
 * @param useSpacing Applies physical spacing when true.
 * @param spacingD Physical spacing along the line axis.
 * @param g Stores the reusable distance-envelope values.
 * @param h Stores the reusable distance-envelope coordinates.
 *
 * Encoded mode preserves each input sign. Explicit mode reapplies the sign from @p inside after the pass.
 * The implementation matches ITK's distinct float conversion order in the build and query loops.
 */
template <bool HasEncodedInsideSign = false>
void Voronoi1D(nonstd::span<float32> line, nonstd::span<const uint8> inside, usize nd, bool insideIsPositive, bool useSpacing, float32 spacingD, std::vector<float32>& g, std::vector<float32>& h)
{
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  // Raw pointers avoid the per-element span contract check in the line kernel.
  float32* lineData = line.data();
  const uint8* insideData = inside.data();
  int32 l = -1;
  for(usize i = 0; i < nd; ++i)
  {
    const float32 di = lineData[i];
    const float32 iw = useSpacing ? (static_cast<float32>(i) * spacingD) : static_cast<float32>(i);
    const bool isReached = HasEncodedInsideSign ? std::abs(di) != k_Max : di != k_Max;
    if(isReached)
    {
      if(l < 1)
      {
        ++l;
        g[static_cast<usize>(l)] = di;
        h[static_cast<usize>(l)] = iw;
      }
      else
      {
        while((l >= 1) && MaurerRemove(g[static_cast<usize>(l - 1)], g[static_cast<usize>(l)], di, h[static_cast<usize>(l - 1)], h[static_cast<usize>(l)], iw))
        {
          --l;
        }
        ++l;
        g[static_cast<usize>(l)] = di;
        h[static_cast<usize>(l)] = iw;
      }
    }
  }
  if(l == -1)
  {
    return; // No feature on this line: leave every value unchanged, including its encoded sign.
  }
  const int32 ns = l;
  l = 0;
  for(usize i = 0; i < nd; ++i)
  {
    bool encodedNegativeSign = false;
    if constexpr(HasEncodedInsideSign)
    {
      encodedNegativeSign = std::signbit(lineData[i]);
    }
    // ITK's query loop uses float(i*spacing) (double-ish intermediate), unlike the build loop's float(i)*float(spacing).
    const float32 iw = useSpacing ? static_cast<float32>(static_cast<float64>(i) * static_cast<float64>(spacingD)) : static_cast<float32>(i);
    float32 d1 = std::abs(g[static_cast<usize>(l)]) + (h[static_cast<usize>(l)] - iw) * (h[static_cast<usize>(l)] - iw);
    while(l < ns)
    {
      const float32 d2 = std::abs(g[static_cast<usize>(l + 1)]) + (h[static_cast<usize>(l + 1)] - iw) * (h[static_cast<usize>(l + 1)] - iw);
      if(d1 <= d2)
      {
        break;
      }
      ++l;
      d1 = d2;
    }
    if constexpr(HasEncodedInsideSign)
    {
      lineData[i] = encodedNegativeSign ? -d1 : d1;
    }
    else
    {
      // Sign table (itk lines 405-426): inside==insideIsPositive -> +d1, else -d1.
      lineData[i] = ((insideData[i] != 0) == insideIsPositive) ? d1 : -d1;
    }
  }
}

/**
 * @brief Runs one 1D pass when each input work value already encodes its final sign.
 * @param line Signed distance values for one line.
 * @param nd Number of values in the line.
 * @param useSpacing Applies physical spacing when true.
 * @param spacingD Physical spacing along the line axis.
 * @param g Stores the distance-envelope values.
 * @param h Stores the distance-envelope coordinates.
 */
inline void Voronoi1DEncodedSign(nonstd::span<float32> line, usize nd, bool useSpacing, float32 spacingD, std::vector<float32>& g, std::vector<float32>& h)
{
  Voronoi1D<true>(line, {}, nd, false, useSpacing, spacingD, g, h);
}

/**
 * @brief Applies the selected sign convention to one distance magnitude.
 * @param magnitude Nonnegative distance magnitude.
 * @param inside True when the voxel is inside the object.
 * @param insideIsPositive Selects a positive result for inside voxels when true.
 * @return Signed distance value.
 */
inline float32 MaurerSignedValue(float32 magnitude, bool inside, bool insideIsPositive) noexcept
{
  return inside == insideIsPositive ? magnitude : -magnitude;
}

/**
 * @brief Blockwise equivalent of Voronoi1D for a line whose envelope cannot reside in the fixed RAM budget.
 * @tparam OutputSink Callable that receives each completed output block.
 * @param source Provides source distances through bounded transfers.
 * @param inside Provides inside-state values through bounded transfers.
 * @param offset First source value for the line.
 * @param nd Number of values in the line.
 * @param insideIsPositive Selects a positive result for inside voxels when true.
 * @param useSpacing Applies physical spacing when true.
 * @param spacingD Physical spacing along the line axis.
 * @param lineBlockValues Maximum number of values in one transfer block.
 * @param gStore Stores the distance-envelope values.
 * @param hStore Stores the distance-envelope coordinates.
 * @param outputSink Receives completed blocks in ascending line order.
 * @param shouldCancel Stops processing between transfer blocks when true.
 * @return An error from a store transfer or output sink.
 *
 * Envelope construction finishes before the sink receives blocks. Therefore, the sink can safely write to the source store.
 */
template <class OutputSink>
Result<> ExternalVoronoi1DToSink(const AbstractDataStore<float32>& source, const AbstractDataStore<uint8>& inside, usize offset, usize nd, bool insideIsPositive, bool useSpacing, float32 spacingD,
                                 usize lineBlockValues, AbstractDataStore<float32>& gStore, AbstractDataStore<float32>& hStore, OutputSink&& outputSink, const std::atomic_bool& shouldCancel)
{
  if(nd == 0)
  {
    return {};
  }
  if(lineBlockValues == 0 || (nd > 1 && lineBlockValues < 2))
  {
    return MakeErrorResult(-8355, fmt::format("Signed Maurer distance-map external line has invalid block size {} for {} values.", lineBlockValues, nd));
  }
  const usize blockValues = std::min(nd, lineBlockValues);
  const usize capacity = std::min(nd, blockValues + 1);
  std::vector<float32> gBlock(capacity);
  std::vector<float32> hBlock(capacity);
  std::vector<float32> sourceBlock(capacity);
  std::vector<uint8> insideBlock(capacity);
  std::vector<float32> outputBlock(capacity);
  usize envelopeBegin = std::numeric_limits<usize>::max();
  usize envelopeCount = 0;
  bool envelopeDirty = false;

  auto flushEnvelope = [&]() -> Result<> {
    if(!envelopeDirty)
    {
      return {};
    }
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = gStore.copyFromBuffer(envelopeBegin, nonstd::span<const float32>(gBlock.data(), envelopeCount)); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = hStore.copyFromBuffer(envelopeBegin, nonstd::span<const float32>(hBlock.data(), envelopeCount)); result.invalid())
    {
      return result;
    }
    envelopeDirty = false;
    return {};
  };

  auto loadEnvelope = [&](usize index) -> Result<> {
    const MaurerLineChunk chunk = GetMaurerLineChunk(index, nd, blockValues);
    if(chunk.begin == envelopeBegin)
    {
      return {};
    }
    if(Result<> result = flushEnvelope(); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }
    envelopeBegin = chunk.begin;
    envelopeCount = chunk.count;
    if(Result<> result = gStore.copyIntoBuffer(chunk.begin, nonstd::span<float32>(gBlock.data(), chunk.count)); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }
    return hStore.copyIntoBuffer(chunk.begin, nonstd::span<float32>(hBlock.data(), chunk.count));
  };

  auto readEnvelope = [&](usize index, float32& gValue, float32& hValue) -> Result<> {
    if(Result<> result = loadEnvelope(index); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }
    gValue = gBlock[index - envelopeBegin];
    hValue = hBlock[index - envelopeBegin];
    return {};
  };

  auto writeEnvelope = [&](usize index, float32 gValue, float32 hValue) -> Result<> {
    if(Result<> result = loadEnvelope(index); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }
    gBlock[index - envelopeBegin] = gValue;
    hBlock[index - envelopeBegin] = hValue;
    envelopeDirty = true;
    return {};
  };

  int64 envelopeTop = -1;
  for(usize queryBegin = 0; queryBegin < nd;)
  {
    const MaurerLineChunk queryChunk = GetMaurerLineChunk(queryBegin, nd, blockValues);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = source.copyIntoBuffer(offset + queryChunk.begin, nonstd::span<float32>(sourceBlock.data(), queryChunk.count)); result.invalid())
    {
      return result;
    }
    for(usize localIndex = 0; localIndex < queryChunk.count; ++localIndex)
    {
      const usize queryIndex = queryChunk.begin + localIndex;
      const float32 distance = sourceBlock[localIndex];
      if(distance == std::numeric_limits<float32>::max())
      {
        continue;
      }
      const float32 coordinate = useSpacing ? static_cast<float32>(queryIndex) * spacingD : static_cast<float32>(queryIndex);
      if(envelopeTop < 1)
      {
        ++envelopeTop;
      }
      else
      {
        while(envelopeTop >= 1)
        {
          float32 previousDistance = 0.0f;
          float32 previousCoordinate = 0.0f;
          float32 topDistance = 0.0f;
          float32 topCoordinate = 0.0f;
          if(Result<> result = readEnvelope(static_cast<usize>(envelopeTop - 1), previousDistance, previousCoordinate); result.invalid())
          {
            return result;
          }
          if(shouldCancel)
          {
            return {};
          }
          if(Result<> result = readEnvelope(static_cast<usize>(envelopeTop), topDistance, topCoordinate); result.invalid())
          {
            return result;
          }
          if(shouldCancel)
          {
            return {};
          }
          if(!MaurerRemove(previousDistance, topDistance, distance, previousCoordinate, topCoordinate, coordinate))
          {
            break;
          }
          --envelopeTop;
        }
        ++envelopeTop;
      }
      if(Result<> result = writeEnvelope(static_cast<usize>(envelopeTop), distance, coordinate); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
    }
    queryBegin = queryChunk.begin + queryChunk.count;
  }

  if(envelopeTop == -1)
  {
    for(usize queryBegin = 0; queryBegin < nd;)
    {
      const MaurerLineChunk queryChunk = GetMaurerLineChunk(queryBegin, nd, blockValues);
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = source.copyIntoBuffer(offset + queryChunk.begin, nonstd::span<float32>(sourceBlock.data(), queryChunk.count)); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = inside.copyIntoBuffer(offset + queryChunk.begin, nonstd::span<uint8>(insideBlock.data(), queryChunk.count)); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = outputSink(queryChunk.begin, nonstd::span<float32>(sourceBlock.data(), queryChunk.count), nonstd::span<const uint8>(insideBlock.data(), queryChunk.count)); result.invalid())
      {
        return result;
      }
      queryBegin = queryChunk.begin + queryChunk.count;
    }
    return flushEnvelope();
  }

  const int64 envelopeSize = envelopeTop;
  envelopeTop = 0;
  for(usize queryBegin = 0; queryBegin < nd;)
  {
    const MaurerLineChunk queryChunk = GetMaurerLineChunk(queryBegin, nd, blockValues);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = inside.copyIntoBuffer(offset + queryChunk.begin, nonstd::span<uint8>(insideBlock.data(), queryChunk.count)); result.invalid())
    {
      return result;
    }
    for(usize localIndex = 0; localIndex < queryChunk.count; ++localIndex)
    {
      const usize queryIndex = queryChunk.begin + localIndex;
      const float32 coordinate = useSpacing ? static_cast<float32>(static_cast<float64>(queryIndex) * static_cast<float64>(spacingD)) : static_cast<float32>(queryIndex);
      float32 currentDistance = 0.0f;
      float32 currentCoordinate = 0.0f;
      if(Result<> result = readEnvelope(static_cast<usize>(envelopeTop), currentDistance, currentCoordinate); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      float32 distance = std::abs(currentDistance) + (currentCoordinate - coordinate) * (currentCoordinate - coordinate);
      while(envelopeTop < envelopeSize)
      {
        float32 nextDistance = 0.0f;
        float32 nextCoordinate = 0.0f;
        if(Result<> result = readEnvelope(static_cast<usize>(envelopeTop + 1), nextDistance, nextCoordinate); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        const float32 candidate = std::abs(nextDistance) + (nextCoordinate - coordinate) * (nextCoordinate - coordinate);
        if(distance <= candidate)
        {
          break;
        }
        ++envelopeTop;
        distance = candidate;
      }
      outputBlock[localIndex] = ((insideBlock[localIndex] != 0) == insideIsPositive) ? distance : -distance;
    }
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = outputSink(queryChunk.begin, nonstd::span<float32>(outputBlock.data(), queryChunk.count), nonstd::span<const uint8>(insideBlock.data(), queryChunk.count)); result.invalid())
    {
      return result;
    }
    queryBegin = queryChunk.begin + queryChunk.count;
  }
  return flushEnvelope();
}

inline Result<> ExternalVoronoi1D(const AbstractDataStore<float32>& source, const AbstractDataStore<uint8>& inside, usize offset, usize nd, bool insideIsPositive, bool useSpacing, float32 spacingD,
                                  usize lineBlockValues, AbstractDataStore<float32>& gStore, AbstractDataStore<float32>& hStore, AbstractDataStore<float32>& output,
                                  const std::atomic_bool& shouldCancel)
{
  auto storeOutput = [&](usize blockBegin, nonstd::span<float32> values, nonstd::span<const uint8>) {
    return output.copyFromBuffer(offset + blockBegin, nonstd::span<const float32>(values.data(), values.size()));
  };
  return ExternalVoronoi1DToSink(source, inside, offset, nd, insideIsPositive, useSpacing, spacingD, lineBlockValues, gStore, hStore, storeOutput, shouldCancel);
}

/**
 * @brief Initializes signed Maurer work values from a resident input span.
 * @tparam T Integer input value type.
 * @param input Input values in Z-Y-X order.
 * @param backgroundValue Value that identifies background voxels.
 * @param dims Image dimensions in X-Y-Z order.
 * @param insideIsPositive Selects the positive sign for object voxels when true.
 * @param work Receives one signed boundary seed or unreached value for each input value.
 * @param shouldCancel Stops the scan between rows when true.
 * @return True if the scan finds an object voxel with a background neighbor.
 *
 * Each work value retains its final sign so later axis passes do not need a separate inside-state volume.
 */
template <class T>
bool BuildMaurerEncodedInit(nonstd::span<const T> input, T backgroundValue, SizeVec3 dims, bool insideIsPositive, nonstd::span<float32> work, const std::atomic_bool& shouldCancel)
{
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  // Raw pointers avoid the per-element span contract check during initialization.
  const T* inputData = input.data();
  float32* workData = work.data();
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const int64 planeStride = nX * nY;
  const int64 wzLo = (nZ == 1) ? 0 : -1;
  const int64 wzHi = (nZ == 1) ? 0 : 1;
  std::array<int64, 26> interiorOffsets{};
  usize interiorOffsetIndex = 0;
  for(int64 dz = -1; dz <= 1; ++dz)
  {
    for(int64 dy = -1; dy <= 1; ++dy)
    {
      for(int64 dx = -1; dx <= 1; ++dx)
      {
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue;
        }
        interiorOffsets[interiorOffsetIndex++] = dz * planeStride + dy * nX + dx;
      }
    }
  }

  std::atomic_bool hasBoundary{false};
  auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
  auto initializeRows = [&](const Range& rowRange) {
    for(usize row = rowRange.min(); row < rowRange.max(); ++row)
    {
      if(shouldCancel)
      {
        return;
      }
      const int64 z = static_cast<int64>(row / static_cast<usize>(nY));
      const int64 y = static_cast<int64>(row % static_cast<usize>(nY));
      bool rowHasBoundary = false;
      auto initializeBoundedVoxel = [&](int64 x) {
        const usize p = flat(x, y, z);
        const bool obj = inputData[p] != backgroundValue;
        bool boundary = false;
        if(obj)
        {
          for(int64 wz = wzLo; wz <= wzHi && !boundary; ++wz)
          {
            for(int64 wy = -1; wy <= 1 && !boundary; ++wy)
            {
              for(int64 wx = -1; wx <= 1 && !boundary; ++wx)
              {
                if(wx == 0 && wy == 0 && wz == 0)
                {
                  continue;
                }
                const int64 ax = x + wx;
                const int64 ay = y + wy;
                const int64 az = z + wz;
                if(ax < 0 || ay < 0 || az < 0 || ax >= nX || ay >= nY || az >= nZ)
                {
                  continue; // ITK ignores out-of-bounds neighbors because the image border is not background.
                }
                if(inputData[flat(ax, ay, az)] == backgroundValue)
                {
                  boundary = true;
                }
              }
            }
          }
        }
        workData[p] = MaurerSignedValue(boundary ? 0.0f : k_Max, obj, insideIsPositive);
        if(boundary)
        {
          rowHasBoundary = true;
        }
      };

      const bool isInteriorRow = nZ > 1 && z >= 1 && z < nZ - 1 && y >= 1 && y < nY - 1 && nX > 2;
      if(!isInteriorRow)
      {
        for(int64 x = 0; x < nX; ++x)
        {
          initializeBoundedVoxel(x);
        }
      }
      else
      {
        initializeBoundedVoxel(0);
        for(int64 x = 1; x < nX - 1; ++x)
        {
          const usize p = flat(x, y, z);
          const bool obj = inputData[p] != backgroundValue;
          bool boundary = false;
          if(obj)
          {
            for(const int64 offset : interiorOffsets)
            {
              if(inputData[static_cast<usize>(static_cast<int64>(p) + offset)] == backgroundValue)
              {
                boundary = true;
                break;
              }
            }
          }
          workData[p] = MaurerSignedValue(boundary ? 0.0f : k_Max, obj, insideIsPositive);
          if(boundary)
          {
            rowHasBoundary = true;
          }
        }
        initializeBoundedVoxel(nX - 1);
      }
      if(rowHasBoundary && !hasBoundary.load(std::memory_order_relaxed))
      {
        hasBoundary.store(true, std::memory_order_relaxed);
      }
    }
  };
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, static_cast<usize>(nZ) * static_cast<usize>(nY));
  parallelAlgorithm.execute(initializeRows);
  return hasBoundary.load(std::memory_order_relaxed);
}

/**
 * @struct Maurer3DSlabParameters
 * @brief Groups the immutable settings for the bounded 3D Maurer passes.
 * @tparam T Integer input value type.
 *
 * The seam keeps store references separate so tests can verify transfer order and concurrency.
 */
template <class T>
struct Maurer3DSlabParameters
{
  SizeVec3 dims;
  T backgroundValue;
  bool insideIsPositive;
  bool squaredDistance;
  bool useSpacing;
  FloatVec3 spacing;
  usize maxYRows;
  usize workerCount;
};

/**
 * @brief Runs resident 3D work with the configured worker limit.
 * @tparam Body Parallel range callable type.
 * @param count Number of independent work units.
 * @param workerCount Maximum number of TBB workers.
 * @param body Processes a range of independent resident work units.
 */
template <class Body>
void ExecuteMaurer3DParallel(usize count, usize workerCount, const Body& body)
{
  ParallelDataAlgorithm algorithm;
  algorithm.setRange(0, count);
#ifdef SIMPLNX_ENABLE_MULTICORE
  tbb::task_arena arena(static_cast<int>(std::max<usize>(1, workerCount)));
  arena.execute([&]() { algorithm.execute(body); });
#else
  static_cast<void>(workerCount);
  algorithm.execute(body);
#endif
}

/**
 * @brief Gathers one Y-row batch from all scratch Z planes.
 * @tparam WorkStoreT Store type that provides flat bulk reads.
 * @param workStore Provides transformed XY planes.
 * @param stagingBuffer Receives the batch in Z-Y-X order.
 * @param dims Image dimensions in X-Y-Z order.
 * @param yBegin First Y row in the batch.
 * @param yCount Number of Y rows in the batch.
 * @param shouldCancel Stops before the next store call when true.
 * @return An empty result on success or cancellation, or the first store error.
 *
 * Each read is a flat transfer because raw scratch stores do not provide an extent API.
 */
template <class WorkStoreT>
Result<> GatherMaurer3DZBatch(const WorkStoreT& workStore, nonstd::span<float32> stagingBuffer, const SizeVec3& dims, usize yBegin, usize yCount, const std::atomic_bool& shouldCancel)
{
  const usize valuesPerPlaneBlock = yCount * dims[0];
  float32* stagingData = stagingBuffer.data();
  for(usize z = 0; z < dims[2]; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize planeRowOffset = (z * dims[1] + yBegin) * dims[0];
    Result<> result = RunMaurerIoCallSafely([&]() { return workStore.copyIntoBuffer(planeRowOffset, nonstd::span<float32>(stagingData + z * valuesPerPlaneBlock, valuesPerPlaneBlock)); },
                                            "3D Z-pass scratch gather");
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}

/**
 * @brief Streams encoded initialization plus X and Y transforms into flat scratch storage.
 * @tparam T Integer input value type.
 * @tparam WorkStoreT Store type that provides flat bulk transfers.
 * @param inputStore Provides input planes in Z-Y-X order.
 * @param workStore Receives transformed XY planes in Z-Y-X order.
 * @param parameters Defines dimensions, signs, spacing, and worker limits.
 * @param shouldCancel Stops before the next store call when true.
 * @param hasBoundary Receives true when the complete scan finds a boundary voxel.
 * @return An empty result on success or cancellation, or the first store error.
 *
 * One I/O thread prefetches the next input plane while workers transform the current resident plane.
 */
template <class T, class WorkStoreT>
Result<> StreamMaurer3DInitAndTransformXY(const AbstractDataStore<T>& inputStore, WorkStoreT& workStore, const Maurer3DSlabParameters<T>& parameters, const std::atomic_bool& shouldCancel,
                                          bool& hasBoundary)
{
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  const int64 nX = static_cast<int64>(parameters.dims[0]);
  const int64 nY = static_cast<int64>(parameters.dims[1]);
  const int64 nZ = static_cast<int64>(parameters.dims[2]);
  const usize slice = parameters.dims[0] * parameters.dims[1];
  const bool is2D = nZ == 1;
  std::vector<T> prev(slice);
  std::vector<T> cur(slice);
  std::vector<T> next(slice);
  std::vector<float32> featurePlane(slice);
  const std::array<int64, 9> adjacentPlaneOffsets = {0, -1, 1, -nX, nX, -nX - 1, -nX + 1, nX - 1, nX + 1};
  const std::array<int64, 8> currentPlaneOffsets = {-1, 1, -nX, nX, -nX - 1, -nX + 1, nX - 1, nX + 1};
  std::atomic_bool boundaryFound{false};
  const auto flat2d = [nX](int64 x, int64 y) { return static_cast<usize>(y * nX + x); };

  if(shouldCancel)
  {
    return {};
  }
  if(Result<> result = RunMaurerIoCallSafely([&]() { return inputStore.copyIntoBuffer(0, nonstd::span<T>(cur.data(), slice)); }, "3D XY input plane 0 read"); result.invalid())
  {
    return result;
  }
  if(!is2D)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = RunMaurerIoCallSafely([&]() { return inputStore.copyIntoBuffer(slice, nonstd::span<T>(next.data(), slice)); }, "3D XY input plane 1 read"); result.invalid())
    {
      return result;
    }
  }

  for(int64 z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const bool hasPrev = z > 0;
    const bool hasNext = z < nZ - 1;
    auto initializeRows = [&](const Range& rowRange) {
      for(usize yIndex = rowRange.min(); yIndex < rowRange.max(); ++yIndex)
      {
        if(shouldCancel)
        {
          return;
        }
        const int64 y = static_cast<int64>(yIndex);
        const T* prevData = prev.data();
        const T* curData = cur.data();
        const T* nextData = next.data();
        float32* featurePlaneData = featurePlane.data();
        bool rowHasBoundary = false;
        auto initializeBoundedVoxel = [&](int64 x) {
          const usize planeIndex = flat2d(x, y);
          const bool isObject = curData[planeIndex] != parameters.backgroundValue;
          bool isBoundary = false;
          if(isObject)
          {
            const int64 minZOffset = is2D ? 0 : -1;
            const int64 maxZOffset = is2D ? 0 : 1;
            for(int64 zOffset = minZOffset; zOffset <= maxZOffset && !isBoundary; ++zOffset)
            {
              for(int64 yOffset = -1; yOffset <= 1 && !isBoundary; ++yOffset)
              {
                for(int64 xOffset = -1; xOffset <= 1 && !isBoundary; ++xOffset)
                {
                  if(xOffset == 0 && yOffset == 0 && zOffset == 0)
                  {
                    continue;
                  }
                  const int64 adjacentX = x + xOffset;
                  const int64 adjacentY = y + yOffset;
                  if(adjacentX < 0 || adjacentY < 0 || adjacentX >= nX || adjacentY >= nY || (zOffset < 0 && !hasPrev) || (zOffset > 0 && !hasNext))
                  {
                    continue; // ITK ignores out-of-bounds neighbors because the image border is not background.
                  }
                  const T neighborValue = zOffset == 0 ? curData[flat2d(adjacentX, adjacentY)] : zOffset < 0 ? prevData[flat2d(adjacentX, adjacentY)] : nextData[flat2d(adjacentX, adjacentY)];
                  if(neighborValue == parameters.backgroundValue)
                  {
                    isBoundary = true;
                  }
                }
              }
            }
          }
          featurePlaneData[planeIndex] = MaurerSignedValue(isBoundary ? 0.0f : k_Max, isObject, parameters.insideIsPositive);
          if(isBoundary)
          {
            rowHasBoundary = true;
          }
        };

        const bool isInteriorRow = hasPrev && hasNext && y >= 1 && y < nY - 1 && nX > 2;
        if(!isInteriorRow)
        {
          for(int64 x = 0; x < nX; ++x)
          {
            initializeBoundedVoxel(x);
          }
        }
        else
        {
          initializeBoundedVoxel(0);
          for(int64 x = 1; x < nX - 1; ++x)
          {
            const usize planeIndex = flat2d(x, y);
            const bool isObject = curData[planeIndex] != parameters.backgroundValue;
            bool isBoundary = false;
            if(isObject)
            {
              const int64 center = static_cast<int64>(planeIndex);
              for(const int64 offset : adjacentPlaneOffsets)
              {
                const usize neighborIndex = static_cast<usize>(center + offset);
                if(prevData[neighborIndex] == parameters.backgroundValue || nextData[neighborIndex] == parameters.backgroundValue)
                {
                  isBoundary = true;
                  break;
                }
              }
              if(!isBoundary)
              {
                for(const int64 offset : currentPlaneOffsets)
                {
                  if(curData[static_cast<usize>(center + offset)] == parameters.backgroundValue)
                  {
                    isBoundary = true;
                    break;
                  }
                }
              }
            }
            featurePlaneData[planeIndex] = MaurerSignedValue(isBoundary ? 0.0f : k_Max, isObject, parameters.insideIsPositive);
            if(isBoundary)
            {
              rowHasBoundary = true;
            }
          }
          initializeBoundedVoxel(nX - 1);
        }
        if(rowHasBoundary && !boundaryFound.load(std::memory_order_relaxed))
        {
          boundaryFound.store(true, std::memory_order_relaxed);
        }
      }
    };
    ParallelDataAlgorithm initializeParallelAlgorithm;
    initializeParallelAlgorithm.setRange(0, parameters.dims[1]);
    initializeParallelAlgorithm.execute(initializeRows);

    MaurerIoWorker ioWorker;
    if(z + 2 < nZ)
    {
      // The boundary scan releases prev before prefetch. Compute workers access only featurePlane.
      ioWorker.start([&, z]() -> Result<> {
        if(shouldCancel)
        {
          return {};
        }
        return inputStore.copyIntoBuffer(static_cast<usize>(z + 2) * slice, nonstd::span<T>(prev.data(), slice));
      });
    }

    auto transformXRows = [&](const Range& rowRange) {
      std::vector<float32> localG(parameters.dims[0]);
      std::vector<float32> localH(parameters.dims[0]);
      float32* featurePlaneData = featurePlane.data();
      for(usize y = rowRange.min(); y < rowRange.max(); ++y)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize offset = y * parameters.dims[0];
        Voronoi1DEncodedSign(nonstd::span<float32>(featurePlaneData + offset, parameters.dims[0]), parameters.dims[0], parameters.useSpacing, parameters.spacing[0], localG, localH);
      }
    };
    ParallelDataAlgorithm xParallelAlgorithm;
    xParallelAlgorithm.setRange(0, parameters.dims[1]);
    xParallelAlgorithm.execute(transformXRows);

    auto transformYColumns = [&](const Range& columnRange) {
      std::vector<float32> localG(parameters.dims[1]);
      std::vector<float32> localH(parameters.dims[1]);
      std::vector<float32> localLine(parameters.dims[1]);
      float32* featurePlaneData = featurePlane.data();
      float32* lineData = localLine.data();
      for(usize x = columnRange.min(); x < columnRange.max(); ++x)
      {
        if(shouldCancel)
        {
          return;
        }
        for(usize y = 0; y < parameters.dims[1]; ++y)
        {
          lineData[y] = featurePlaneData[y * parameters.dims[0] + x];
        }
        Voronoi1DEncodedSign(nonstd::span<float32>(lineData, parameters.dims[1]), parameters.dims[1], parameters.useSpacing, parameters.spacing[1], localG, localH);
        for(usize y = 0; y < parameters.dims[1]; ++y)
        {
          featurePlaneData[y * parameters.dims[0] + x] = lineData[y];
        }
      }
    };
    ParallelDataAlgorithm yParallelAlgorithm;
    yParallelAlgorithm.setRange(0, parameters.dims[0]);
    yParallelAlgorithm.execute(transformYColumns);

    ioWorker.join();
    if(ioWorker.result().invalid())
    {
      return ioWorker.result();
    }
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result =
           RunMaurerIoCallSafely([&]() { return workStore.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const float32>(featurePlane.data(), slice)); }, "3D XY scratch plane write");
       result.invalid())
    {
      return result;
    }

    if(!is2D && hasNext)
    {
      std::swap(prev, cur);
      std::swap(cur, next);
    }
  }
  hasBoundary = boundaryFound.load(std::memory_order_relaxed);
  return {};
}

/**
 * @brief Transforms bounded XZ staging buffers along Z and writes output extents.
 * @tparam T Integer input value type carried by the shared parameter seam.
 * @tparam WorkStoreT Store type that provides flat bulk reads.
 * @param workStore Provides transformed XY planes in Z-Y-X order.
 * @param outputStore Receives final signed distances through Y-batch extents.
 * @param parameters Defines dimensions, distance form, spacing, and worker limits.
 * @param shouldCancel Stops before the next store call when true.
 * @param hasBoundary True when initialization found at least one boundary voxel.
 * @return An empty result on success or cancellation, or the first store error.
 *
 * The I/O thread writes the previous batch and gathers the next batch while workers transform the current batch.
 */
template <class T, class WorkStoreT>
Result<> RunMaurer3DZPass(const WorkStoreT& workStore, AbstractDataStore<float32>& outputStore, const Maurer3DSlabParameters<T>& parameters, const std::atomic_bool& shouldCancel, bool hasBoundary)
{
  const usize nX = parameters.dims[0];
  const usize nY = parameters.dims[1];
  const usize nZ = parameters.dims[2];
  if(nZ == 1)
  {
    std::vector<float32> plane(nX * nY);
    if(Result<> result = RunMaurerIoCallSafely([&]() { return workStore.copyIntoBuffer(0, nonstd::span<float32>(plane.data(), plane.size())); }, "2D XY scratch plane read"); result.invalid())
    {
      return result;
    }
    if(!parameters.squaredDistance || !hasBoundary)
    {
      auto finalizeValues = [&](const Range& valueRange) {
        float32* planeData = plane.data();
        for(usize index = valueRange.min(); index < valueRange.max(); ++index)
        {
          if(parameters.squaredDistance)
          {
            planeData[index] = std::abs(planeData[index]);
          }
          else
          {
            const float32 magnitude = std::sqrt(std::abs(planeData[index]));
            planeData[index] = std::signbit(planeData[index]) ? -magnitude : magnitude;
          }
        }
      };
      ParallelDataAlgorithm finalizeParallelAlgorithm;
      finalizeParallelAlgorithm.setRange(0, plane.size());
      finalizeParallelAlgorithm.execute(finalizeValues);
    }
    if(shouldCancel)
    {
      return {};
    }
    return RunMaurerIoCallSafely([&]() { return outputStore.copyFromBuffer(0, nonstd::span<const float32>(plane.data(), plane.size())); }, "2D output plane write");
  }

  usize maxYRows = parameters.maxYRows;
  if(maxYRows == 0)
  {
    // This limit bounds both fallback staging buffers together.
    constexpr usize k_DefaultMaxStagingBytes = 16ULL * 1024ULL * 1024ULL;
    const usize maxStagedValues = std::max<usize>(1, (k_DefaultMaxStagingBytes / 2) / sizeof(float32));
    maxYRows = std::max<usize>(1, std::min(nY, maxStagedValues / nX / nZ));
  }
  else
  {
    maxYRows = std::min(maxYRows, nY);
  }
  if(const auto outputChunkShape = outputStore.getChunkShape(); outputChunkShape.has_value() && outputChunkShape->size() >= 3)
  {
    maxYRows = AlignMaurer3DYBatchRows(maxYRows, (*outputChunkShape)[1]);
  }

  const usize stagingValues = maxYRows * nX * nZ;
  std::vector<float32> stagingA(stagingValues);
  std::vector<float32> stagingB(stagingValues);
  std::vector<float32>* current = &stagingA;
  std::vector<float32>* other = &stagingB;
  const usize batchCount = 1 + (nY - 1) / maxYRows;
  const usize firstYCount = std::min(maxYRows, nY);
  if(Result<> result = GatherMaurer3DZBatch(workStore, nonstd::span<float32>(current->data(), current->size()), parameters.dims, 0, firstYCount, shouldCancel); result.invalid())
  {
    return result;
  }
  if(shouldCancel)
  {
    return {};
  }

  for(usize batchIndex = 0; batchIndex < batchCount; ++batchIndex)
  {
    const usize yBegin = batchIndex * maxYRows;
    const usize yCount = std::min(maxYRows, nY - yBegin);
    const usize valuesPerPlaneBlock = yCount * nX;
    const usize batchValues = valuesPerPlaneBlock * nZ;
    MaurerIoWorker ioWorker;
    // The I/O thread exclusively owns other until join. Compute workers exclusively own current during this interval.
    ioWorker.start([&, batchIndex, other]() -> Result<> {
      if(batchIndex > 0)
      {
        const usize previousYBegin = (batchIndex - 1) * maxYRows;
        const usize previousYCount = std::min(maxYRows, nY - previousYBegin);
        const usize previousBatchValues = previousYCount * nX * nZ;
        const Extent previousExtent({0, static_cast<uint64>(previousYBegin), 0}, {static_cast<uint64>(nZ - 1), static_cast<uint64>(previousYBegin + previousYCount - 1), static_cast<uint64>(nX - 1)});
        if(Result<> result = WriteExtentSafely(outputStore, previousExtent, nonstd::span<const float32>(other->data(), previousBatchValues), shouldCancel, "3D Z-pass output"); result.invalid())
        {
          return result;
        }
      }
      if(batchIndex + 1 < batchCount)
      {
        const usize nextYBegin = (batchIndex + 1) * maxYRows;
        const usize nextYCount = std::min(maxYRows, nY - nextYBegin);
        return GatherMaurer3DZBatch(workStore, nonstd::span<float32>(other->data(), other->size()), parameters.dims, nextYBegin, nextYCount, shouldCancel);
      }
      return {};
    });

    auto transformZLines = [&](const Range& lineRange) {
      std::vector<float32> localG(nZ);
      std::vector<float32> localH(nZ);
      std::vector<float32> localLine(nZ);
      float32* currentData = current->data();
      float32* lineData = localLine.data();
      for(usize line = lineRange.min(); line < lineRange.max(); ++line)
      {
        if(shouldCancel)
        {
          return;
        }
        for(usize z = 0; z < nZ; ++z)
        {
          lineData[z] = currentData[z * valuesPerPlaneBlock + line];
        }
        Voronoi1DEncodedSign(nonstd::span<float32>(lineData, nZ), nZ, parameters.useSpacing, parameters.spacing[2], localG, localH);
        for(usize z = 0; z < nZ; ++z)
        {
          currentData[z * valuesPerPlaneBlock + line] = lineData[z];
        }
      }
    };
    ExecuteMaurer3DParallel(valuesPerPlaneBlock, parameters.workerCount, transformZLines);

    if(!shouldCancel && (!parameters.squaredDistance || !hasBoundary))
    {
      auto finalizeValues = [&](const Range& valueRange) {
        float32* currentData = current->data();
        for(usize index = valueRange.min(); index < valueRange.max(); ++index)
        {
          if(parameters.squaredDistance)
          {
            currentData[index] = std::abs(currentData[index]);
          }
          else
          {
            const float32 magnitude = std::sqrt(std::abs(currentData[index]));
            currentData[index] = std::signbit(currentData[index]) ? -magnitude : magnitude;
          }
        }
      };
      ExecuteMaurer3DParallel(batchValues, parameters.workerCount, finalizeValues);
    }

    ioWorker.join();
    if(ioWorker.result().invalid())
    {
      return ioWorker.result();
    }
    if(shouldCancel)
    {
      return {};
    }
    if(batchIndex + 1 < batchCount)
    {
      // The swap makes the gathered next batch current and preserves the transformed batch in other for the next write.
      std::swap(current, other);
    }
  }

  const usize lastYBegin = (batchCount - 1) * maxYRows;
  const usize lastYCount = nY - lastYBegin;
  const usize lastBatchValues = lastYCount * nX * nZ;
  const Extent lastExtent({0, static_cast<uint64>(lastYBegin), 0}, {static_cast<uint64>(nZ - 1), static_cast<uint64>(lastYBegin + lastYCount - 1), static_cast<uint64>(nX - 1)});
  return WriteExtentSafely(outputStore, lastExtent, nonstd::span<const float32>(current->data(), lastBatchValues), shouldCancel, "3D Z-pass output");
}
} // namespace detail

/**
 * @class MaurerDistanceInCore
 * @brief Computes the in-core exact signed Maurer distance transform.
 * @tparam T Integer input value type.
 *
 * The algorithm borrows a resident input or makes one input copy. One float32 volume stores each distance and its final sign.
 * Independent lines run in parallel. Each axis completes before the next axis reads the work volume.
 * @pre The input and output stores contain one value for each image voxel.
 */
template <class T>
class MaurerDistanceInCore
{
public:
  MaurerDistanceInCore(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, T backgroundValue, bool insideIsPositive, bool squaredDistance, bool useSpacing,
                       FloatVec3 spacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Bg(backgroundValue)
  , m_InsidePos(insideIsPositive)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }
  ~MaurerDistanceInCore() = default;
  MaurerDistanceInCore(const MaurerDistanceInCore&) = delete;
  MaurerDistanceInCore(MaurerDistanceInCore&&) noexcept = delete;
  MaurerDistanceInCore& operator=(const MaurerDistanceInCore&) = delete;
  MaurerDistanceInCore& operator=(MaurerDistanceInCore&&) noexcept = delete;

  Result<> operator()()
  {
    const int64 nX = static_cast<int64>(m_Dims[0]);
    const int64 nY = static_cast<int64>(m_Dims[1]);
    const int64 nZ = static_cast<int64>(m_Dims[2]);
    const usize vol = static_cast<usize>(nX * nY * nZ);
    if(vol == 0)
    {
      return {};
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    std::unique_ptr<T[]> inputOwner;
    nonstd::span<const T> input;
    const auto* inStorePtr = dynamic_cast<const DataStore<T>*>(&m_In);
    if(inStorePtr != nullptr && m_In.getStoreType() != IDataStore::StoreType::OutOfCore)
    {
      input = inStorePtr->createSpan();
    }
    else
    {
      inputOwner = std::make_unique_for_overwrite<T[]>(vol);
      nonstd::span<T> inputBuffer(inputOwner.get(), vol);
      if(Result<> result = m_In.copyIntoBuffer(0, inputBuffer); result.invalid())
      {
        return result;
      }
      input = nonstd::span<const T>(inputOwner.get(), vol);
    }

    auto workOwner = std::make_unique_for_overwrite<float32[]>(vol);
    nonstd::span<float32> work(workOwner.get(), vol);
    const bool hasBoundary = detail::BuildMaurerEncodedInit<T>(input, m_Bg, m_Dims, m_InsidePos, work, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return {};
    }

    const float32 sp[3] = {m_Spacing[0], m_Spacing[1], m_Spacing[2]};

    const int32 dimCount = (nZ == 1) ? 2 : 3;
    const usize slice = static_cast<usize>(nX * nY);
    // The lines partition the volume for each axis. Workers update disjoint work values and keep line scratch private.
    // Each axis completes before the next axis reads the encoded distances.
    for(int32 d = 0; d < dimCount; ++d)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize nd = (d == 0) ? static_cast<usize>(nX) : (d == 1) ? static_cast<usize>(nY) : static_cast<usize>(nZ);
      const usize stride = (d == 0) ? 1u : (d == 1) ? static_cast<usize>(nX) : slice;
      // The line count is the product of the orthogonal extents. The flattened line index preserves Z-Y-X storage order.
      const usize numLines = (d == 0) ? (static_cast<usize>(nZ) * static_cast<usize>(nY)) :
                             (d == 1) ? (static_cast<usize>(nZ) * static_cast<usize>(nX)) :
                                        (static_cast<usize>(nY) * static_cast<usize>(nX));
      // Each worker reuses private envelope and line buffers across its assigned lines.
      auto processLines = [&](const Range& lineRange) {
        // The raw pointer avoids the per-element span contract check in the X-axis line kernel.
        float32* workData = work.data();
        std::vector<float32> g(nd), h(nd), lineBuf(nd);
        for(usize k = lineRange.min(); k < lineRange.max(); ++k)
        {
          if(m_ShouldCancel)
          {
            return;
          }
          const usize base = (d == 0) ? (k * static_cast<usize>(nX)) : (d == 1) ? ((k / static_cast<usize>(nX)) * slice + (k % static_cast<usize>(nX))) : k;
          if(d == 0)
          {
            detail::Voronoi1DEncodedSign(nonstd::span<float32>(workData + base, nd), nd, m_UseSpacing, sp[d], g, h);
          }
          else
          {
            RunLine(work, base, stride, nd, sp[d], g, h, lineBuf);
          }
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, numLines);
      parallelAlgorithm.execute(processLines);
      if(m_ShouldCancel)
      {
        return {};
      }
    }

    // The encoded sign remains unchanged while the final pass converts squared magnitudes to distances.
    if(!m_Squared)
    {
      auto finalizeDistances = [&](const Range& valueRange) {
        // The raw pointer avoids the per-element span contract check during finalization.
        float32* workData = work.data();
        for(usize p = valueRange.min(); p < valueRange.max(); ++p)
        {
          const float32 mag = std::sqrt(std::abs(workData[p]));
          workData[p] = std::signbit(workData[p]) ? -mag : mag;
        }
      };
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, vol);
      parallelAlgorithm.execute(finalizeDistances);
      if(m_ShouldCancel)
      {
        return {};
      }
    }
    else if(!hasBoundary)
    {
      // ITK leaves an unreached squared distance at positive FLT_MAX when the image has no boundary voxels.
      std::fill(work.begin(), work.end(), std::numeric_limits<float32>::max());
    }
    return m_Out.copyFromBuffer(0, nonstd::span<const float32>(work.data(), work.size()));
  }

private:
  // The buffer makes a strided line contiguous for the encoded-sign Voronoi pass.
  void RunLine(nonstd::span<float32> work, usize base, usize stride, usize nd, float32 spacingD, std::vector<float32>& g, std::vector<float32>& h, std::vector<float32>& lineBuf)
  {
    // The raw pointer avoids the per-element span contract check during gather and scatter.
    float32* workData = work.data();
    for(usize i = 0; i < nd; ++i)
    {
      lineBuf[i] = workData[base + i * stride];
    }
    detail::Voronoi1DEncodedSign(nonstd::span<float32>(lineBuf.data(), nd), nd, m_UseSpacing, spacingD, g, h);
    for(usize i = 0; i < nd; ++i)
    {
      workData[base + i * stride] = lineBuf[i];
    }
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  T m_Bg;
  bool m_InsidePos;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @class MaurerDistanceSlab
 * @brief Computes the out-of-core exact signed Maurer distance transform with bounded slabs.
 * @tparam T Integer input value type.
 *
 * Fused initialization, X, and Y passes stream one Z plane at a time.
 * One serialized I/O thread prefetches the next input plane while workers transform the current plane.
 * The Z pass alternates two bounded XZ staging buffers.
 * The I/O thread writes the previous Y batch and gathers the next batch while workers transform the current batch.
 * Each work value retains the inside or outside state in its sign. Later passes do not reread the input.
 * Initialization holds three input planes and one float32 work plane. The Z pass holds two staged XZ bands.
 * Both peaks add worker line scratch for the longest axis.
 * Store I/O remains serial while parallel workers access only resident buffers.
 */
template <class T>
class MaurerDistanceSlab
{
public:
  MaurerDistanceSlab(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, T backgroundValue, bool insideIsPositive, bool squaredDistance, bool useSpacing,
                     FloatVec3 spacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize residentLimit2D = detail::k_Maurer2DResidentLimit,
                     usize max3DYRows = 0, usize max3DWorkers = 1)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Bg(backgroundValue)
  , m_InsidePos(insideIsPositive)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_ResidentLimit2D(residentLimit2D)
  , m_Max3DYRows(max3DYRows)
  , m_Max3DWorkers(max3DWorkers)
  {
  }
  ~MaurerDistanceSlab() = default;
  MaurerDistanceSlab(const MaurerDistanceSlab&) = delete;
  MaurerDistanceSlab(MaurerDistanceSlab&&) noexcept = delete;
  MaurerDistanceSlab& operator=(const MaurerDistanceSlab&) = delete;
  MaurerDistanceSlab& operator=(MaurerDistanceSlab&&) noexcept = delete;

  Result<> operator()()
  {
    const int64 nX = static_cast<int64>(m_Dims[0]);
    const int64 nY = static_cast<int64>(m_Dims[1]);
    const int64 nZ = static_cast<int64>(m_Dims[2]);
    const usize slice = static_cast<usize>(nX * nY);
    const usize vol = slice * static_cast<usize>(nZ);
    if(vol == 0)
    {
      return {};
    }

    const bool hasOutOfCoreEndpoint = m_In.getStoreType() == IDataStore::StoreType::OutOfCore || m_Out.getStoreType() == IDataStore::StoreType::OutOfCore;
    if(nZ == 1 && hasOutOfCoreEndpoint)
    {
      return RunBounded2D(static_cast<usize>(nX), static_cast<usize>(nY));
    }

    // A disk-backed endpoint also makes the full-volume work store disk-backed.
    // One raw float32 record per cell avoids compression work for the complete write and read passes.
    // With resident endpoints, the resolved in-core format avoids unnecessary disk I/O.
    if(hasOutOfCoreEndpoint)
    {
      auto scratchResult = detail::CreateSweepTemporaryStore<float32>(vol, slice, m_ShouldCancel, "Signed Maurer distance-map 3D work");
      if(scratchResult.invalid())
      {
        return ConvertResult(std::move(scratchResult));
      }
      std::unique_ptr<detail::SweepTemporaryStore<float32>> workScratch = std::move(scratchResult.value());
      return Run3D(*workScratch);
    }

    const std::string workingFormat = detail::SelectDistanceWorkingDataFormat(m_In.getStoreType(), m_In.getDataFormat(), m_Out.getStoreType(), m_Out.getDataFormat());
    const ShapeType workTupleShape{static_cast<usize>(nZ), static_cast<usize>(nY), static_cast<usize>(nX)};
    std::optional<ShapeType> workChunkHint;
    if(const auto outputChunkShape = m_Out.getChunkShape(); outputChunkShape.has_value())
    {
      workChunkHint = detail::SelectMaurer3DScratchChunkHint(*outputChunkShape, workTupleShape);
    }
    auto workPtr = DataStoreUtilities::CreateDataStoreWithFormat<float32>(workingFormat, workTupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute, workChunkHint);
    return Run3D(*workPtr);
  }

private:
  // @p WorkStoreT is a template parameter (rather than a fixed AbstractDataStore<float32>) so the out-of-core route
  // above can pass a raw fixed-record detail::SweepTemporaryStore<float32> -- exposing only flat linear-offset bulk
  // transfers, no multi-dimensional extent API -- while the bounded-resident-memory route keeps passing a real
  // AbstractDataStore<float32> (a plain in-core allocation in that case, so an extent API costs nothing there).
  template <class WorkStoreT>
  Result<> Run3D(WorkStoreT& work)
  {
    const detail::Maurer3DSlabParameters<T> parameters{m_Dims, m_Bg, m_InsidePos, m_Squared, m_UseSpacing, m_Spacing, m_Max3DYRows, m_Max3DWorkers};
    bool hasBoundary = false;
    if(Result<> result = detail::StreamMaurer3DInitAndTransformXY(m_In, work, parameters, m_ShouldCancel, hasBoundary); result.invalid())
    {
      return result;
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    return detail::RunMaurer3DZPass(work, m_Out, parameters, m_ShouldCancel, hasBoundary);
  }

  template <class U>
  static std::string DescribeResultError(const Result<U>& result)
  {
    if(result.errors().empty())
    {
      return "provider returned an unspecified error";
    }
    const Error& error = result.errors().front();
    return fmt::format("{} (provider code {})", error.message, error.code);
  }

  Result<> WriteTemporaryRecordsSafely(ITemporaryRecordStore& store, uint64 recordOffset, uint64 recordCount, nonstd::span<const std::byte> records, std::string_view context)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      Result<> result = store.write(recordOffset, recordCount, records, m_ShouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: {}", context, DescribeResultError(result)));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  Result<> ReadTemporaryRecordsSafely(const ITemporaryRecordStore& store, uint64 recordOffset, uint64 recordCount, nonstd::span<std::byte> records, std::string_view context)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      Result<uint64> result = store.read(recordOffset, recordCount, records, m_ShouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: {}", context, DescribeResultError(result)));
      }
      if(result.value() != recordCount)
      {
        return MakeErrorResult(-8358,
                               fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: temporary record read returned {} of {} records", context, result.value(), recordCount));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8358, fmt::format("Signed Maurer distance-map bulk extent transfer failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  Result<> RunBounded2D(usize nx, usize ny)
  {
    const detail::Maurer2DBufferPlan plan = detail::BuildMaurer2DBufferPlan(nx, ny, sizeof(T), m_ResidentLimit2D);
    if(plan.overflow)
    {
      return MakeErrorResult(-8356, fmt::format("Signed Maurer distance-map 2D buffer plan cannot represent dimensions {} due to arithmetic overflow.", StringUtilities::formatDimensions3D(m_Dims)));
    }
    if(!plan.valid)
    {
      return MakeErrorResult(-8355, fmt::format("Signed Maurer distance-map 2D buffer plan exceeds the 67108864-byte resident limit. Dimensions: {}; input bytes: {}.",
                                                StringUtilities::formatDimensions3D(m_Dims), sizeof(T)));
    }
    usize directRowBatchRows = plan.rowBatchRows;
    if(!plan.spillX && !plan.spillY)
    {
      const std::optional<ShapeType> outputChunkShape = m_Out.getChunkShape();
      if(outputChunkShape.has_value() && outputChunkShape->size() >= 3 && (*outputChunkShape)[1] > 0 && (*outputChunkShape)[1] <= directRowBatchRows)
      {
        directRowBatchRows = (directRowBatchRows / (*outputChunkShape)[1]) * (*outputChunkShape)[1];
      }
    }

    const std::string format = detail::SelectDistanceWorkingDataFormat(m_In.getStoreType(), m_In.getDataFormat(), m_Out.getStoreType(), m_Out.getDataFormat());
    std::shared_ptr<AbstractDataStore<float32>> transposedWork;
    std::shared_ptr<AbstractDataStore<uint8>> transposedInside;
    std::shared_ptr<AbstractDataStore<float32>> envelopeG;
    std::shared_ptr<AbstractDataStore<float32>> envelopeH;
    std::shared_ptr<AbstractDataStore<float32>> xLine;
    std::shared_ptr<AbstractDataStore<uint8>> xInside;
    std::unique_ptr<ITemporaryRecordStore> directRecords;
    usize directRecordSize = 0;
    try
    {
      if(plan.spillX || plan.spillY)
      {
        transposedWork = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, std::vector<usize>{1, nx, ny}, std::vector<usize>{1});
        transposedInside = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(format, std::vector<usize>{1, nx, ny}, std::vector<usize>{1});
        const usize maxLine = std::max(nx, ny);
        envelopeG = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, std::vector<usize>{maxLine}, std::vector<usize>{1});
        envelopeH = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, std::vector<usize>{maxLine}, std::vector<usize>{1});
      }
      if(plan.spillX)
      {
        xLine = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, std::vector<usize>{nx}, std::vector<usize>{1});
        xInside = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(format, std::vector<usize>{nx}, std::vector<usize>{1});
      }
    } catch(const std::bad_alloc& exception)
    {
      return MakeErrorResult(-8357, fmt::format("Signed Maurer distance-map failed to create 2D scratch store using data format '{}': {}", format, exception.what()));
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8357, fmt::format("Signed Maurer distance-map failed to create 2D scratch store using data format '{}': {}", format, exception.what()));
    }

    const auto isOutOfCore = [](const auto& store) { return store != nullptr && store->getStoreType() == IDataStore::StoreType::OutOfCore; };
    if(((plan.spillX || plan.spillY) && (!isOutOfCore(transposedWork) || !isOutOfCore(transposedInside) || !isOutOfCore(envelopeG) || !isOutOfCore(envelopeH))) ||
       (plan.spillX && (!isOutOfCore(xLine) || !isOutOfCore(xInside))))
    {
      return MakeErrorResult(-8357, fmt::format("Signed Maurer distance-map failed to create 2D scratch store using data format '{}': resolved store is not out-of-core", format));
    }
    if(!plan.spillY)
    {
      const usize directBatchCount = 1 + (ny - 1) / directRowBatchRows;
      usize directRecordCount = 0;
      if(!detail::MaurerCheckedMultiply(directRowBatchRows, sizeof(float32) + sizeof(uint8), directRecordSize) || !detail::MaurerCheckedMultiply(directBatchCount, nx, directRecordCount) ||
         directRecordSize > std::numeric_limits<uint64>::max() || directRecordCount > std::numeric_limits<uint64>::max())
      {
        return MakeErrorResult(
            -8356, fmt::format("Signed Maurer distance-map 2D temporary-record layout cannot represent dimensions {} due to arithmetic overflow.", StringUtilities::formatDimensions3D(m_Dims)));
      }
      TemporaryRecordStoreConfig config;
      config.recordSize = static_cast<uint64>(directRecordSize);
      config.maxRecordsPerBatch = static_cast<uint64>(plan.columnBatchCols);
      config.initialRecordCount = static_cast<uint64>(directRecordCount);
      try
      {
        auto storeResult = DataStoreUtilities::CreateTemporaryRecordStore(config);
        if(storeResult.invalid())
        {
          return MakeErrorResult(-8357, fmt::format("Signed Maurer distance-map failed to create direct-transpose scratch store using data format '{}': {}", format, DescribeResultError(storeResult)));
        }
        directRecords = std::move(storeResult.value());
      } catch(const std::exception& exception)
      {
        return MakeErrorResult(-8357, fmt::format("Signed Maurer distance-map failed to create direct-transpose scratch store using data format '{}': {}", format, exception.what()));
      }
      if(directRecords == nullptr)
      {
        return MakeErrorResult(-8357, fmt::format("Signed Maurer distance-map failed to create direct-transpose scratch store using data format '{}': provider returned a null store", format));
      }
    }
    constexpr float32 k_Max = std::numeric_limits<float32>::max();

    // Phase 1: feature initialization plus X transform, written directly in transposed storage order.
    if(!plan.spillX)
    {
      for(usize yBegin = 0; yBegin < ny; yBegin += directRowBatchRows)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize rowCount = std::min(directRowBatchRows, ny - yBegin);
        const usize haloBegin = yBegin == 0 ? 0 : yBegin - 1;
        const usize haloEnd = std::min(ny, yBegin + rowCount + 1);
        const usize haloRows = haloEnd - haloBegin;
        std::vector<float32> rowWork(rowCount * nx, k_Max);
        std::vector<uint8> rowInside(rowCount * nx, 0);
        {
          std::vector<T> input(haloRows * nx);
          if(Result<> result = m_In.copyIntoBuffer(haloBegin * nx, nonstd::span<T>(input.data(), input.size())); result.invalid())
          {
            return result;
          }
          auto processRows = [&](const Range& range) {
            std::vector<float32> g(nx);
            std::vector<float32> h(nx);
            for(usize localY = range.min(); localY < range.max(); ++localY)
            {
              const usize y = yBegin + localY;
              for(usize x = 0; x < nx; ++x)
              {
                const usize localIndex = localY * nx + x;
                const bool object = input[(y - haloBegin) * nx + x] != m_Bg;
                rowInside[localIndex] = object ? uint8{1} : uint8{0};
                if(!object)
                {
                  continue;
                }
                bool boundary = false;
                for(int64 deltaY = -1; deltaY <= 1 && !boundary; ++deltaY)
                {
                  for(int64 deltaX = -1; deltaX <= 1 && !boundary; ++deltaX)
                  {
                    if(deltaX == 0 && deltaY == 0)
                    {
                      continue;
                    }
                    const int64 adjacentX = static_cast<int64>(x) + deltaX;
                    const int64 adjacentY = static_cast<int64>(y) + deltaY;
                    if(adjacentX < 0 || adjacentY < 0 || adjacentX >= static_cast<int64>(nx) || adjacentY >= static_cast<int64>(ny))
                    {
                      continue;
                    }
                    if(input[(static_cast<usize>(adjacentY) - haloBegin) * nx + static_cast<usize>(adjacentX)] == m_Bg)
                    {
                      boundary = true;
                    }
                  }
                }
                rowWork[localIndex] = boundary ? 0.0f : k_Max;
              }
              detail::Voronoi1D(nonstd::span<float32>(rowWork.data() + localY * nx, nx), nonstd::span<const uint8>(rowInside.data() + localY * nx, nx), nx, m_InsidePos, m_UseSpacing, m_Spacing[0], g,
                                h);
            }
          };
          detail::ExecuteMaurer2DParallel(rowCount, processRows);
        }

        if(plan.spillY)
        {
          const Extent transposedExtent({0, 0, static_cast<uint64>(yBegin)}, {0, static_cast<uint64>(nx - 1), static_cast<uint64>(yBegin + rowCount - 1)});
          {
            std::vector<float32> transposed(rowCount * nx);
            for(usize x = 0; x < nx; ++x)
            {
              for(usize localY = 0; localY < rowCount; ++localY)
              {
                transposed[x * rowCount + localY] = rowWork[localY * nx + x];
              }
            }
            if(Result<> result = detail::WriteExtentSafely(*transposedWork, transposedExtent, nonstd::span<const float32>(transposed.data(), transposed.size()), m_ShouldCancel, "2D work transpose");
               result.invalid())
            {
              return result;
            }
          }
          std::vector<float32>().swap(rowWork);
          {
            std::vector<uint8> transposed(rowCount * nx);
            for(usize x = 0; x < nx; ++x)
            {
              for(usize localY = 0; localY < rowCount; ++localY)
              {
                transposed[x * rowCount + localY] = rowInside[localY * nx + x];
              }
            }
            if(Result<> result = detail::WriteExtentSafely(*transposedInside, transposedExtent, nonstd::span<const uint8>(transposed.data(), transposed.size()), m_ShouldCancel, "2D inside transpose");
               result.invalid())
            {
              return result;
            }
          }
        }
        else
        {
          const usize batchRecordOffset = (yBegin / directRowBatchRows) * nx;
          for(usize xBegin = 0; xBegin < nx; xBegin += plan.columnBatchCols)
          {
            const usize columnCount = std::min(plan.columnBatchCols, nx - xBegin);
            std::vector<std::byte> recordBytes(columnCount * directRecordSize);
            auto packRecords = [&](const Range& range) {
              for(usize localX = range.min(); localX < range.max(); ++localX)
              {
                std::byte* record = recordBytes.data() + localX * directRecordSize;
                for(usize localY = 0; localY < rowCount; ++localY)
                {
                  const float32 value = rowWork[localY * nx + xBegin + localX];
                  std::memcpy(record + localY * sizeof(float32), &value, sizeof(float32));
                  record[directRowBatchRows * sizeof(float32) + localY] = static_cast<std::byte>(rowInside[localY * nx + xBegin + localX]);
                }
              }
            };
            detail::ExecuteMaurer2DParallel(columnCount, packRecords);
            if(Result<> result = WriteTemporaryRecordsSafely(*directRecords, batchRecordOffset + xBegin, columnCount, nonstd::span<const std::byte>(recordBytes.data(), recordBytes.size()),
                                                             "2D direct-transpose write");
               result.invalid())
            {
              return result;
            }
          }
        }
      }
    }
    else
    {
      const usize blockValues = plan.lineBlockValues;
      const usize capacity = std::min(nx, blockValues + 1);
      for(usize y = 0; y < ny; ++y)
      {
        {
          std::vector<float32> feature(capacity);
          std::vector<uint8> insideValues(capacity);
          for(usize xBegin = 0; xBegin < nx;)
          {
            if(m_ShouldCancel)
            {
              return {};
            }
            const detail::MaurerLineChunk chunk = detail::GetMaurerLineChunk(xBegin, nx, blockValues);
            const usize xHaloBegin = chunk.begin == 0 ? 0 : chunk.begin - 1;
            const usize xHaloEnd = std::min(nx, chunk.begin + chunk.count + 1);
            const usize xHaloCount = xHaloEnd - xHaloBegin;
            const usize yHaloBegin = y == 0 ? 0 : y - 1;
            const usize yHaloEnd = std::min(ny, y + 2);
            const usize yHaloCount = yHaloEnd - yHaloBegin;
            std::vector<T> input(yHaloCount * xHaloCount);
            for(usize haloY = 0; haloY < yHaloCount; ++haloY)
            {
              if(Result<> result = m_In.copyIntoBuffer((yHaloBegin + haloY) * nx + xHaloBegin, nonstd::span<T>(input.data() + haloY * xHaloCount, xHaloCount)); result.invalid())
              {
                return result;
              }
            }
            for(usize localX = 0; localX < chunk.count; ++localX)
            {
              const usize x = chunk.begin + localX;
              const bool object = input[(y - yHaloBegin) * xHaloCount + (x - xHaloBegin)] != m_Bg;
              insideValues[localX] = object ? uint8{1} : uint8{0};
              feature[localX] = k_Max;
              if(!object)
              {
                continue;
              }
              bool boundary = false;
              for(int64 deltaY = -1; deltaY <= 1 && !boundary; ++deltaY)
              {
                for(int64 deltaX = -1; deltaX <= 1 && !boundary; ++deltaX)
                {
                  if(deltaX == 0 && deltaY == 0)
                  {
                    continue;
                  }
                  const int64 adjacentX = static_cast<int64>(x) + deltaX;
                  const int64 adjacentY = static_cast<int64>(y) + deltaY;
                  if(adjacentX < 0 || adjacentY < 0 || adjacentX >= static_cast<int64>(nx) || adjacentY >= static_cast<int64>(ny))
                  {
                    continue;
                  }
                  if(input[(static_cast<usize>(adjacentY) - yHaloBegin) * xHaloCount + (static_cast<usize>(adjacentX) - xHaloBegin)] == m_Bg)
                  {
                    boundary = true;
                  }
                }
              }
              feature[localX] = boundary ? 0.0f : k_Max;
            }
            if(Result<> result = xLine->copyFromBuffer(chunk.begin, nonstd::span<const float32>(feature.data(), chunk.count)); result.invalid())
            {
              return result;
            }
            if(Result<> result = xInside->copyFromBuffer(chunk.begin, nonstd::span<const uint8>(insideValues.data(), chunk.count)); result.invalid())
            {
              return result;
            }
            const Extent insideExtent({0, static_cast<uint64>(chunk.begin), static_cast<uint64>(y)}, {0, static_cast<uint64>(chunk.begin + chunk.count - 1), static_cast<uint64>(y)});
            if(Result<> result = detail::WriteExtentSafely(*transposedInside, insideExtent, nonstd::span<const uint8>(insideValues.data(), chunk.count), m_ShouldCancel, "2D spill-X inside transpose");
               result.invalid())
            {
              return result;
            }
            xBegin = chunk.begin + chunk.count;
          }
        }
        auto writeTransposedX = [&](usize blockBegin, nonstd::span<float32> values, nonstd::span<const uint8>) {
          const Extent workExtent({0, static_cast<uint64>(blockBegin), static_cast<uint64>(y)}, {0, static_cast<uint64>(blockBegin + values.size() - 1), static_cast<uint64>(y)});
          return detail::WriteExtentSafely(*transposedWork, workExtent, nonstd::span<const float32>(values.data(), values.size()), m_ShouldCancel, "2D spill-X work transpose");
        };
        if(Result<> result = detail::ExternalVoronoi1DToSink(*xLine, *xInside, 0, nx, m_InsidePos, m_UseSpacing, m_Spacing[0], blockValues, *envelopeG, *envelopeH, writeTransposedX, m_ShouldCancel);
           result.invalid())
        {
          return result;
        }
      }
    }

    // Phase 2: contiguous transposed Y lines plus final sign/sqrt, written directly to the output geometry.
    if(!plan.spillY)
    {
      for(usize xBegin = 0; xBegin < nx; xBegin += plan.columnBatchCols)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize columnCount = std::min(plan.columnBatchCols, nx - xBegin);
        std::vector<float32> columns(columnCount * ny);
        {
          std::vector<uint8> insideValues(columnCount * ny);
          for(usize yBegin = 0; yBegin < ny; yBegin += directRowBatchRows)
          {
            const usize rowCount = std::min(directRowBatchRows, ny - yBegin);
            const usize batchRecordOffset = (yBegin / directRowBatchRows) * nx;
            std::vector<std::byte> recordBytes(columnCount * directRecordSize);
            if(Result<> result =
                   ReadTemporaryRecordsSafely(*directRecords, batchRecordOffset + xBegin, columnCount, nonstd::span<std::byte>(recordBytes.data(), recordBytes.size()), "2D direct-transpose read");
               result.invalid())
            {
              return result;
            }
            auto unpackRecords = [&](const Range& range) {
              for(usize localX = range.min(); localX < range.max(); ++localX)
              {
                const std::byte* record = recordBytes.data() + localX * directRecordSize;
                for(usize localY = 0; localY < rowCount; ++localY)
                {
                  std::memcpy(columns.data() + localX * ny + yBegin + localY, record + localY * sizeof(float32), sizeof(float32));
                  insideValues[localX * ny + yBegin + localY] = static_cast<uint8>(record[directRowBatchRows * sizeof(float32) + localY]);
                }
              }
            };
            detail::ExecuteMaurer2DParallel(columnCount, unpackRecords);
          }
          auto processColumns = [&](const Range& range) {
            // Raw pointers avoid the per-element span contract check during finalization.
            float32* columnsData = columns.data();
            const uint8* insideValuesData = insideValues.data();
            std::vector<float32> g(ny);
            std::vector<float32> h(ny);
            for(usize localX = range.min(); localX < range.max(); ++localX)
            {
              float32* lineData = columnsData + localX * ny;
              const uint8* lineInsideData = insideValuesData + localX * ny;
              detail::Voronoi1D(nonstd::span<float32>(lineData, ny), nonstd::span<const uint8>(lineInsideData, ny), ny, m_InsidePos, m_UseSpacing, m_Spacing[1], g, h);
              if(!m_Squared)
              {
                for(usize y = 0; y < ny; ++y)
                {
                  const float32 magnitude = std::sqrt(std::abs(lineData[y]));
                  lineData[y] = ((lineInsideData[y] != 0) == m_InsidePos) ? magnitude : -magnitude;
                }
              }
            }
          };
          detail::ExecuteMaurer2DParallel(columnCount, processColumns);
          for(usize yBegin = 0; yBegin < ny; yBegin += directRowBatchRows)
          {
            const usize rowCount = std::min(directRowBatchRows, ny - yBegin);
            const usize batchRecordOffset = (yBegin / directRowBatchRows) * nx;
            std::vector<std::byte> recordBytes(columnCount * directRecordSize);
            auto packRecords = [&](const Range& range) {
              for(usize localX = range.min(); localX < range.max(); ++localX)
              {
                std::byte* record = recordBytes.data() + localX * directRecordSize;
                for(usize localY = 0; localY < rowCount; ++localY)
                {
                  const float32 value = columns[localX * ny + yBegin + localY];
                  std::memcpy(record + localY * sizeof(float32), &value, sizeof(float32));
                  record[directRowBatchRows * sizeof(float32) + localY] = static_cast<std::byte>(insideValues[localX * ny + yBegin + localY]);
                }
              }
            };
            detail::ExecuteMaurer2DParallel(columnCount, packRecords);
            if(Result<> result = WriteTemporaryRecordsSafely(*directRecords, batchRecordOffset + xBegin, columnCount, nonstd::span<const std::byte>(recordBytes.data(), recordBytes.size()),
                                                             "2D transformed-record write");
               result.invalid())
            {
              return result;
            }
          }
        }
      }

      for(usize yBegin = 0; yBegin < ny; yBegin += directRowBatchRows)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize rowCount = std::min(directRowBatchRows, ny - yBegin);
        const usize batchRecordOffset = (yBegin / directRowBatchRows) * nx;
        std::vector<float32> output(nx * rowCount);
        for(usize xBegin = 0; xBegin < nx; xBegin += plan.columnBatchCols)
        {
          const usize columnCount = std::min(plan.columnBatchCols, nx - xBegin);
          std::vector<std::byte> recordBytes(columnCount * directRecordSize);
          if(Result<> result =
                 ReadTemporaryRecordsSafely(*directRecords, batchRecordOffset + xBegin, columnCount, nonstd::span<std::byte>(recordBytes.data(), recordBytes.size()), "2D final-output record read");
             result.invalid())
          {
            return result;
          }
          auto unpackOutput = [&](const Range& range) {
            for(usize localX = range.min(); localX < range.max(); ++localX)
            {
              const std::byte* record = recordBytes.data() + localX * directRecordSize;
              for(usize localY = 0; localY < rowCount; ++localY)
              {
                std::memcpy(output.data() + localY * nx + xBegin + localX, record + localY * sizeof(float32), sizeof(float32));
              }
            }
          };
          detail::ExecuteMaurer2DParallel(columnCount, unpackOutput);
        }
        if(Result<> result = m_Out.copyFromBuffer(yBegin * nx, nonstd::span<const float32>(output.data(), output.size())); result.invalid())
        {
          return result;
        }
      }
    }
    else
    {
      const usize blockValues = plan.lineBlockValues;
      for(usize x = 0; x < nx; ++x)
      {
        const usize offset = x * ny;
        auto writeOutputY = [&](usize blockBegin, nonstd::span<float32> values, nonstd::span<const uint8> insideValues) {
          // Raw pointers avoid the per-element span contract check during finalization.
          float32* valuesData = values.data();
          const uint8* insideValuesData = insideValues.data();
          if(!m_Squared)
          {
            for(usize index = 0; index < values.size(); ++index)
            {
              const float32 magnitude = std::sqrt(std::abs(valuesData[index]));
              valuesData[index] = ((insideValuesData[index] != 0) == m_InsidePos) ? magnitude : -magnitude;
            }
          }
          const Extent outputExtent({0, static_cast<uint64>(blockBegin), static_cast<uint64>(x)}, {0, static_cast<uint64>(blockBegin + values.size() - 1), static_cast<uint64>(x)});
          return detail::WriteExtentSafely(m_Out, outputExtent, nonstd::span<const float32>(values.data(), values.size()), m_ShouldCancel, "2D spill-Y output");
        };
        if(Result<> result = detail::ExternalVoronoi1DToSink(*transposedWork, *transposedInside, offset, ny, m_InsidePos, m_UseSpacing, m_Spacing[1], blockValues, *envelopeG, *envelopeH, writeOutputY,
                                                             m_ShouldCancel);
           result.invalid())
        {
          return result;
        }
      }
    }
    return {};
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  T m_Bg;
  bool m_InsidePos;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_ResidentLimit2D = detail::k_Maurer2DResidentLimit;
  usize m_Max3DYRows = 0;
  usize m_Max3DWorkers = 1;
};

template <class T>
class MaurerDistanceWorkingMemory
{
public:
  MaurerDistanceWorkingMemory(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, T backgroundValue, bool insideIsPositive, bool squaredDistance, bool useSpacing,
                              FloatVec3 spacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Bg(backgroundValue)
  , m_InsidePos(insideIsPositive)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    if(detail::ShouldUseMaurerResidentState(m_Dims))
    {
      auto allocationResult = detail::ReserveMaurerResidentWorkingMemory<T>(m_Dims);
      if(allocationResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(allocationResult));
      }
      auto allocation = std::move(allocationResult.value());
      if(allocation.holdsCompleteState())
      {
        try
        {
          return MaurerDistanceInCore<T>{m_In, m_Out, m_Dims, m_Bg, m_InsidePos, m_Squared, m_UseSpacing, m_Spacing, m_ShouldCancel, m_MessageHandler}();
        } catch(const std::bad_alloc&)
        {
          // The complete-state allocation is optional. Reuse its reservation for the slab plan below.
        }
      }
      if(allocation.reservation.sizeBytes() > 0 && allocation.reservation.sizeBytes() <= std::numeric_limits<usize>::max())
      {
        auto slabPlanResult = detail::CreateMaurer3DSlabMemoryPlan<T>(m_Dims, static_cast<usize>(allocation.reservation.sizeBytes()));
        if(slabPlanResult.valid())
        {
          allocation.reservation.shrinkTo(static_cast<uint64>(slabPlanResult.value().residentBytes));
          return MaurerDistanceSlab<T>{m_In,
                                       m_Out,
                                       m_Dims,
                                       m_Bg,
                                       m_InsidePos,
                                       m_Squared,
                                       m_UseSpacing,
                                       m_Spacing,
                                       m_ShouldCancel,
                                       m_MessageHandler,
                                       detail::k_Maurer2DResidentLimit,
                                       slabPlanResult.value().maxYRows,
                                       slabPlanResult.value().workerCount}();
        }
      }
    }
    return MaurerDistanceSlab<T>{m_In, m_Out, m_Dims, m_Bg, m_InsidePos, m_Squared, m_UseSpacing, m_Spacing, m_ShouldCancel, m_MessageHandler}();
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  T m_Bg;
  bool m_InsidePos;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Dispatches the signed Maurer distance transform for the input and output storage types.
 * @tparam T Integer input value type.
 * @param inStore Provides input values.
 * @param outStore Receives float32 distance values.
 * @param dims Image dimensions in X-Y-Z order.
 * @param backgroundValue Value that identifies background voxels.
 * @param insideIsPositive Selects a positive result for inside voxels when true.
 * @param squaredDistance Keeps squared distance magnitudes when true.
 * @param useSpacing Applies physical image spacing when true.
 * @param spacing Physical image spacing in X-Y-Z order.
 * @param inArray Identifies the input storage route.
 * @param outArray Identifies the output storage route.
 * @param shouldCancel Stops processing between bounded work units when true.
 * @param messageHandler Receives algorithm messages.
 * @return An empty result on success or cancellation, or an error for invalid sizes or transfers.
 *
 * Resident arrays use the in-core algorithm. Other routes require a complete memory grant or use bounded slabs.
 */
template <class T>
Result<> ApplySignedMaurerDistanceMap(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, T backgroundValue, bool insideIsPositive, bool squaredDistance,
                                      bool useSpacing, FloatVec3 spacing, const IDataArray& inArray, IDataArray& outArray, const std::atomic_bool& shouldCancel,
                                      const IFilter::MessageHandler& messageHandler)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }

  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dims[0] > k_Int64Max || dims[1] > k_Int64Max || dims[2] > k_Int64Max)
  {
    return MakeErrorResult(-8354, fmt::format("Signed Maurer distance-map dimensions exceed the signed 64-bit indexing limit. Actual dimensions: {}.", StringUtilities::formatDimensions3D(dims)));
  }

  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  const std::string formattedDims = StringUtilities::formatDimensions3D(dims);
  usize slice = 0;
  if(!checkedMultiply(dims[0], dims[1], slice))
  {
    return MakeErrorResult(-8350, fmt::format("Signed Maurer distance-map dimensions overflow usize while computing the XY plane size. Actual dimensions: {}.", formattedDims));
  }
  usize volume = 0;
  if(!checkedMultiply(slice, dims[2], volume))
  {
    return MakeErrorResult(-8351, fmt::format("Signed Maurer distance-map dimensions overflow usize while computing the XYZ volume. Actual dimensions: {}.", formattedDims));
  }
  if(inStore.getSize() != volume)
  {
    return MakeErrorResult(-8352,
                           fmt::format("Signed Maurer distance-map input store size ({}) does not match the expected XYZ volume ({}) for dimensions {}.", inStore.getSize(), volume, formattedDims));
  }
  if(outStore.getSize() != volume)
  {
    return MakeErrorResult(-8353,
                           fmt::format("Signed Maurer distance-map output store size ({}) does not match the expected XYZ volume ({}) for dimensions {}.", outStore.getSize(), volume, formattedDims));
  }

  return DispatchAlgorithm<MaurerDistanceInCore<T>, MaurerDistanceWorkingMemory<T>>({&inArray, &outArray}, inStore, outStore, dims, backgroundValue, insideIsPositive, squaredDistance, useSpacing,
                                                                                    spacing, shouldCancel, messageHandler);
}
} // namespace nx::core::ImageProcessing
