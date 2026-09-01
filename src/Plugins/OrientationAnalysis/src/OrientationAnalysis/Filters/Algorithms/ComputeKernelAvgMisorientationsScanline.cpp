#include "ComputeKernelAvgMisorientationsScanline.hpp"
#include "ComputeKernelAvgMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ParallelData2DAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cassert>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

using namespace nx::core;

namespace nx::core
{
namespace
{
constexpr uint64 k_InputBytesPerTuple = 2 * sizeof(int32) + 4 * sizeof(float32);
constexpr uint64 k_OutputBytesPerTuple = sizeof(float32);
constexpr usize k_TargetBlockTuples = 65536;
constexpr int32 k_WorkingSetOverflowError = -67200;
constexpr int32 k_WorkingSetInputError = -67202;

/**
 * @brief Multiplies values without an unsigned overflow.
 * @tparam ValueT Specifies the unsigned value type.
 * @param lhs Identifies the first factor.
 * @param rhs Identifies the second factor.
 * @param product Receives the product when multiplication succeeds.
 * @return True when multiplication succeeds.
 */
template <typename ValueT>
bool CheckedMultiply(ValueT lhs, ValueT rhs, ValueT& product)
{
  if(lhs != 0 && rhs > std::numeric_limits<ValueT>::max() / lhs)
  {
    return false;
  }

  product = lhs * rhs;
  return true;
}

/**
 * @brief Adds values without an unsigned overflow.
 * @tparam ValueT Specifies the unsigned value type.
 * @param lhs Identifies the first addend.
 * @param rhs Identifies the second addend.
 * @param sum Receives the sum when addition succeeds.
 * @return True when addition succeeds.
 */
template <typename ValueT>
bool CheckedAdd(ValueT lhs, ValueT rhs, ValueT& sum)
{
  if(rhs > std::numeric_limits<ValueT>::max() - lhs)
  {
    return false;
  }

  sum = lhs + rhs;
  return true;
}

/**
 * @brief Creates a KAM working-set overflow error.
 * @param dimensions Identifies the Image Geometry dimensions in X, Y, Z order.
 * @param kernelSize Identifies the kernel radii in X, Y, Z order.
 * @param cacheBudgetBytes Specifies the cache budget in bytes.
 * @param imageGeometryPath Identifies the Image Geometry in the error message.
 * @param overflowedQuantity Identifies the quantity that overflowed.
 * @return A working-set overflow error with the affected input context.
 */
Result<ComputeKernelAvgMisorientationsWorkingSet> MakeWorkingSetOverflowResult(const SizeVec3& dimensions, const VectorInt32Parameter::ValueType& kernelSize, uint64 cacheBudgetBytes,
                                                                               const DataPath& imageGeometryPath, std::string_view overflowedQuantity)
{
  return MakeErrorResult<ComputeKernelAvgMisorientationsWorkingSet>(
      k_WorkingSetOverflowError,
      fmt::format("Compute Kernel Average Misorientations cannot size its working set for Image Geometry '{}' with dimensions ({}) and kernel radius ({}, {}, {}) under cache budget {} bytes "
                  "because {} overflows.",
                  imageGeometryPath.toString(), StringUtilities::formatDimensions3D(dimensions), kernelSize[0], kernelSize[1], kernelSize[2], cacheBudgetBytes, overflowedQuantity));
}
} // namespace

Result<ComputeKernelAvgMisorientationsWorkingSet> CreateComputeKernelAvgMisorientationsWorkingSet(const SizeVec3& dimensions, const VectorInt32Parameter::ValueType& kernelSize,
                                                                                                  uint64 cacheBudgetBytes, uint64 cacheUsedBytes, const DataPath& imageGeometryPath)
{
  if(kernelSize.size() != 3)
  {
    return MakeErrorResult<ComputeKernelAvgMisorientationsWorkingSet>(
        k_WorkingSetInputError,
        fmt::format(
            "Compute Kernel Average Misorientations cannot size its working set for Image Geometry '{}' with dimensions ({}) under cache budget {} bytes because kernel radius must contain exactly "
            "3 values (X, Y, Z), but {} values were provided: [{}].",
            imageGeometryPath.toString(), StringUtilities::formatDimensions3D(dimensions), cacheBudgetBytes, kernelSize.size(), fmt::join(kernelSize, ", ")));
  }
  if(std::any_of(kernelSize.cbegin(), kernelSize.cend(), [](int32 radius) { return radius < 0; }))
  {
    return MakeErrorResult<ComputeKernelAvgMisorientationsWorkingSet>(
        k_WorkingSetInputError,
        fmt::format("Compute Kernel Average Misorientations cannot size its working set for Image Geometry '{}' with dimensions ({}) and kernel radius ({}, {}, {}) under cache budget {} bytes "
                    "because all kernel radii must be nonnegative.",
                    imageGeometryPath.toString(), StringUtilities::formatDimensions3D(dimensions), kernelSize[0], kernelSize[1], kernelSize[2], cacheBudgetBytes));
  }

  ComputeKernelAvgMisorientationsWorkingSet plan;

  const uint64 cacheUsed = std::min(cacheUsedBytes, cacheBudgetBytes);
  const uint64 cacheAvailable = cacheBudgetBytes - cacheUsed;
  plan.CapBytes = std::min(cacheBudgetBytes / 4, cacheAvailable / 2);

  const usize xPoints = dimensions[0];
  const usize yPoints = dimensions[1];
  const usize zPoints = dimensions[2];

  if(!CheckedMultiply(xPoints, yPoints, plan.SliceTuples))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the slice tuple count");
  }

