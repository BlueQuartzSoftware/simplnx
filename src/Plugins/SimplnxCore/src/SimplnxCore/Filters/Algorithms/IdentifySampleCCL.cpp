/**
 * @file IdentifySampleCCL.cpp
 * @brief Implements bounded scanline connected-component labeling (CCL) for sample identification.
 *
 * Breadth-first search (BFS) has random neighbor access that can repeatedly load and evict disk-backed
 * chunks. CCL scans Z-Y-X slices sequentially. A replay repeats deterministic
 * label assignment instead of retaining one label for every voxel. The replay
 * costs an extra sequential read but keeps label memory proportional to two slices.
 */

#include "IdentifySampleCCL.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ExternalEquivalence.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <limits>
#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
/**
 * @struct CCLResult
 * @brief Owns one forward CCL pass's equivalence state and summary.
 *
 * Replay re-derives provisional labels instead of storing one label per voxel.
 */
struct CCLResult
{
  std::unique_ptr<ExternalEquivalence> equivalences; // Owns bounded external label relationships.
  uint64 nextLabel = 1;                              // First unused provisional label.
  uint64 largestRoot = 0;                            // Root label of the largest component.
  uint64 largestSize = 0;                            // Voxel count of the largest component.
};

constexpr uint64 k_RecordsPerPage = 4096; // Bounds records read or written in one cache page.
constexpr usize k_MaxCachedPages = 16;    // Bounds resident temporary-record cache pages.

/**
 * @brief Creates fixed-record scratch through the registered provider, with an explicitly permitted resident fallback.
 * @param recordSize Bytes in one record.
 * @param recordCount Initial record count.
 * @param allowInMemoryFallback True to permit resident scratch.
 * @return Temporary record store or a provider error.
 *
 * Disk-backed callers set allowInMemoryFallback false. This prevents a missing
 * provider from silently allocating cell-count scratch in RAM.
 */
Result<std::unique_ptr<ITemporaryRecordStore>> CreateTemporaryRecordStore(uint64 recordSize, uint64 recordCount, bool allowInMemoryFallback)
{
  TemporaryRecordStoreConfig config;
  config.recordSize = recordSize;
  config.maxRecordsPerBatch = k_RecordsPerPage;
  config.initialRecordCount = recordCount;
  auto result = DataStoreUtilities::GetIOCollection().createTemporaryRecordStore(config);
  if(result.invalid() && allowInMemoryFallback)
  {
    auto fallbackResult = InMemoryTemporaryRecordStore::Create(config);
    if(fallbackResult.invalid())
    {
      return ConvertInvalidResult<std::unique_ptr<ITemporaryRecordStore>>(std::move(fallbackResult));
    }
    result = {std::move(fallbackResult.value())};
  }
  if(result.valid() && result.value() == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-45460, "IdentifySample temporary-record provider returned a null store.");
  }
  return result;
}

/**
 * @brief Creates external equivalence nodes for provisional labels.
 * @param maximumLabel Largest possible provisional label.
 * @param allowInMemoryFallback True to permit resident scratch.
 * @return External equivalence table or a record-store error.
 */
Result<std::unique_ptr<ExternalEquivalence>> CreateEquivalences(uint64 maximumLabel, bool allowInMemoryFallback)
{
  if(maximumLabel == std::numeric_limits<uint64>::max())
  {
    return MakeErrorResult<std::unique_ptr<ExternalEquivalence>>(-45461, "IdentifySample cannot create external equivalences for this image size.");
  }
  auto storeResult = CreateTemporaryRecordStore(sizeof(ExternalEquivalence::Node), maximumLabel + 1, allowInMemoryFallback);
  if(storeResult.invalid())
  {
    return ConvertInvalidResult<std::unique_ptr<ExternalEquivalence>>(std::move(storeResult));
  }
  return ExternalEquivalence::Create(std::move(storeResult.value()), k_RecordsPerPage, k_MaxCachedPages, 0);
}

/**
 * @class ExternalRootFlags
 * @brief Stores boundary-touch flags for component roots.
 *
 * Hole filling reads these flags during replay. The cache avoids a root-count
 * resident vector when a volume has many components.
 */
class ExternalRootFlags
{
public:
  /**
   * @brief Creates zero-initialized root flags and a bounded cache.
   * @param maximumLabel Largest possible root label.
   * @param allowInMemoryFallback True to permit resident scratch.
   * @return Root flags or a record-store or allocation error.
   */
  static Result<std::unique_ptr<ExternalRootFlags>> Create(uint64 maximumLabel, bool allowInMemoryFallback)
  {
    auto storeResult = CreateTemporaryRecordStore(sizeof(uint8), maximumLabel + 1, allowInMemoryFallback);
    if(storeResult.invalid())
    {
      return ConvertInvalidResult<std::unique_ptr<ExternalRootFlags>>(std::move(storeResult));
    }
    try
    {
      return {std::unique_ptr<ExternalRootFlags>(new ExternalRootFlags(std::move(storeResult.value())))};
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<std::unique_ptr<ExternalRootFlags>>(-45462, "IdentifySample could not allocate bounded boundary-root cache.");
    }
  }

  /**
   * @brief Marks one root as boundary connected.
   * @param label Resolved root label.
   * @param shouldCancel Cancellation flag.
   * @return Cache write or cancellation result.
   */
  Result<> mark(uint64 label, const std::atomic_bool& shouldCancel)
  {
    return m_Cache.write(label, uint8{1}, shouldCancel);
  }

  /**
   * @brief Tests one root through the bounded page cache.
   * @param label Resolved root label.
   * @param shouldCancel Cancellation flag.
   * @return True when label is boundary connected, or a cache result.
   */
  Result<bool> isMarked(uint64 label, const std::atomic_bool& shouldCancel)
  {
    auto result = m_Cache.read(label, shouldCancel);
    if(result.invalid())
    {
      return ConvertInvalidResult<bool>(std::move(result));
    }
    return {result.value() != 0};
  }

  /**
   * @brief Flushes dirty root-flag pages before replay reads them.
   * @param shouldCancel Cancellation flag.
   * @return Cache flush or cancellation result.
   */
  Result<> flush(const std::atomic_bool& shouldCancel)
  {
    return m_Cache.flush(shouldCancel);
  }

private:
  /**
   * @brief Takes ownership of a validated root-flag store.
   * @param store Root-flag record store.
   */
  explicit ExternalRootFlags(std::unique_ptr<ITemporaryRecordStore> store)
  : m_Store(std::move(store))
  , m_Cache(*m_Store, k_RecordsPerPage, k_MaxCachedPages)
  {
  }

  std::unique_ptr<ITemporaryRecordStore> m_Store;
  BoundedRecordPageCache<uint8> m_Cache;
};

/**
 * @brief Labels a 3D volume in Z/Y/X order with two resident label slices and external equivalences.
 * @tparam T Mask value type.
 * @tparam ConditionFn Callable that selects a voxel in the current slice.
 * @param store Mask store.
 * @param dimX Image X dimension.
 * @param dimY Image Y dimension.
 * @param dimZ Image Z dimension.
 * @param condition Selects voxels to label.
 * @param shouldCancel Cancellation flag.
 * @return CCL state, partial state on cancellation, or bulk-I/O or equivalence error.
 *
 * Each voxel compares only x-1, y-1, and z-1. Two rolling label slices are
 * enough for this traversal and avoid a volume-sized label array.
 * The pass flushes equivalence pages before a replay resolves labels.
 */