  usize totalTuples = 0;
  if(!CheckedMultiply(plan.SliceTuples, zPoints, totalTuples))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the total tuple count");
  }

  usize totalQuaternionValues = 0;
  if(!CheckedMultiply(totalTuples, usize{4}, totalQuaternionValues))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, fmt::format("the total quaternion value count ({} tuples * 4 components)", totalTuples));
  }

  const uint64 kernelZRadius = static_cast<uint64>(kernelSize[2]);
  uint64 doubledKernelZRadius = 0;
  uint64 fullWindowSlices = 0;
  if(!CheckedMultiply(kernelZRadius, uint64{2}, doubledKernelZRadius) || !CheckedAdd(doubledKernelZRadius, uint64{1}, fullWindowSlices) || fullWindowSlices > std::numeric_limits<usize>::max())
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the rolling-window slice count");
  }
  plan.WindowSlices = std::min(zPoints, static_cast<usize>(fullWindowSlices));

  uint64 rollingWindowTuples = 0;
  uint64 rollingInputBytes = 0;
  uint64 focalAndOutputBytes = 0;
  if(!CheckedMultiply(static_cast<uint64>(plan.WindowSlices), static_cast<uint64>(plan.SliceTuples), rollingWindowTuples))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the rolling-window tuple count");
  }
  if(!CheckedMultiply(rollingWindowTuples, k_InputBytesPerTuple, rollingInputBytes))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the rolling-window input byte count");
  }
  if(!CheckedMultiply(static_cast<uint64>(plan.SliceTuples), k_OutputBytesPerTuple, focalAndOutputBytes))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the focal/output byte count");
  }
  if(!CheckedAdd(rollingInputBytes, focalAndOutputBytes, plan.RollingBytes))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the total rolling byte count");
  }

  plan.UseRollingWindow = plan.RollingBytes <= plan.CapBytes;
  if(plan.UseRollingWindow)
  {
    return {std::move(plan)};
  }

  plan.BlockTuples = std::max<usize>(1, std::min(k_TargetBlockTuples, totalTuples));

  uint64 blockWorkingBytes = 0;
  if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_InputBytesPerTuple + k_OutputBytesPerTuple, blockWorkingBytes))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the bounded-block byte count");
  }
  while(plan.BlockTuples > 1 && blockWorkingBytes > plan.CapBytes)
  {
    plan.BlockTuples /= 2;
    if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_InputBytesPerTuple + k_OutputBytesPerTuple, blockWorkingBytes))
    {
      return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the bounded-block byte count");
    }
  }

  if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_OutputBytesPerTuple, focalAndOutputBytes))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the focal/output block byte count");
  }
  uint64 bytesPerInputSlot = 0;
  if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_InputBytesPerTuple, bytesPerInputSlot))
  {
    return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the input-slot byte count");
  }

  const uint64 bytesForInputSlots = plan.CapBytes > focalAndOutputBytes ? plan.CapBytes - focalAndOutputBytes : 0;
  plan.CacheSlots = std::max<usize>(1, static_cast<usize>(bytesForInputSlots / bytesPerInputSlot));

  // Whole-row blocks align to X rows. The planner shrinks such blocks until the
  // cache retains the clamped Z/Y neighbor rows and avoids repeated eviction.
  if(plan.BlockTuples >= xPoints)
  {
    plan.BlockTuples = std::max(xPoints, (plan.BlockTuples / xPoints) * xPoints);

    if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_OutputBytesPerTuple, focalAndOutputBytes))
    {
      return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the focal/output block byte count");
    }
    if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_InputBytesPerTuple, bytesPerInputSlot))
    {
      return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the input-slot byte count");
    }
    const uint64 adjustedBytesForInputSlots = plan.CapBytes > focalAndOutputBytes ? plan.CapBytes - focalAndOutputBytes : 0;
    plan.CacheSlots = std::max<usize>(1, static_cast<usize>(adjustedBytesForInputSlots / bytesPerInputSlot));

    uint64 doubledKernelYRadius = 0;
    uint64 fullWindowRows = 0;
    if(!CheckedMultiply(static_cast<uint64>(kernelSize[1]), uint64{2}, doubledKernelYRadius) || !CheckedAdd(doubledKernelYRadius, uint64{1}, fullWindowRows))
    {
      return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the bounded-cache neighbor-row count");
    }

    const usize windowRows = std::min(yPoints, static_cast<usize>(fullWindowRows));
    usize requiredCacheSlots = 0;
    if(!CheckedMultiply(plan.WindowSlices, windowRows, requiredCacheSlots))
    {
      return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the bounded-cache neighbor-row working set");
    }

    while(plan.BlockTuples > xPoints && plan.CacheSlots < requiredCacheSlots)
    {
      const usize blockRows = plan.BlockTuples / xPoints;
      plan.BlockTuples = std::max<usize>(1, blockRows / 2) * xPoints;

      if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_OutputBytesPerTuple, focalAndOutputBytes))
      {
        return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the focal/output block byte count");
      }
      if(!CheckedMultiply(static_cast<uint64>(plan.BlockTuples), k_InputBytesPerTuple, bytesPerInputSlot))
      {
        return MakeWorkingSetOverflowResult(dimensions, kernelSize, cacheBudgetBytes, imageGeometryPath, "the input-slot byte count");
      }

      const uint64 remainingBytesForInputSlots = plan.CapBytes > focalAndOutputBytes ? plan.CapBytes - focalAndOutputBytes : 0;
      plan.CacheSlots = std::max<usize>(1, static_cast<usize>(remainingBytesForInputSlots / bytesPerInputSlot));
    }
  }

  return {std::move(plan)};
}