template <typename T, typename ConditionFn>
Result<CCLResult> runForwardCCL(AbstractDataStore<T>& store, int64 dimX, int64 dimY, int64 dimZ, ConditionFn condition, const std::atomic_bool& shouldCancel)
{
  CCLResult result;
  if(shouldCancel)
  {
    return {std::move(result)};
  }
  const usize sliceSize = static_cast<usize>(dimX * dimY);
  const uint64 totalPoints = static_cast<uint64>(dimX) * static_cast<uint64>(dimY) * static_cast<uint64>(dimZ);
  const bool allowInMemoryFallback = store.getStoreType() != IDataStore::StoreType::OutOfCore;
  auto equivalencesResult = CreateEquivalences(totalPoints, allowInMemoryFallback);
  if(equivalencesResult.invalid())
  {
    return ConvertInvalidResult<CCLResult>(std::move(equivalencesResult));
  }
  result.equivalences = std::move(equivalencesResult.value());

  // Two rolling slices cover the only backward neighbors needed by Z-Y-X traversal.
  std::vector<int64> labelBuffer(2 * sliceSize, 0);
  auto sliceData = std::make_unique<T[]>(sliceSize);

  for(int64 z = 0; z < dimZ; z++)
  {
    if(shouldCancel)
    {
      return {std::move(result)};
    }
    auto readResult = store.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<T>(sliceData.get(), sliceSize));
    if(readResult.invalid())
    {
      return ConvertInvalidResult<CCLResult>(std::move(readResult));
    }

    const usize curOff = (static_cast<usize>(z) % 2) * sliceSize;
    std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
    const usize prevOff = ((static_cast<usize>(z) + 1) % 2) * sliceSize;

    for(int64 y = 0; y < dimY; y++)
    {
      for(int64 x = 0; x < dimX; x++)
      {
        const usize inSlice = static_cast<usize>(y) * static_cast<usize>(dimX) + static_cast<usize>(x);

        if(!condition(sliceData.get(), inSlice))
        {
          continue;
        }

        int64 nbrA = 0, nbrB = 0, nbrC = 0;

        if(x > 0)
        {
          nbrA = labelBuffer[curOff + inSlice - 1];
        }
        if(y > 0)
        {
          nbrB = labelBuffer[curOff + inSlice - static_cast<usize>(dimX)];
        }
        if(z > 0)
        {
          nbrC = labelBuffer[prevOff + inSlice];
        }

        int64 minLabel = 0;
        if(nbrA > 0)
        {
          minLabel = nbrA;
        }
        if(nbrB > 0 && (minLabel == 0 || nbrB < minLabel))
        {
          minLabel = nbrB;
        }
        if(nbrC > 0 && (minLabel == 0 || nbrC < minLabel))
        {
          minLabel = nbrC;
        }

        int64 assignedLabel = 0;
        if(minLabel == 0)
        {
          assignedLabel = result.nextLabel++;
        }
        else
        {
          assignedLabel = minLabel;
          if(nbrA > 0 && nbrA != assignedLabel)
          {
            auto uniteResult = result.equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(nbrA), shouldCancel);
            if(uniteResult.invalid())
            {
              return ConvertInvalidResult<CCLResult>(std::move(uniteResult));
            }
          }
          if(nbrB > 0 && nbrB != assignedLabel)
          {
            auto uniteResult = result.equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(nbrB), shouldCancel);
            if(uniteResult.invalid())
            {
              return ConvertInvalidResult<CCLResult>(std::move(uniteResult));
            }
          }
          if(nbrC > 0 && nbrC != assignedLabel)
          {
            auto uniteResult = result.equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(nbrC), shouldCancel);
            if(uniteResult.invalid())
            {
              return ConvertInvalidResult<CCLResult>(std::move(uniteResult));
            }
          }
        }

        labelBuffer[curOff + inSlice] = assignedLabel;
        auto sizeResult = result.equivalences->addSize(static_cast<uint64>(assignedLabel), 1, shouldCancel);
        if(sizeResult.invalid())
        {
          return ConvertInvalidResult<CCLResult>(std::move(sizeResult));
        }
      }
    }
  }

  // Ascending root scans make an equal-size tie select the largest label.
  for(uint64 label = 1; label < result.nextLabel; label++)
  {
    auto rootResult = result.equivalences->find(label, shouldCancel);
    if(rootResult.invalid())
    {
      return ConvertInvalidResult<CCLResult>(std::move(rootResult));
    }
    if(rootResult.value() == label)
    {
      auto sizeResult = result.equivalences->componentSize(label, shouldCancel);
      if(sizeResult.invalid())
      {
        return ConvertInvalidResult<CCLResult>(std::move(sizeResult));
      }
      if(sizeResult.value() >= result.largestSize)
      {
        result.largestSize = sizeResult.value();
        result.largestRoot = label;
      }
    }
  }

  auto flushResult = result.equivalences->flush(shouldCancel);
  if(flushResult.invalid())
  {
    return ConvertInvalidResult<CCLResult>(std::move(flushResult));
  }
  return {std::move(result)};
}

/**
 * @brief Re-derives the first pass's provisional labels and applies an action using resolved roots.
 * @tparam T Mask value type.
 * @tparam ConditionFn Callable that selects a voxel in the current slice.
 * @tparam ActionFn Callable that processes one resolved voxel label.
 * @param store Mask store.
 * @param dimX Image X dimension.
 * @param dimY Image Y dimension.
 * @param dimZ Image Z dimension.
 * @param equivalences Provisional-label equivalences from the forward scan.
 * @param condition Selects voxels to label.
 * @param action Processes a resolved root and reports slice modification.
 * @param shouldCancel Cancellation flag.
 * @return Bulk-I/O, equivalence, action, or cancellation result.
 *
 * The identical traversal re-derives the same labels without another unite pass.
 * A modified slice writes back once after all its voxel actions complete.
 */