namespace
{
constexpr usize k_UnloadedSlice = std::numeric_limits<usize>::max();
constexpr usize k_UnloadedBlock = std::numeric_limits<usize>::max();
constexpr usize k_FullyAssociativeCacheSlotLimit = 16;

/**
 * @struct SliceSlot
 * @brief Stores one loaded Z slice in the rolling window.
 *
 * Z is k_UnloadedSlice until the slot contains matching feature ID, Cell
 * Phase, and quaternion values.
 */
struct SliceSlot
{
  usize Z = k_UnloadedSlice;
  std::vector<int32> FeatureIds;
  std::vector<int32> CellPhases;
  std::vector<float32> Quats;
};

/**
 * @struct InputBlock
 * @brief Stores one cached input range for the fallback traversal.
 *
 * Each slot holds matching feature ID, Cell Phase, and quaternion ranges. The
 * cache replaces the least recently used block in the selected set.
 */
struct InputBlock
{
  usize BlockIndex = k_UnloadedBlock;
  usize TupleStart = 0;
  usize TupleCount = 0;
  uint64 LastUse = 0;
  std::vector<int32> FeatureIds;
  std::vector<int32> CellPhases;
  std::vector<float32> Quats;
};

/**
 * @class InputBlockCache
 * @brief Caches source tuple blocks for the sequential KAM fallback.
 *
 * ExecuteBlockCache() owns this cache on one thread. A cache miss reads all
 * three input stores before the block becomes resident. The cache propagates
 * every bulk-I/O error to stop the traversal.
 */
class InputBlockCache
{
public:
  /**
   * @brief Initializes the bounded source-block cache.
   * @param featureIdsStore Provides source feature IDs.
   * @param cellPhasesStore Provides source Cell Phases.
   * @param quatsStore Provides source quaternions.
   * @param plan Specifies the block size and slot count.
   * @param totalTuples Specifies the number of source tuples.
   * @pre The source stores contain totalTuples values and matching quaternion
   *      components.
   * @pre plan.BlockTuples and plan.CacheSlots are greater than zero.
   */
  InputBlockCache(const AbstractDataStore<int32>& featureIdsStore, const AbstractDataStore<int32>& cellPhasesStore, const AbstractDataStore<float32>& quatsStore,
                  const ComputeKernelAvgMisorientationsWorkingSet& plan, usize totalTuples)
  : m_FeatureIdsStore(featureIdsStore)
  , m_CellPhasesStore(cellPhasesStore)
  , m_QuatsStore(quatsStore)
  , m_BlockTuples(plan.BlockTuples)
  , m_TotalTuples(totalTuples)
  , m_Slots(plan.CacheSlots)
  , m_SetCount(plan.CacheSlots <= k_FullyAssociativeCacheSlotLimit ? 1 : (plan.CacheSlots + 3) / 4)
  , m_WaysPerSet(plan.CacheSlots <= k_FullyAssociativeCacheSlotLimit ? plan.CacheSlots : 4)
  {
    assert(m_BlockTuples > 0);
    assert(!m_Slots.empty());
    assert(m_SetCount > 0);

    for(InputBlock& slot : m_Slots)
    {
      slot.FeatureIds.resize(m_BlockTuples);
      slot.CellPhases.resize(m_BlockTuples);
      slot.Quats.resize(m_BlockTuples * 4);
    }
  }