template <typename T, typename ConditionFn, typename ActionFn>
Result<> replayForwardCCL(AbstractDataStore<T>& store, int64 dimX, int64 dimY, int64 dimZ, ExternalEquivalence& equivalences, ConditionFn condition, ActionFn action,
                          const std::atomic_bool& shouldCancel)
{
  const usize sliceSize = static_cast<usize>(dimX * dimY);
  auto sliceData = std::make_unique<T[]>(sliceSize);
  std::vector<int64> labelBuffer(2 * sliceSize, 0);
  int64 nextLabel = 1;

  for(int64 z = 0; z < dimZ; z++)
  {
    if(shouldCancel)
    {
      return {};
    }
    auto readResult = store.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<T>(sliceData.get(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }
    bool modified = false;

    const usize curOff = (static_cast<usize>(z) % 2) * sliceSize;
    std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
    const usize prevOff = ((static_cast<usize>(z) + 1) % 2) * sliceSize;

    for(int64 y = 0; y < dimY; y++)
    {
      for(int64 x = 0; x < dimX; x++)
      {
        const usize inSlice = static_cast<usize>(y) * static_cast<usize>(dimX) + static_cast<usize>(x);

        if(!condition(sliceData.get(), inSlice))
        {
          continue;
        }

        int64 nbrA = 0, nbrB = 0, nbrC = 0;

        if(x > 0)
        {
          nbrA = labelBuffer[curOff + inSlice - 1];
        }
        if(y > 0)
        {
          nbrB = labelBuffer[curOff + inSlice - static_cast<usize>(dimX)];
        }
        if(z > 0)
        {
          nbrC = labelBuffer[prevOff + inSlice];
        }

        int64 minLabel = 0;
        if(nbrA > 0)
        {
          minLabel = nbrA;
        }
        if(nbrB > 0 && (minLabel == 0 || nbrB < minLabel))
        {
          minLabel = nbrB;
        }
        if(nbrC > 0 && (minLabel == 0 || nbrC < minLabel))
        {
          minLabel = nbrC;
        }

        int64 assignedLabel = 0;
        if(minLabel == 0)
        {
          assignedLabel = nextLabel++;
        }
        else
        {
          assignedLabel = minLabel;
        }

        labelBuffer[curOff + inSlice] = assignedLabel;

        auto rootResult = equivalences.find(static_cast<uint64>(assignedLabel), shouldCancel);
        if(rootResult.invalid())
        {
          return ConvertResult(std::move(rootResult));
        }
        auto actionResult = action(sliceData.get(), inSlice, rootResult.value(), static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z));
        if(actionResult.invalid())
        {
          return ConvertResult(std::move(actionResult));
        }
        if(actionResult.value())
        {
          modified = true;
        }
      }
    }

    if(modified)
    {
      auto writeResult = store.copyFromBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<const T>(sliceData.get(), sliceSize));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
  }
  return {};
}

/**
 * @brief Labels one buffered 2D plane with rolling row labels.
 * @tparam T Mask value type.
 * @tparam ConditionFn Callable that selects a plane value.
 * @param data Plane data.
 * @param dim1 Plane fast dimension.
 * @param dim2 Plane slow dimension.
 * @param condition Selects values to label.
 * @param allowInMemoryFallback True to permit resident temporary records.
 * @param shouldCancel Cancellation flag.
 * @return CCL state, partial state on cancellation, or an equivalence error.
 *
 * Two label rows cover left and above neighbors. This keeps per-plane CCL
 * memory proportional to its row width.
 */
template <typename T, typename ConditionFn>
Result<CCLResult> runPlaneCCL(T* data, int64 dim1, int64 dim2, ConditionFn condition, bool allowInMemoryFallback, const std::atomic_bool& shouldCancel)
{
  CCLResult result;
  if(shouldCancel)
  {
    return {std::move(result)};
  }
  const uint64 pointCount = static_cast<uint64>(dim1) * static_cast<uint64>(dim2);
  auto equivalencesResult = CreateEquivalences(pointCount, allowInMemoryFallback);
  if(equivalencesResult.invalid())
  {
    return ConvertInvalidResult<CCLResult>(std::move(equivalencesResult));
  }
  result.equivalences = std::move(equivalencesResult.value());
  std::vector<int64> labels(static_cast<usize>(2 * dim1), 0);

  for(int64 row = 0; row < dim2; row++)
  {
    if(shouldCancel)
    {
      return {std::move(result)};
    }
    const usize currentOffset = static_cast<usize>(row % 2) * static_cast<usize>(dim1);
    const usize previousOffset = static_cast<usize>((row + 1) % 2) * static_cast<usize>(dim1);
    std::fill(labels.begin() + currentOffset, labels.begin() + currentOffset + static_cast<usize>(dim1), 0);
    for(int64 column = 0; column < dim1; column++)
    {
      const usize index = static_cast<usize>(row * dim1 + column);
      if(!condition(data[index]))
      {
        continue;
      }
      const int64 left = column > 0 ? labels[currentOffset + static_cast<usize>(column - 1)] : 0;
      const int64 above = row > 0 ? labels[previousOffset + static_cast<usize>(column)] : 0;
      const int64 assigned = left > 0 && above > 0 ? std::min(left, above) : std::max(left, above);
      const int64 label = assigned > 0 ? assigned : static_cast<int64>(result.nextLabel++);
      labels[currentOffset + static_cast<usize>(column)] = label;
      if(left > 0 && left != label)
      {
        auto uniteResult = result.equivalences->unite(static_cast<uint64>(label), static_cast<uint64>(left), shouldCancel);
        if(uniteResult.invalid())
        {
          return ConvertInvalidResult<CCLResult>(std::move(uniteResult));
        }
      }
      if(above > 0 && above != label)
      {
        auto uniteResult = result.equivalences->unite(static_cast<uint64>(label), static_cast<uint64>(above), shouldCancel);
        if(uniteResult.invalid())
        {
          return ConvertInvalidResult<CCLResult>(std::move(uniteResult));
        }
      }
      auto sizeResult = result.equivalences->addSize(static_cast<uint64>(label), 1, shouldCancel);
      if(sizeResult.invalid())
      {
        return ConvertInvalidResult<CCLResult>(std::move(sizeResult));
      }
    }
  }
  for(uint64 label = 1; label < result.nextLabel; label++)
  {
    auto rootResult = result.equivalences->find(label, shouldCancel);
    if(rootResult.invalid())
    {
      return ConvertInvalidResult<CCLResult>(std::move(rootResult));
    }
    if(rootResult.value() == label)
    {
      auto sizeResult = result.equivalences->componentSize(label, shouldCancel);
      if(sizeResult.invalid())
      {
        return ConvertInvalidResult<CCLResult>(std::move(sizeResult));
      }
      if(sizeResult.value() >= result.largestSize)
      {
        result.largestRoot = label;
        result.largestSize = sizeResult.value();
      }
    }
  }
  auto flushResult = result.equivalences->flush(shouldCancel);
  if(flushResult.invalid())
  {
    return ConvertInvalidResult<CCLResult>(std::move(flushResult));
  }
  return {std::move(result)};
}