  /**
   * @brief Returns the cached block that contains one tuple.
   * @param tupleIndex Identifies a source tuple.
   * @pre tupleIndex is less than the source tuple count.
   * @return The resident block, or a source bulk-I/O error.
   *
   * A later cache miss can replace the returned block.
   */
  Result<const InputBlock*> getBlock(usize tupleIndex)
  {
    assert(tupleIndex < m_TotalTuples);
    const usize blockIndex = tupleIndex / m_BlockTuples;
    const usize setIndex = blockIndex % m_SetCount;
    const usize firstWay = setIndex * 4;
    // The final set can have fewer ways because the plan allocates only
    // CacheSlots entries.
    const usize endWay = std::min(firstWay + m_WaysPerSet, m_Slots.size());

    InputBlock* replacement = nullptr;
    for(usize way = firstWay; way < endWay; way++)
    {
      InputBlock& slot = m_Slots[way];
      if(slot.BlockIndex == blockIndex)
      {
        slot.LastUse = m_NextUse++;
        return {&slot};
      }

      if(slot.BlockIndex == k_UnloadedBlock)
      {
        replacement = &slot;
        break;
      }
      if(replacement == nullptr || slot.LastUse < replacement->LastUse)
      {
        replacement = &slot;
      }
    }

    assert(replacement != nullptr);
    const usize tupleStart = blockIndex * m_BlockTuples;
    const usize tupleCount = std::min(m_BlockTuples, m_TotalTuples - tupleStart);
    if(Result<> result = m_FeatureIdsStore.copyIntoBuffer(tupleStart, nonstd::span<int32>(replacement->FeatureIds.data(), tupleCount)); result.invalid())
    {
      return ConvertInvalidResult<const InputBlock*>(std::move(result));
    }
    if(Result<> result = m_CellPhasesStore.copyIntoBuffer(tupleStart, nonstd::span<int32>(replacement->CellPhases.data(), tupleCount)); result.invalid())
    {
      return ConvertInvalidResult<const InputBlock*>(std::move(result));
    }
    if(Result<> result = m_QuatsStore.copyIntoBuffer(tupleStart * 4, nonstd::span<float32>(replacement->Quats.data(), tupleCount * 4)); result.invalid())
    {
      return ConvertInvalidResult<const InputBlock*>(std::move(result));
    }

    replacement->BlockIndex = blockIndex;
    replacement->TupleStart = tupleStart;
    replacement->TupleCount = tupleCount;
    replacement->LastUse = m_NextUse++;
    return {replacement};
  }

private:
  const AbstractDataStore<int32>& m_FeatureIdsStore;
  const AbstractDataStore<int32>& m_CellPhasesStore;
  const AbstractDataStore<float32>& m_QuatsStore;
  usize m_BlockTuples = 0;
  usize m_TotalTuples = 0;
  std::vector<InputBlock> m_Slots;
  usize m_SetCount = 0;
  usize m_WaysPerSet = 0;
  uint64 m_NextUse = 1;
};

/**
 * @class RollingPlaneWorker
 * @brief Calculates one KAM plane from local rolling-window buffers.
 *
 * ExecuteRollingWindow() fills every slice before it starts the parallel
 * algorithm. Each worker range reads immutable local buffers and writes a
 * disjoint part of the local output slice. The worker does not access a
 * DataStore.
 */
class RollingPlaneWorker
{
public:
  /**
   * @brief Initializes a rolling-window plane worker.
   * @param xPoints Specifies the X dimension.
   * @param yPoints Specifies the Y dimension.
   * @param zPoints Specifies the Z dimension.
   * @param plane Identifies the focal Z plane.
   * @param kernelX Specifies the X kernel radius.
   * @param kernelY Specifies the Y kernel radius.
   * @param kernelZ Specifies the Z kernel radius.
   * @param sliceTuples Specifies the number of tuples in one slice.
   * @param sliceSlots Provides the loaded rolling-window slices.
   * @param windowSlices Specifies the number of rolling-window slices.
   * @param useFeatureIds Selects same-feature neighbor admission.
   * @param crystalStructures Provides local crystal structures.
   * @param output Receives local KAM values.
   * @pre sliceSlots contains windowSlices loaded slices.
   * @pre output contains sliceTuples elements.
   */
  RollingPlaneWorker(usize xPoints, usize yPoints, usize zPoints, usize plane, int32 kernelX, int32 kernelY, int32 kernelZ, usize sliceTuples, const std::vector<SliceSlot>& sliceSlots,
                     usize windowSlices, bool useFeatureIds, nonstd::span<const uint32> crystalStructures, nonstd::span<float32> output)
  : m_XPoints(xPoints)
  , m_YPoints(yPoints)
  , m_ZPoints(zPoints)
  , m_Plane(plane)
  , m_KernelX(kernelX)
  , m_KernelY(kernelY)
  , m_KernelZ(kernelZ)
  , m_SliceTuples(sliceTuples)
  , m_SliceSlots(sliceSlots)
  , m_WindowSlices(windowSlices)
  , m_UseFeatureIds(useFeatureIds)
  , m_CrystalStructures(crystalStructures)
  , m_Output(output)
  {
  }

  /**
   * @brief Calculates a half-open local output range.
   * @param range Identifies the local row and column range.
   */
  void operator()(const Range2D& range) const
  {
    compute(range.minRow(), range.maxRow(), range.minCol(), range.maxCol());
  }

#ifdef SIMPLNX_ENABLE_MULTICORE
  /**
   * @brief Calculates a TBB local output range.
   * @param range Identifies the local TBB row and column range.
   */
  void operator()(const tbb::blocked_range2d<size_t, size_t>& range) const
  {
    compute(range.rows().begin(), range.rows().end(), range.cols().begin(), range.cols().end());
  }
#endif

private:
  void compute(usize minRow, usize maxRow, usize minCol, usize maxCol) const
  {
    assert(m_XPoints > 0);
    assert(m_YPoints > 0);
    assert(m_ZPoints > 0);
    assert(m_Plane < m_ZPoints);
    assert(m_KernelX >= 0);
    assert(m_KernelY >= 0);
    assert(m_KernelZ >= 0);
    assert(m_Output.size() == m_SliceTuples);
    assert(m_WindowSlices > 0);
    assert(m_SliceSlots.size() == m_WindowSlices);

    // Each concurrent invocation creates orientation operations for its
    // disjoint local row and column range.
    std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

    const usize kernelX = static_cast<usize>(m_KernelX);
    const usize kernelY = static_cast<usize>(m_KernelY);
    const usize kernelZ = static_cast<usize>(m_KernelZ);
    const usize windowZMin = m_Plane - std::min(m_Plane, kernelZ);
    const usize windowZMax = m_Plane + std::min(kernelZ, (m_ZPoints - 1) - m_Plane);

    const SliceSlot& focalSlice = m_SliceSlots[m_Plane % m_WindowSlices];
    assert(focalSlice.Z == m_Plane);

    for(usize row = minRow; row < maxRow; row++)
    {
      const usize neighborYMin = row - std::min(row, kernelY);
      const usize neighborYMax = row + std::min(kernelY, (m_YPoints - 1) - row);

      for(usize col = minCol; col < maxCol; col++)
      {
        const usize pointInSlice = row * m_XPoints + col;
        const int32 featureId = focalSlice.FeatureIds[pointInSlice];
        const int32 cellPhase = focalSlice.CellPhases[pointInSlice];

        if(featureId <= 0 || cellPhase <= 0)
        {
          m_Output[pointInSlice] = 0.0F;
          continue;
        }

        ebsdlib::QuatD q1;
        const usize q1Idx = pointInSlice * 4;
        q1[0] = focalSlice.Quats[q1Idx];
        q1[1] = focalSlice.Quats[q1Idx + 1];
        q1[2] = focalSlice.Quats[q1Idx + 2];
        q1[3] = focalSlice.Quats[q1Idx + 3];

        const uint32 laueClass = m_CrystalStructures[static_cast<usize>(cellPhase)];
        float32 totalMisorientation = 0.0F;
        usize numVoxel = 0;

        const usize neighborXMin = col - std::min(col, kernelX);
        const usize neighborXMax = col + std::min(kernelX, (m_XPoints - 1) - col);

        // Ascending Z/Y/X traversal preserves the established floating-point
        // accumulation order for each focal cell.
        for(usize nz = windowZMin;; nz++)
        {
          const SliceSlot& neighborSlice = m_SliceSlots[nz % m_WindowSlices];
          assert(neighborSlice.Z == nz);

          for(usize ny = neighborYMin;; ny++)
          {
            for(usize nx = neighborXMin;; nx++)
            {
              const usize neighborInSlice = ny * m_XPoints + nx;
              const int32 neighborFeatureId = neighborSlice.FeatureIds[neighborInSlice];
              const bool neighborContributes = m_UseFeatureIds ? (neighborFeatureId == featureId) : (neighborFeatureId > 0 && neighborSlice.CellPhases[neighborInSlice] == cellPhase);
              if(neighborContributes)
              {
                const usize q2Idx = neighborInSlice * 4;
                ebsdlib::QuatD q2;
                q2[0] = neighborSlice.Quats[q2Idx];
                q2[1] = neighborSlice.Quats[q2Idx + 1];
                q2[2] = neighborSlice.Quats[q2Idx + 2];
                q2[3] = neighborSlice.Quats[q2Idx + 3];

                ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(q1, q2);
                totalMisorientation += (axisAngle[3] * nx::core::Constants::k_180OverPiF);
                numVoxel++;
              }

              if(nx == neighborXMax)
              {
                break;
              }
            }

            if(ny == neighborYMax)
            {
              break;
            }
          }

          if(nz == windowZMax)
          {
            break;
          }
        }

        m_Output[pointInSlice] = (numVoxel > 0) ? (totalMisorientation / static_cast<float32>(numVoxel)) : 0.0F;
      }
    }
  }