/**
 * @brief Replays buffered 2D labels and invokes an action for each selected value.
 * @tparam T Mask value type.
 * @tparam ConditionFn Callable that selects a plane value.
 * @tparam ActionFn Callable that processes a resolved plane root.
 * @param data Plane data.
 * @param dim1 Plane fast dimension.
 * @param dim2 Plane slow dimension.
 * @param equivalences Provisional-label equivalences from the plane scan.
 * @param condition Selects values to label.
 * @param action Processes each resolved root.
 * @param shouldCancel Cancellation flag.
 * @return Equivalence, action, or cancellation result.
 */
template <typename T, typename ConditionFn, typename ActionFn>
Result<> replayPlaneCCL(T* data, int64 dim1, int64 dim2, ExternalEquivalence& equivalences, ConditionFn condition, ActionFn action, const std::atomic_bool& shouldCancel)
{
  std::vector<int64> labels(static_cast<usize>(2 * dim1), 0);
  uint64 nextLabel = 1;
  for(int64 row = 0; row < dim2; row++)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize currentOffset = static_cast<usize>(row % 2) * static_cast<usize>(dim1);
    const usize previousOffset = static_cast<usize>((row + 1) % 2) * static_cast<usize>(dim1);
    std::fill(labels.begin() + currentOffset, labels.begin() + currentOffset + static_cast<usize>(dim1), 0);
    for(int64 column = 0; column < dim1; column++)
    {
      const usize index = static_cast<usize>(row * dim1 + column);
      if(!condition(data[index]))
      {
        continue;
      }
      const int64 left = column > 0 ? labels[currentOffset + static_cast<usize>(column - 1)] : 0;
      const int64 above = row > 0 ? labels[previousOffset + static_cast<usize>(column)] : 0;
      const int64 label = left > 0 && above > 0 ? std::min(left, above) : std::max(left, above);
      const uint64 assigned = label > 0 ? static_cast<uint64>(label) : nextLabel++;
      labels[currentOffset + static_cast<usize>(column)] = static_cast<int64>(assigned);
      auto rootResult = equivalences.find(assigned, shouldCancel);
      if(rootResult.invalid())
      {
        return ConvertResult(std::move(rootResult));
      }
      auto actionResult = action(data[index], rootResult.value(), column, row);
      if(actionResult.invalid())
      {
        return ConvertResult(std::move(actionResult));
      }
    }
  }
  return {};
}

/**
 * @brief Retains the largest good plane component and optionally fills holes.
 * @tparam T Mask value type.
 * @param data Plane data modified in place.
 * @param dim1 Plane fast dimension.
 * @param dim2 Plane slow dimension.
 * @param fillHoles True to fill non-boundary bad components.
 * @param allowInMemoryFallback True to permit resident temporary records.
 * @param shouldCancel Cancellation flag.
 * @return CCL, root-flag, replay, or cancellation result.
 */