  usize m_XPoints = 0;
  usize m_YPoints = 0;
  usize m_ZPoints = 0;
  usize m_Plane = 0;
  int32 m_KernelX = 0;
  int32 m_KernelY = 0;
  int32 m_KernelZ = 0;
  usize m_SliceTuples = 0;
  const std::vector<SliceSlot>& m_SliceSlots;
  usize m_WindowSlices = 0;
  bool m_UseFeatureIds = true;
  nonstd::span<const uint32> m_CrystalStructures;
  nonstd::span<float32> m_Output;
};

/**
 * @brief Executes KAM with a local rolling Z window.
 * @param dataStructure Provides the selected arrays and Image Geometry.
 * @param inputValues Identifies the selected arrays and KAM settings.
 * @param plan Specifies the rolling-window buffer sizes.
 * @param shouldCancel Signals cancellation.
 * @pre plan.UseRollingWindow is true.
 * @return Success, or a crystal-structure, input, or output bulk-I/O error.
 *
 * The function reads source slices before parallel workers access local
 * buffers. It writes each output slice after the parallel work completes.
 * Cancellation is checked before each plane and returns success.
 */
Result<> ExecuteRollingWindow(DataStructure& dataStructure, const ComputeKernelAvgMisorientationsInputValues& inputValues, const ComputeKernelAvgMisorientationsWorkingSet& plan,
                              const std::atomic_bool& shouldCancel)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues.InputImageGeometry);
  const SizeVec3 dimensions = imageGeom.getDimensions();
  const usize xPoints = dimensions[0];
  const usize yPoints = dimensions[1];
  const usize zPoints = dimensions[2];

  // An empty Image Geometry produces no output. Return before DataStore access,
  // orientation-operation creation, or rolling-window allocation.
  if(xPoints == 0 || yPoints == 0 || zPoints == 0)
  {
    return {};
  }

  const VectorInt32Parameter::ValueType kernelSize = inputValues.KernelSize;
  const usize kZ = static_cast<usize>(kernelSize[2]);

  const auto& cellPhasesStore = dataStructure.getDataRefAs<Int32Array>(inputValues.CellPhasesArrayPath).getDataStoreRef();
  const auto& featureIdsStore = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureIdsArrayPath).getDataStoreRef();
  const auto& quatsStore = dataStructure.getDataRefAs<Float32Array>(inputValues.QuatsArrayPath).getDataStoreRef();
  auto& outputStore = dataStructure.getDataRefAs<Float32Array>(inputValues.KernelAverageMisorientationsArrayName).getDataStoreRef();

  const auto& crystalStructuresStore = dataStructure.getDataRefAs<UInt32Array>(inputValues.CrystalStructuresArrayPath).getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStore.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numCrystalStructures);
  if(Result<> result = crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size())); result.invalid())
  {
    return result;
  }

  const usize quaternionSliceValues = plan.SliceTuples * 4;
  std::vector<SliceSlot> sliceSlots(plan.WindowSlices);
  for(SliceSlot& slot : sliceSlots)
  {
    slot.FeatureIds.resize(plan.SliceTuples);
    slot.CellPhases.resize(plan.SliceTuples);
    slot.Quats.resize(quaternionSliceValues);
  }
  std::vector<float32> outputSlice(plan.SliceTuples);

  auto loadSlice = [&](usize z, SliceSlot& slot) -> Result<> {
    const usize offset = z * plan.SliceTuples;
    if(Result<> result = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(slot.FeatureIds.data(), plan.SliceTuples)); result.invalid())
    {
      return result;
    }
    if(Result<> result = cellPhasesStore.copyIntoBuffer(offset, nonstd::span<int32>(slot.CellPhases.data(), plan.SliceTuples)); result.invalid())
    {
      return result;
    }
    if(Result<> result = quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(slot.Quats.data(), quaternionSliceValues)); result.invalid())
    {
      return result;
    }
    slot.Z = z;
    return {};
  };

  usize nextSliceToLoad = 0;
  for(usize plane = 0; plane < zPoints; plane++)
  {
    if(shouldCancel)
    {
      return {};
    }

    assert(plan.WindowSlices > 0);
    const usize windowZMin = plane - std::min(plane, kZ);
    const usize windowZMax = plane + std::min(kZ, (zPoints - 1) - plane);

    // Monotonic Z loads reuse only slots below the clamped window. The ring
    // holds 2*kZ+1 slices or the entire volume when it is shorter.
    while(nextSliceToLoad <= windowZMax)
    {
      SliceSlot& slot = sliceSlots[nextSliceToLoad % plan.WindowSlices];
      assert(slot.Z == k_UnloadedSlice || slot.Z < windowZMin);
      if(Result<> result = loadSlice(nextSliceToLoad, slot); result.invalid())
      {
        return result;
      }
      nextSliceToLoad++;
    }

    ParallelData2DAlgorithm algorithm;
    algorithm.setRange(0, xPoints, 0, yPoints);
    const RollingPlaneWorker worker(xPoints, yPoints, zPoints, plane, kernelSize[0], kernelSize[1], kernelSize[2], plan.SliceTuples, sliceSlots, plan.WindowSlices, inputValues.UseFeatureIds,
                                    nonstd::span<const uint32>(crystalStructures.data(), crystalStructures.size()), nonstd::span<float32>(outputSlice.data(), outputSlice.size()));
    algorithm.execute(worker);

    const usize planeOffset = plane * plan.SliceTuples;
    if(Result<> result = outputStore.copyFromBuffer(planeOffset, nonstd::span<const float32>(outputSlice.data(), plan.SliceTuples)); result.invalid())
    {
      return result;
    }
  }

  return {};
}