template <typename T>
Result<> identifyPlane(T* data, int64 dim1, int64 dim2, bool fillHoles, bool allowInMemoryFallback, const std::atomic_bool& shouldCancel)
{
  const auto good = [](T value) { return static_cast<bool>(value); };
  auto goodResult = runPlaneCCL(data, dim1, dim2, good, allowInMemoryFallback, shouldCancel);
  if(goodResult.invalid())
  {
    return ConvertResult(std::move(goodResult));
  }
  if(shouldCancel || goodResult.value().largestRoot == 0)
  {
    return {};
  }
  const uint64 largestRoot = goodResult.value().largestRoot;
  auto removeResult = replayPlaneCCL(
      data, dim1, dim2, *goodResult.value().equivalences, good,
      [largestRoot](T& value, uint64 root, int64, int64) -> Result<> {
        if(root != largestRoot)
        {
          value = static_cast<T>(false);
        }
        return {};
      },
      shouldCancel);
  if(removeResult.invalid() || shouldCancel || !fillHoles)
  {
    return removeResult;
  }
  const auto bad = [](T value) { return !static_cast<bool>(value); };
  auto holesResult = runPlaneCCL(data, dim1, dim2, bad, allowInMemoryFallback, shouldCancel);
  if(holesResult.invalid())
  {
    return ConvertResult(std::move(holesResult));
  }
  auto flagsResult = ExternalRootFlags::Create(holesResult.value().nextLabel, allowInMemoryFallback);
  if(flagsResult.invalid())
  {
    return ConvertResult(std::move(flagsResult));
  }
  auto flags = std::move(flagsResult.value());
  auto boundaryResult = replayPlaneCCL(
      data, dim1, dim2, *holesResult.value().equivalences, bad,
      [&flags, dim1, dim2, &shouldCancel](T&, uint64 root, int64 column, int64 row) -> Result<> {
        if(column == 0 || column == dim1 - 1 || row == 0 || row == dim2 - 1)
        {
          return flags->mark(root, shouldCancel);
        }
        return {};
      },
      shouldCancel);
  if(boundaryResult.invalid())
  {
    return boundaryResult;
  }
  auto flushResult = flags->flush(shouldCancel);
  if(flushResult.invalid())
  {
    return flushResult;
  }
  return replayPlaneCCL(
      data, dim1, dim2, *holesResult.value().equivalences, bad,
      [&flags, &shouldCancel](T& value, uint64 root, int64, int64) -> Result<> {
        auto markedResult = flags->isMarked(root, shouldCancel);
        if(markedResult.invalid())
        {
          return ConvertResult(std::move(markedResult));
        }
        if(!markedResult.value())
        {
          value = static_cast<T>(true);
        }
        return {};
      },
      shouldCancel);
}

/**
 * @struct IdentifySampleSliceCCLFunctor
 * @brief Dispatches bounded slice-by-slice CCL by mask value type.
 */
struct IdentifySampleSliceCCLFunctor
{
  /**
   * @brief Processes selected planes with one plane buffer at a time.
   * @tparam T Mask value type.
   * @param imageGeom Image geometry that supplies dimensions.
   * @param maskArray Mask array modified in place.
   * @param fillHoles True to fill non-boundary bad components.
   * @param plane Selected plane orientation.
   * @param messageHandler Receives slice progress messages.
   * @param shouldCancel Cancellation flag.
   * @return Bulk-I/O, CCL, or cancellation result.
   *
   * XY planes transfer contiguously. XZ planes transfer contiguous rows. YZ
   * planes bulk-read Z slices before extracting and updating one column.
   */
  template <typename T>
  Result<> operator()(const ImageGeom* imageGeom, IDataArray* maskArray, bool fillHoles, IdentifySampleSliceBySliceFunctor::Plane plane, const IFilter::MessageHandler& messageHandler,
                      const std::atomic_bool& shouldCancel) const
  {
    auto& store = maskArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    const bool allowInMemoryFallback = store.getStoreType() != IDataStore::StoreType::OutOfCore;
    const SizeVec3 dimensions = imageGeom->getDimensions();
    const int64 dimX = static_cast<int64>(dimensions[0]);
    const int64 dimY = static_cast<int64>(dimensions[1]);
    const int64 dimZ = static_cast<int64>(dimensions[2]);
    const usize zSliceSize = static_cast<usize>(dimX * dimY);

    int64 planeDim1 = 0;
    int64 planeDim2 = 0;
    int64 fixedDim = 0;
    if(plane == IdentifySampleSliceBySliceFunctor::Plane::XY)
    {
      planeDim1 = dimX;
      planeDim2 = dimY;
      fixedDim = dimZ;
    }
    else if(plane == IdentifySampleSliceBySliceFunctor::Plane::XZ)
    {
      planeDim1 = dimX;
      planeDim2 = dimZ;
      fixedDim = dimY;
    }
    else
    {
      planeDim1 = dimY;
      planeDim2 = dimZ;
      fixedDim = dimX;
    }
    const usize planeSize = static_cast<usize>(planeDim1 * planeDim2);
    auto planeBuffer = std::make_unique<T[]>(planeSize);
    auto zBuffer = std::make_unique<T[]>(zSliceSize);

    for(int64 fixed = 0; fixed < fixedDim; fixed++)
    {
      if(shouldCancel)
      {
        return {};
      }
      messageHandler(IFilter::Message::Type::Info, fmt::format("Slice {}", fixed));
      if(plane == IdentifySampleSliceBySliceFunctor::Plane::XY)
      {
        auto readResult = store.copyIntoBuffer(static_cast<usize>(fixed) * planeSize, nonstd::span<T>(planeBuffer.get(), planeSize));
        if(readResult.invalid())
        {
          return readResult;
        }
      }
      else if(plane == IdentifySampleSliceBySliceFunctor::Plane::XZ)
      {
        for(int64 z = 0; z < dimZ; z++)
        {
          auto readResult = store.copyIntoBuffer(static_cast<usize>(z * dimX * dimY + fixed * dimX), nonstd::span<T>(planeBuffer.get() + static_cast<usize>(z * dimX), static_cast<usize>(dimX)));
          if(readResult.invalid())
          {
            return readResult;
          }
        }
      }
      else
      {
        for(int64 z = 0; z < dimZ; z++)
        {
          auto readResult = store.copyIntoBuffer(static_cast<usize>(z) * zSliceSize, nonstd::span<T>(zBuffer.get(), zSliceSize));
          if(readResult.invalid())
          {
            return readResult;
          }
          for(int64 y = 0; y < dimY; y++)
          {
            planeBuffer[static_cast<usize>(z * dimY + y)] = zBuffer[static_cast<usize>(y * dimX + fixed)];
          }
        }
      }

      auto identifyResult = identifyPlane(planeBuffer.get(), planeDim1, planeDim2, fillHoles, allowInMemoryFallback, shouldCancel);
      if(identifyResult.invalid())
      {
        return identifyResult;
      }
      if(shouldCancel)
      {
        return {};
      }

      if(plane == IdentifySampleSliceBySliceFunctor::Plane::XY)
      {
        auto writeResult = store.copyFromBuffer(static_cast<usize>(fixed) * planeSize, nonstd::span<const T>(planeBuffer.get(), planeSize));
        if(writeResult.invalid())
        {
          return writeResult;
        }
      }
      else if(plane == IdentifySampleSliceBySliceFunctor::Plane::XZ)
      {
        for(int64 z = 0; z < dimZ; z++)
        {
          auto writeResult =
              store.copyFromBuffer(static_cast<usize>(z * dimX * dimY + fixed * dimX), nonstd::span<const T>(planeBuffer.get() + static_cast<usize>(z * dimX), static_cast<usize>(dimX)));
          if(writeResult.invalid())
          {
            return writeResult;
          }
        }
      }
      else
      {
        for(int64 z = 0; z < dimZ; z++)
        {
          auto readResult = store.copyIntoBuffer(static_cast<usize>(z) * zSliceSize, nonstd::span<T>(zBuffer.get(), zSliceSize));
          if(readResult.invalid())
          {
            return readResult;
          }
          for(int64 y = 0; y < dimY; y++)
          {
            zBuffer[static_cast<usize>(y * dimX + fixed)] = planeBuffer[static_cast<usize>(z * dimY + y)];
          }
          auto writeResult = store.copyFromBuffer(static_cast<usize>(z) * zSliceSize, nonstd::span<const T>(zBuffer.get(), zSliceSize));
          if(writeResult.invalid())
          {
            return writeResult;
          }
        }
      }
    }
    return {};
  }
};

/**
 * @struct IdentifySampleCCLFunctor
 * @brief Runs full-volume bounded CCL and replay by mask value type.
 */
struct IdentifySampleCCLFunctor
{
  /**
   * @brief Retains the largest sample and optionally fills interior holes.
   * @tparam T Mask value type.
   * @param imageGeom Image geometry that supplies dimensions.
   * @param goodVoxelsPtr Mask array modified in place.
   * @param fillHoles True to fill non-boundary bad components.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Cancellation flag.
   * @return CCL, replay, root-flag, bulk-I/O, or cancellation result.
   *
   * The function discovers good components, replays to remove non-sample roots,
   * then optionally identifies boundary-connected bad roots before filling holes.
   */
  template <typename T>
  Result<> operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    SizeVec3 udims = imageGeom->getDimensions();
    const int64 dimX = static_cast<int64>(udims[0]);
    const int64 dimY = static_cast<int64>(udims[1]);
    const int64 dimZ = static_cast<int64>(udims[2]);