/**
 * @brief Executes KAM with a bounded source-block cache.
 * @param dataStructure Provides the selected arrays and Image Geometry.
 * @param inputValues Identifies the selected arrays and KAM settings.
 * @param plan Specifies the fallback block and cache sizes.
 * @param shouldCancel Signals cancellation.
 * @pre plan.UseRollingWindow is false.
 * @return Success, or a crystal-structure, input, or output bulk-I/O error.
 *
 * One thread owns the cache and output block. Cancellation is checked before
 * each plane and focal block and returns success.
 */
Result<> ExecuteBlockCache(DataStructure& dataStructure, const ComputeKernelAvgMisorientationsInputValues& inputValues, const ComputeKernelAvgMisorientationsWorkingSet& plan,
                           const std::atomic_bool& shouldCancel)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues.InputImageGeometry);
  const SizeVec3 dimensions = imageGeom.getDimensions();
  const usize xPoints = dimensions[0];
  const usize yPoints = dimensions[1];
  const usize zPoints = dimensions[2];

  // An empty Image Geometry produces no output. Return before DataStore access,
  // orientation-operation creation, or bounded-cache allocation.
  if(xPoints == 0 || yPoints == 0 || zPoints == 0)
  {
    return {};
  }

  const VectorInt32Parameter::ValueType kernelSize = inputValues.KernelSize;
  const usize kZ = static_cast<usize>(kernelSize[2]);
  const usize kY = static_cast<usize>(kernelSize[1]);
  const usize kX = static_cast<usize>(kernelSize[0]);

  const auto& cellPhasesStore = dataStructure.getDataRefAs<Int32Array>(inputValues.CellPhasesArrayPath).getDataStoreRef();
  const auto& featureIdsStore = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureIdsArrayPath).getDataStoreRef();
  const auto& quatsStore = dataStructure.getDataRefAs<Float32Array>(inputValues.QuatsArrayPath).getDataStoreRef();
  auto& outputStore = dataStructure.getDataRefAs<Float32Array>(inputValues.KernelAverageMisorientationsArrayName).getDataStoreRef();

  const auto& crystalStructuresStore = dataStructure.getDataRefAs<UInt32Array>(inputValues.CrystalStructuresArrayPath).getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStore.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numCrystalStructures);
  if(Result<> result = crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size())); result.invalid())
  {
    return result;
  }

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const usize totalTuples = plan.SliceTuples * zPoints;
  InputBlockCache inputCache(featureIdsStore, cellPhasesStore, quatsStore, plan, totalTuples);
  std::vector<float32> outputBlock(plan.BlockTuples);

  for(usize plane = 0; plane < zPoints; plane++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize planeOffset = plane * plan.SliceTuples;
    const usize windowZMin = plane - std::min(plane, kZ);
    const usize windowZMax = plane + std::min(kZ, (zPoints - 1) - plane);

    for(usize pointInSliceStart = 0; pointInSliceStart < plan.SliceTuples;)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize focalCount = std::min(plan.BlockTuples, plan.SliceTuples - pointInSliceStart);
      const usize focalTupleStart = planeOffset + pointInSliceStart;

      const InputBlock* focalInputBlock = nullptr;
      for(usize focalOffset = 0; focalOffset < focalCount; focalOffset++)
      {
        const usize focalTuple = focalTupleStart + focalOffset;
        if(focalInputBlock == nullptr || focalTuple < focalInputBlock->TupleStart || focalTuple >= focalInputBlock->TupleStart + focalInputBlock->TupleCount)
        {
          auto blockResult = inputCache.getBlock(focalTuple);
          if(blockResult.invalid())
          {
            return ConvertResult(std::move(blockResult));
          }
          focalInputBlock = blockResult.value();
        }

        const usize localFocalTuple = focalTuple - focalInputBlock->TupleStart;
        const int32 featureId = focalInputBlock->FeatureIds[localFocalTuple];
        const int32 cellPhase = focalInputBlock->CellPhases[localFocalTuple];

        if(featureId <= 0 || cellPhase <= 0)
        {
          outputBlock[focalOffset] = 0.0F;
          continue;
        }

        ebsdlib::QuatD q1;
        const usize q1Idx = localFocalTuple * 4;
        q1[0] = focalInputBlock->Quats[q1Idx];
        q1[1] = focalInputBlock->Quats[q1Idx + 1];
        q1[2] = focalInputBlock->Quats[q1Idx + 2];
        q1[3] = focalInputBlock->Quats[q1Idx + 3];

        const uint32 laueClass = crystalStructures[static_cast<usize>(cellPhase)];
        float32 totalMisorientation = 0.0F;
        usize numVoxel = 0;

        const usize pointInSlice = pointInSliceStart + focalOffset;
        const usize row = pointInSlice / xPoints;
        const usize col = pointInSlice % xPoints;
        const usize neighborYMin = row - std::min(row, kY);
        const usize neighborYMax = row + std::min(kY, (yPoints - 1) - row);
        const usize neighborXMin = col - std::min(col, kX);
        const usize neighborXMax = col + std::min(kX, (xPoints - 1) - col);
        const usize neighborXEnd = neighborXMax + 1;

        for(usize nz = windowZMin;; nz++)
        {
          for(usize ny = neighborYMin;; ny++)
          {
            const usize neighborRowStart = nz * plan.SliceTuples + ny * xPoints;
            usize segmentX = neighborXMin;
            while(segmentX < neighborXEnd)
            {
              const usize segmentTuple = neighborRowStart + segmentX;
              auto blockResult = inputCache.getBlock(segmentTuple);
              if(blockResult.invalid())
              {
                return ConvertResult(std::move(blockResult));
              }
              const InputBlock& neighborBlock = *blockResult.value();
              const usize localSegmentStart = segmentTuple - neighborBlock.TupleStart;
              const usize segmentCount = std::min(neighborXEnd - segmentX, neighborBlock.TupleCount - localSegmentStart);

              for(usize segmentOffset = 0; segmentOffset < segmentCount; segmentOffset++)
              {
                const usize localNeighborTuple = localSegmentStart + segmentOffset;
                const int32 neighborFeatureId = neighborBlock.FeatureIds[localNeighborTuple];
                const bool neighborContributes = inputValues.UseFeatureIds ? (neighborFeatureId == featureId) : (neighborFeatureId > 0 && neighborBlock.CellPhases[localNeighborTuple] == cellPhase);
                if(neighborContributes)
                {
                  const usize q2Idx = localNeighborTuple * 4;
                  ebsdlib::QuatD q2;
                  q2[0] = neighborBlock.Quats[q2Idx];
                  q2[1] = neighborBlock.Quats[q2Idx + 1];
                  q2[2] = neighborBlock.Quats[q2Idx + 2];
                  q2[3] = neighborBlock.Quats[q2Idx + 3];

                  ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(q1, q2);
                  totalMisorientation += (axisAngle[3] * nx::core::Constants::k_180OverPiF);
                  numVoxel++;
                }
              }

              segmentX += segmentCount;
            }

            if(ny == neighborYMax)
            {
              break;
            }
          }

          if(nz == windowZMax)
          {
            break;
          }
        }

        outputBlock[focalOffset] = (numVoxel > 0) ? (totalMisorientation / static_cast<float32>(numVoxel)) : 0.0F;
      }

      if(Result<> result = outputStore.copyFromBuffer(focalTupleStart, nonstd::span<const float32>(outputBlock.data(), focalCount)); result.invalid())
      {
        return result;
      }
      pointInSliceStart += focalCount;
    }
  }

  return {};
}
} // namespace
} // namespace nx::core

ComputeKernelAvgMisorientationsScanline::ComputeKernelAvgMisorientationsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                 const ComputeKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeKernelAvgMisorientationsScanline::~ComputeKernelAvgMisorientationsScanline() noexcept = default;

Result<> ComputeKernelAvgMisorientationsScanline::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  const SizeVec3 dimensions = imageGeom.getDimensions();
  if(dimensions[0] == 0 || dimensions[1] == 0 || dimensions[2] == 0)
  {
    return {};
  }

  auto& cacheMemoryBudgetManager = CacheMemoryBudgetManager::instance();
  const uint64 cacheBudgetBytes = cacheMemoryBudgetManager.budgetBytes();
  const uint64 cacheUsedBytes = cacheMemoryBudgetManager.usedBytes();
  auto planResult = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, m_InputValues->KernelSize, cacheBudgetBytes, cacheUsedBytes, m_InputValues->InputImageGeometry);
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }

  const auto& plan = planResult.value();
  if(plan.UseRollingWindow)
  {
    return ExecuteRollingWindow(m_DataStructure, *m_InputValues, plan, m_ShouldCancel);
  }

  return ExecuteBlockCache(m_DataStructure, *m_InputValues, plan, m_ShouldCancel);
}