    // Discover good components and select the largest root.
    auto goodCondition = [](const T* data, usize inSlice) -> bool { return static_cast<bool>(data[inSlice]); };
    auto cclResult = runForwardCCL<T>(goodVoxels, dimX, dimY, dimZ, goodCondition, shouldCancel);
    if(cclResult.invalid())
    {
      return ConvertResult(std::move(cclResult));
    }

    if(shouldCancel || cclResult.value().largestRoot == 0)
    {
      return {};
    }

    // Replay the same traversal to remove good voxels outside the largest root.
    const uint64 largestRoot = cclResult.value().largestRoot;
    auto replayGoodResult = replayForwardCCL<T>(
        goodVoxels, dimX, dimY, dimZ, *cclResult.value().equivalences, goodCondition,
        [&largestRoot](T* data, usize inSlice, uint64 root, usize /*x*/, usize /*y*/, usize /*z*/) -> Result<bool> {
          if(root != largestRoot)
          {
            data[inSlice] = static_cast<T>(false);
            return {true};
          }
          return {false};
        },
        shouldCancel);
    if(replayGoodResult.invalid())
    {
      return replayGoodResult;
    }

    if(shouldCancel)
    {
      return {};
    }

    if(fillHoles)
    {
      // Discover bad components. Boundary-connected roots are exterior space.
      auto holeCondition = [](const T* data, usize inSlice) -> bool { return !static_cast<bool>(data[inSlice]); };
      auto holeCCL = runForwardCCL<T>(goodVoxels, dimX, dimY, dimZ, holeCondition, shouldCancel);
      if(holeCCL.invalid())
      {
        return ConvertResult(std::move(holeCCL));
      }

      if(shouldCancel)
      {
        return {};
      }

      // Replay bad labels to mark roots that touch a domain boundary.
      const bool allowInMemoryFallback = goodVoxels.getStoreType() != IDataStore::StoreType::OutOfCore;
      auto boundaryRootsResult = ExternalRootFlags::Create(holeCCL.value().nextLabel, allowInMemoryFallback);
      if(boundaryRootsResult.invalid())
      {
        return ConvertResult(std::move(boundaryRootsResult));
      }
      auto boundaryRoots = std::move(boundaryRootsResult.value());
      auto boundaryResult = replayForwardCCL<T>(
          goodVoxels, dimX, dimY, dimZ, *holeCCL.value().equivalences, holeCondition,
          [&boundaryRoots, dimX, dimY, dimZ, &shouldCancel](T* /*data*/, usize /*inSlice*/, uint64 root, usize x, usize y, usize z) -> Result<bool> {
            if(x == 0 || x == static_cast<usize>(dimX - 1) || y == 0 || y == static_cast<usize>(dimY - 1) || z == 0 || z == static_cast<usize>(dimZ - 1))
            {
              auto markResult = boundaryRoots->mark(root, shouldCancel);
              if(markResult.invalid())
              {
                return ConvertInvalidResult<bool>(std::move(markResult));
              }
            }
            return {false};
          },
          shouldCancel);
      if(boundaryResult.invalid())
      {
        return boundaryResult;
      }
      auto flushResult = boundaryRoots->flush(shouldCancel);
      if(flushResult.invalid())
      {
        return flushResult;
      }

      if(shouldCancel)
      {
        return {};
      }

      // Replay bad labels again to fill roots that do not touch the boundary.
      auto fillResult = replayForwardCCL<T>(
          goodVoxels, dimX, dimY, dimZ, *holeCCL.value().equivalences, holeCondition,
          [&boundaryRoots, &shouldCancel](T* data, usize inSlice, uint64 root, usize /*x*/, usize /*y*/, usize /*z*/) -> Result<bool> {
            auto markedResult = boundaryRoots->isMarked(root, shouldCancel);
            if(markedResult.invalid())
            {
              return ConvertInvalidResult<bool>(std::move(markedResult));
            }
            if(!markedResult.value())
            {
              data[inSlice] = static_cast<T>(true);
              return {true};
            }
            return {false};
          },
          shouldCancel);
      if(fillResult.invalid())
      {
        return fillResult;
      }
    }
    return {};
  }
};
} // namespace

IdentifySampleCCL::IdentifySampleCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

IdentifySampleCCL::~IdentifySampleCCL() noexcept = default;

Result<> IdentifySampleCCL::operator()()
{
  auto* inputData = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometryPath);

  if(m_InputValues->SliceBySlice)
  {
    return ExecuteDataFunction(IdentifySampleSliceCCLFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles,
                               static_cast<IdentifySampleSliceBySliceFunctor::Plane>(m_InputValues->SliceBySlicePlaneIndex), m_MessageHandler, m_ShouldCancel);
  }
  else
  {
    return ExecuteDataFunction(IdentifySampleCCLFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}
