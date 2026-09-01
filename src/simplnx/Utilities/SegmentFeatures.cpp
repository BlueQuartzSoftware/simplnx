#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ExternalEquivalence.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/UnionFind.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <vector>

using namespace nx::core;

namespace
{
constexpr uint64 k_RecordsPerPage = 4096;
constexpr usize k_MaxCachedPages = 16;

/**
 * @brief Creates fixed-record connected-component scratch.
 * @param recordSize Specifies bytes per record.
 * @param recordCount Specifies initial records.
 * @param allowInMemoryFallback Permits a resident provider when true.
 * @return Temporary store or provider error.
 *
 * Genuine OOC input disables resident fallback. This prevents equivalence tables
 * from silently becoming volume-scale RAM allocations.
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
    result = {std::unique_ptr<ITemporaryRecordStore>(std::move(fallbackResult.value()))};
  }
  if(result.valid() && result.value() == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-87010, "SegmentFeatures temporary-record provider returned a null store.");
  }
  return result;
}

/**
 * @class LabelEquivalence
 * @brief Storage-neutral union/find and final-label table for scanline CCL.
 *
 * Resident execution delegates to UnionFind and a vector. OOC execution stores
 * both provisional equivalences and dense final labels behind bounded caches,
 * preventing worst-case label state from scaling resident memory with volume.
 */
class LabelEquivalence
{
public:
  /**
   * @brief Creates resident or external state for a maximum provisional label.
   * @param useExternalStorage Selects temporary record stores when true.
   * @param maximumLabel Specifies the largest possible provisional label.
   * @param allowInMemoryFallback Permits resident temporary stores when true.
   * @return Initialized equivalence state or allocation/provider error.
   */
  static Result<std::unique_ptr<LabelEquivalence>> Create(bool useExternalStorage, uint64 maximumLabel, bool allowInMemoryFallback)
  {
    try
    {
      auto result = std::unique_ptr<LabelEquivalence>(new LabelEquivalence());
      result->m_UseExternalStorage = useExternalStorage;
      if(!useExternalStorage)
      {
        return {std::move(result)};
      }
      if(maximumLabel == std::numeric_limits<uint64>::max())
      {
        return MakeErrorResult<std::unique_ptr<LabelEquivalence>>(-87011, "SegmentFeatures cannot create external equivalences for this image size.");
      }

      auto equivalenceStoreResult = CreateTemporaryRecordStore(sizeof(ExternalEquivalence::Node), maximumLabel + 1, allowInMemoryFallback);
      if(equivalenceStoreResult.invalid())
      {
        return ConvertInvalidResult<std::unique_ptr<LabelEquivalence>>(std::move(equivalenceStoreResult));
      }
      auto equivalenceResult = ExternalEquivalence::Create(std::move(equivalenceStoreResult.value()), k_RecordsPerPage, k_MaxCachedPages);
      if(equivalenceResult.invalid())
      {
        return ConvertInvalidResult<std::unique_ptr<LabelEquivalence>>(std::move(equivalenceResult));
      }
      result->m_ExternalEquivalence = std::move(equivalenceResult.value());

      auto finalLabelStoreResult = CreateTemporaryRecordStore(sizeof(int32), maximumLabel + 1, allowInMemoryFallback);
      if(finalLabelStoreResult.invalid())
      {
        return ConvertInvalidResult<std::unique_ptr<LabelEquivalence>>(std::move(finalLabelStoreResult));
      }
      result->m_FinalLabelStore = std::move(finalLabelStoreResult.value());
      result->m_FinalLabelCache = std::make_unique<BoundedRecordPageCache<int32>>(*result->m_FinalLabelStore, k_RecordsPerPage, k_MaxCachedPages);
      return {std::move(result)};
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<std::unique_ptr<LabelEquivalence>>(-87012, "SegmentFeatures could not allocate its bounded equivalence cache.");
    }
  }

  /**
   * @brief Finds the canonical root of one provisional label.
   * @param label Specifies the provisional label.
   * @param shouldCancel Stops external cache work when true.
   * @return Canonical root or external-store error.
   */
  Result<uint64> find(uint64 label, const std::atomic_bool& shouldCancel)
  {
    if(m_UseExternalStorage)
    {
      return m_ExternalEquivalence->find(label, shouldCancel);
    }
    return {static_cast<uint64>(m_DirectEquivalence.find(static_cast<int64>(label)))};
  }

  /**
   * @brief Materializes one provisional label in the selected backend.
   * @param label Specifies the provisional label.
   * @param shouldCancel Stops external cache work when true.
   * @return External-store error or success.
   */
  Result<> initialize(uint64 label, const std::atomic_bool& shouldCancel)
  {
    auto result = find(label, shouldCancel);
    if(result.invalid())
    {
      return ConvertResult(std::move(result));
    }
    return {};
  }

  /**
   * @brief Unites two provisional labels.
   * @param left Specifies the first label.
   * @param right Specifies the second label.
   * @param shouldCancel Stops external cache work when true.
   * @return External-store error or success.
   */
  Result<> unite(uint64 left, uint64 right, const std::atomic_bool& shouldCancel)
  {
    if(m_UseExternalStorage)
    {
      return m_ExternalEquivalence->unite(left, right, shouldCancel);
    }
    m_DirectEquivalence.unite(static_cast<int64>(left), static_cast<int64>(right));
    return {};
  }

  /**
   * @brief Prepares state for dense final relabeling.
   * @param nextLabel Specifies one past the largest provisional label.
   * @return Allocation error or success.
   */
  Result<> prepareFinalLabels(uint64 nextLabel)
  {
    if(!m_UseExternalStorage)
    {
      try
      {
        m_DirectFinalLabels.assign(static_cast<usize>(nextLabel), 0);
      } catch(const std::bad_alloc&)
      {
        return MakeErrorResult(-87013, "SegmentFeatures could not allocate its in-core final-label table.");
      }
      m_DirectEquivalence.flatten();
    }
    return {};
  }

  /**
   * @brief Maps one provisional label's root to a stable dense Int32 feature ID.
   * @param label Specifies the provisional label.
   * @param finalFeatureCount Provides and receives the largest dense ID.
   * @param shouldCancel Stops external cache work when true.
   * @return Dense Feature ID or external-store error.
   *
   * The mapping is cached so later voxels in the same component avoid another root walk.
   */
  Result<int32> resolveFinalLabel(uint64 label, int32& finalFeatureCount, const std::atomic_bool& shouldCancel)
  {
    auto cachedResult = finalLabel(label, shouldCancel);
    if(cachedResult.invalid())
    {
      return cachedResult;
    }
    if(cachedResult.value() != 0)
    {
      return cachedResult;
    }

    auto rootResult = find(label, shouldCancel);
    if(rootResult.invalid())
    {
      return ConvertInvalidResult<int32>(std::move(rootResult));
    }
    const uint64 root = rootResult.value();
    auto rootFinalResult = finalLabel(root, shouldCancel);
    if(rootFinalResult.invalid())
    {
      return rootFinalResult;
    }
    int32 rootFinal = rootFinalResult.value();
    if(rootFinal == 0)
    {
      if(finalFeatureCount == std::numeric_limits<int32>::max())
      {
        return MakeErrorResult<int32>(-87014, "SegmentFeatures exceeded the Int32 feature-ID capacity.");
      }
      rootFinal = ++finalFeatureCount;
      auto writeRootResult = setFinalLabel(root, rootFinal, shouldCancel);
      if(writeRootResult.invalid())
      {
        return ConvertResultTo<int32>(std::move(writeRootResult), int32{});
      }
    }
    if(root != label)
    {
      auto writeLabelResult = setFinalLabel(label, rootFinal, shouldCancel);
      if(writeLabelResult.invalid())
      {
        return ConvertResultTo<int32>(std::move(writeLabelResult), int32{});
      }
    }
    return {rootFinal};
  }

  /**
   * @brief Flushes external equivalence and final-label pages.
   * @param shouldCancel Stops before later cache flushes when true.
   * @return First external-store error, or success.
   */
  Result<> flush(const std::atomic_bool& shouldCancel)
  {
    if(!m_UseExternalStorage)
    {
      return {};
    }
    auto equivalenceResult = m_ExternalEquivalence->flush(shouldCancel);
    if(equivalenceResult.invalid())
    {
      return equivalenceResult;
    }
    return m_FinalLabelCache->flush(shouldCancel);
  }

private:
  /**
   * @brief Reads a cached dense label.
   * @param label Specifies a provisional label or root.
   * @param shouldCancel Stops external cache work when true.
   * @return Dense label, zero when unresolved, or external-store error.
   */
  Result<int32> finalLabel(uint64 label, const std::atomic_bool& shouldCancel)
  {
    if(m_UseExternalStorage)
    {
      return m_FinalLabelCache->read(label, shouldCancel);
    }
    return {m_DirectFinalLabels[static_cast<usize>(label)]};
  }

  /**
   * @brief Caches one dense label.
   * @param label Specifies a provisional label or root.
   * @param finalLabel Specifies its dense Feature ID.
   * @param shouldCancel Stops external cache work when true.
   * @return External-store error or success.
   */
  Result<> setFinalLabel(uint64 label, int32 finalLabel, const std::atomic_bool& shouldCancel)
  {
    if(m_UseExternalStorage)
    {
      return m_FinalLabelCache->write(label, finalLabel, shouldCancel);
    }
    m_DirectFinalLabels[static_cast<usize>(label)] = finalLabel;
    return {};
  }

  LabelEquivalence() = default;

  bool m_UseExternalStorage = false;
  UnionFind m_DirectEquivalence;
  std::vector<int32> m_DirectFinalLabels;
  std::unique_ptr<ExternalEquivalence> m_ExternalEquivalence;
  std::unique_ptr<ITemporaryRecordStore> m_FinalLabelStore;
  std::unique_ptr<BoundedRecordPageCache<int32>> m_FinalLabelCache;
};
} // namespace

SegmentFeatures::SegmentFeatures(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHelper(mesgHandler)
{
}

SegmentFeatures::~SegmentFeatures() = default;

// CCL uses three stages. The forward scan creates provisional labels and
// equivalences. Periodic merging joins wrapped boundaries. Final resolution writes dense IDs.
Result<> SegmentFeatures::executeCCL(IGridGeometry* gridGeom, AbstractDataStore<int32>& featureIdsStore, bool usesOutOfCoreInput)
{
  const SizeVec3 udims = gridGeom->getDimensions();
  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);
  const usize totalVoxels = static_cast<usize>(dimX) * static_cast<usize>(dimY) * static_cast<usize>(dimZ);

  const int64 sliceStride = dimX * dimY;

  const bool useFaceOnly = (m_NeighborScheme == NeighborScheme::Face);
  bool hasNonContiguousFeature = false;

  const bool usesOutOfCoreStore = usesOutOfCoreInput || featureIdsStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  const bool useExternalEquivalence = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useExternalEquivalence ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  auto equivalenceResult = LabelEquivalence::Create(useExternalEquivalence, static_cast<uint64>(totalVoxels), !usesOutOfCoreStore);
  if(equivalenceResult.invalid())
  {
    return ConvertResult(std::move(equivalenceResult));
  }
  auto equivalences = std::move(equivalenceResult.value());
  int32 nextLabel = 1;

  // Backward neighbors use only the current and previous Z slices. A two-slice
  // rolling buffer therefore keeps label memory proportional to slice area.
  const usize sliceSize = static_cast<usize>(sliceStride);
  std::vector<int32> labelBuffer(2 * sliceSize, 0);

  // The forward pass writes one complete Feature-ID slice at a time.
  std::vector<int32> featureIdsSlice(sliceSize, 0);

  for(int64 iz = 0; iz < dimZ; iz++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    auto prepareResult = prepareForSlice(iz, dimX, dimY, dimZ);
    if(prepareResult.invalid())
    {
      return prepareResult;
    }

    // Clear invalid-voxel positions before the slice write.
    std::fill(featureIdsSlice.begin(), featureIdsSlice.end(), 0);

    // Reuse the current rolling-buffer slot for this slice.
    const usize currentSliceOffset = static_cast<usize>(iz % 2) * sliceSize;
    std::fill(labelBuffer.begin() + currentSliceOffset, labelBuffer.begin() + currentSliceOffset + sliceSize, 0);

    for(int64 iy = 0; iy < dimY; iy++)
    {
      for(int64 ix = 0; ix < dimX; ix++)
      {
        const int64 index = iz * sliceStride + iy * dimX + ix;
        const usize bufIdx = currentSliceOffset + static_cast<usize>(iy * dimX + ix);

        if(!isValidVoxel(index))
        {
          continue;
        }

        // A backward neighbor has a smaller Z-Y-X linear index and already has
        // a provisional label. Read these labels from the rolling buffer.
        int32 assignedLabel = 0;
        const usize prevSliceOffset = static_cast<usize>((iz + 1) % 2) * sliceSize;

        if(useFaceOnly)
        {
          // Face connectivity checks the three labeled directions: -X, -Y, and -Z.

          if(ix > 0)
          {
            const int64 neighIdx = index - 1;
            int32 neighLabel = labelBuffer[bufIdx - 1];
            if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                auto uniteResult = equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(neighLabel), m_ShouldCancel);
                if(uniteResult.invalid())
                {
                  return uniteResult;
                }
              }
            }
          }
          if(iy > 0)
          {
            const int64 neighIdx = index - dimX;
            int32 neighLabel = labelBuffer[currentSliceOffset + static_cast<usize>((iy - 1) * dimX + ix)];
            if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                auto uniteResult = equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(neighLabel), m_ShouldCancel);
                if(uniteResult.invalid())
                {
                  return uniteResult;
                }
              }
            }
          }
          if(iz > 0)
          {
            const int64 neighIdx = index - sliceStride;
            int32 neighLabel = labelBuffer[prevSliceOffset + static_cast<usize>(iy * dimX + ix)];
            if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                auto uniteResult = equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(neighLabel), m_ShouldCancel);
                if(uniteResult.invalid())
                {
                  return uniteResult;
                }
              }
            }
          }
        }
        else
        {
          // Complete connectivity has 13 backward neighbors. The previous slice
          // supplies nine. The current slice supplies three from -Y and one from -X.
          for(int64 dz = -1; dz <= 0; ++dz)
          {
            const int64 nz = iz + dz;
            if(nz < 0 || nz >= dimZ)
            {
              continue;
            }

            const usize neighSliceOffset = (dz < 0) ? prevSliceOffset : currentSliceOffset;

            const int64 dyStart = -1;
            const int64 dyEnd = (dz < 0) ? 1 : 0;

            for(int64 dy = dyStart; dy <= dyEnd; ++dy)
            {
              const int64 ny = iy + dy;
              if(ny < 0 || ny >= dimY)
              {
                continue;
              }

              int64 dxStart;
              int64 dxEnd;
              if(dz < 0)
              {
                dxStart = -1;
                dxEnd = 1;
              }
              else if(dy < 0)
              {
                dxStart = -1;
                dxEnd = 1;
              }
              else
              {
                dxStart = -1;
                dxEnd = -1;
              }

              for(int64 dx = dxStart; dx <= dxEnd; ++dx)
              {
                const int64 nx = ix + dx;
                if(nx < 0 || nx >= dimX)
                {
                  continue;
                }
                if(dx == 0 && dy == 0 && dz == 0)
                {
                  continue;
                }

                const int64 neighIdx = nz * sliceStride + ny * dimX + nx;
                int32 neighLabel = labelBuffer[neighSliceOffset + static_cast<usize>(ny * dimX + nx)];
                if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
                {
                  if(assignedLabel == 0)
                  {
                    assignedLabel = neighLabel;
                  }
                  else if(assignedLabel != neighLabel)
                  {
                    auto uniteResult = equivalences->unite(static_cast<uint64>(assignedLabel), static_cast<uint64>(neighLabel), m_ShouldCancel);
                    if(uniteResult.invalid())
                    {
                      return uniteResult;
                    }
                  }
                }
              }
            }
          }
        }

        // Create a provisional label when no backward neighbor matches.
        if(assignedLabel == 0)
        {
          if(nextLabel == std::numeric_limits<int32>::max())
          {
            return MakeErrorResult(-87014, "SegmentFeatures exceeded the Int32 provisional-label capacity.");
          }
          assignedLabel = nextLabel++;
          auto initializeResult = equivalences->initialize(static_cast<uint64>(assignedLabel), m_ShouldCancel);
          if(initializeResult.invalid())
          {
            return initializeResult;
          }
        }

        // Keep the label for neighbor lookup and the final slice write.
        labelBuffer[bufIdx] = assignedLabel;
        const usize inSlice = static_cast<usize>(iy * dimX + ix);
        featureIdsSlice[inSlice] = assignedLabel;
      }
    }

    // Write the completed provisional-label slice.
    auto writeResult = featureIdsStore.copyFromBuffer(static_cast<usize>(iz) * sliceSize, nonstd::span<const int32>(featureIdsSlice.data(), sliceSize));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // The forward scan cannot see wrapped neighbors with higher linear indexes.
  // Periodic merging reads one or two label slices and joins opposite boundaries.
  if(m_IsPeriodic)
  {
    std::vector<int32> featureIdsSliceCur(sliceSize, 0);

    if(useFaceOnly)
    {
      // Face connectivity compares low and high boundary faces independently.

      // X boundaries share one Z slice.
      if(dimX > 1)
      {
        for(int64 iz = 0; iz < dimZ; iz++)
        {
          auto readResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(iz) * sliceSize, nonstd::span<int32>(featureIdsSliceCur.data(), sliceSize));
          if(readResult.invalid())
          {
            return readResult;
          }
          auto prepareResult = prepareForSlice(iz, dimX, dimY, dimZ);
          if(prepareResult.invalid())
          {
            return prepareResult;
          }

          for(int64 iy = 0; iy < dimY; iy++)
          {
            const int64 idxA = iz * sliceStride + iy * dimX;
            const int64 idxB = iz * sliceStride + iy * dimX + (dimX - 1);
            const int32 labelA = featureIdsSliceCur[static_cast<usize>(iy * dimX)];
            const int32 labelB = featureIdsSliceCur[static_cast<usize>(iy * dimX + (dimX - 1))];
            if(labelA > 0 && labelB > 0 && areNeighborsSimilar(idxA, idxB))
            {
              auto uniteResult = equivalences->unite(static_cast<uint64>(labelA), static_cast<uint64>(labelB), m_ShouldCancel);
              if(uniteResult.invalid())
              {
                return uniteResult;
              }
              hasNonContiguousFeature = true;
            }
          }
        }
      }

      // Y boundaries share one Z slice.
      if(dimY > 1)
      {
        for(int64 iz = 0; iz < dimZ; iz++)
        {
          auto readResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(iz) * sliceSize, nonstd::span<int32>(featureIdsSliceCur.data(), sliceSize));
          if(readResult.invalid())
          {
            return readResult;
          }
          auto prepareResult = prepareForSlice(iz, dimX, dimY, dimZ);
          if(prepareResult.invalid())
          {
            return prepareResult;
          }

          for(int64 ix = 0; ix < dimX; ix++)
          {
            const int64 idxA = iz * sliceStride + ix;
            const int64 idxB = iz * sliceStride + (dimY - 1) * dimX + ix;
            const int32 labelA = featureIdsSliceCur[static_cast<usize>(ix)];
            const int32 labelB = featureIdsSliceCur[static_cast<usize>((dimY - 1) * dimX + ix)];
            if(labelA > 0 && labelB > 0 && areNeighborsSimilar(idxA, idxB))
            {
              auto uniteResult = equivalences->unite(static_cast<uint64>(labelA), static_cast<uint64>(labelB), m_ShouldCancel);
              if(uniteResult.invalid())
              {
                return uniteResult;
              }
              hasNonContiguousFeature = true;
            }
          }
        }
      }

      // Z boundaries use separate first-slice and last-slice buffers.
      if(dimZ > 1)
      {
        std::vector<int32> featureIdsSliceOther(sliceSize, 0);
        auto firstReadResult = featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(featureIdsSliceCur.data(), sliceSize));
        if(firstReadResult.invalid())
        {
          return firstReadResult;
        }
        auto lastReadResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(dimZ - 1) * sliceSize, nonstd::span<int32>(featureIdsSliceOther.data(), sliceSize));
        if(lastReadResult.invalid())
        {
          return lastReadResult;
        }

        // Prepare both boundary slices for subclass comparisons.
        auto firstPrepareResult = prepareForSlice(0, dimX, dimY, dimZ);
        if(firstPrepareResult.invalid())
        {
          return firstPrepareResult;
        }
        auto lastPrepareResult = prepareForSlice(dimZ - 1, dimX, dimY, dimZ);
        if(lastPrepareResult.invalid())
        {
          return lastPrepareResult;
        }

        for(int64 iy = 0; iy < dimY; iy++)
        {
          for(int64 ix = 0; ix < dimX; ix++)
          {
            const usize inSlice = static_cast<usize>(iy * dimX + ix);
            const int64 idxA = iy * dimX + ix;
            const int64 idxB = (dimZ - 1) * sliceStride + iy * dimX + ix;
            const int32 labelA = featureIdsSliceCur[inSlice];
            const int32 labelB = featureIdsSliceOther[inSlice];
            if(labelA > 0 && labelB > 0 && areNeighborsSimilar(idxA, idxB))
            {
              auto uniteResult = equivalences->unite(static_cast<uint64>(labelA), static_cast<uint64>(labelB), m_ShouldCancel);
              if(uniteResult.invalid())
              {
                return uniteResult;
              }
              hasNonContiguousFeature = true;
            }
          }
        }
      }
    }
    else
    {
      // Complete connectivity can wrap across one, two, or three axes. Process
      // one current Z slice and one optional wrapped partner slice.
      // Keep first and last labels available for both Z-boundary passes.
      std::vector<int32> featureIdsSlice0(sliceSize, 0);
      std::vector<int32> featureIdsSliceLast(sliceSize, 0);
      auto firstReadResult = featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(featureIdsSlice0.data(), sliceSize));
      if(firstReadResult.invalid())
      {
        return firstReadResult;
      }
      if(dimZ > 1)
      {
        auto lastReadResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(dimZ - 1) * sliceSize, nonstd::span<int32>(featureIdsSliceLast.data(), sliceSize));
        if(lastReadResult.invalid())
        {
          return lastReadResult;
        }
      }

      for(int64 iz = 0; iz < dimZ; iz++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }

        // Reuse boundary buffers and read each interior slice when needed.
        if(iz == 0)
        {
          std::copy(featureIdsSlice0.begin(), featureIdsSlice0.end(), featureIdsSliceCur.begin());
        }
        else if(iz == dimZ - 1)
        {
          std::copy(featureIdsSliceLast.begin(), featureIdsSliceLast.end(), featureIdsSliceCur.begin());
        }
        else
        {
          auto readResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(iz) * sliceSize, nonstd::span<int32>(featureIdsSliceCur.data(), sliceSize));
          if(readResult.invalid())
          {
            return readResult;
          }
        }

        auto prepareResult = prepareForSlice(iz, dimX, dimY, dimZ);
        if(prepareResult.invalid())
        {
          return prepareResult;
        }

        // Only first and last Z slices need a wrapped partner.
        int64 wrappedPartnerZ = -1; // sentinel: no Z-wrapped partner
        const int32* wrappedSlicePtr = nullptr;
        if(iz == 0 && dimZ > 1)
        {
          wrappedPartnerZ = dimZ - 1;
          wrappedSlicePtr = featureIdsSliceLast.data();
          // Prepare the wrapped partner for cross-slice comparisons.
          auto wrappedPrepareResult = prepareForSlice(wrappedPartnerZ, dimX, dimY, dimZ);
          if(wrappedPrepareResult.invalid())
          {
            return wrappedPrepareResult;
          }
        }
        else if(iz == dimZ - 1 && dimZ > 1)
        {
          wrappedPartnerZ = 0;
          wrappedSlicePtr = featureIdsSlice0.data();
          auto wrappedPrepareResult = prepareForSlice(wrappedPartnerZ, dimX, dimY, dimZ);
          if(wrappedPrepareResult.invalid())
          {
            return wrappedPrepareResult;
          }
        }

        for(int64 iy = 0; iy < dimY; iy++)
        {
          for(int64 ix = 0; ix < dimX; ix++)
          {
            // Only boundary voxels can have wrapped neighbors.
            const bool onBoundary = (ix == 0 || ix == dimX - 1 || iy == 0 || iy == dimY - 1 || iz == 0 || iz == dimZ - 1);
            if(!onBoundary)
            {
              continue;
            }

            const usize inSlice = static_cast<usize>(iy * dimX + ix);
            const int64 index = iz * sliceStride + iy * dimX + ix;
            const int32 labelCurrent = featureIdsSliceCur[inSlice];
            if(labelCurrent <= 0)
            {
              continue;
            }

            for(int64 dz = -1; dz <= 1; ++dz)
            {
              int64 nz = iz + dz;
              bool wrappedZ = false;
              if(nz < 0)
              {
                nz += dimZ;
                wrappedZ = true;
              }
              else if(nz >= dimZ)
              {
                nz -= dimZ;
                wrappedZ = true;
              }

              for(int64 dy = -1; dy <= 1; ++dy)
              {
                int64 ny = iy + dy;
                bool wrappedY = false;
                if(ny < 0)
                {
                  ny += dimY;
                  wrappedY = true;
                }
                else if(ny >= dimY)
                {
                  ny -= dimY;
                  wrappedY = true;
                }

                for(int64 dx = -1; dx <= 1; ++dx)
                {
                  if(dx == 0 && dy == 0 && dz == 0)
                  {
                    continue;
                  }

                  int64 nx = ix + dx;
                  bool wrappedX = false;
                  if(nx < 0)
                  {
                    nx += dimX;
                    wrappedX = true;
                  }
                  else if(nx >= dimX)
                  {
                    nx -= dimX;
                    wrappedX = true;
                  }

                  // The forward scan already processed every nonwrapped pair.
                  if(!wrappedX && !wrappedY && !wrappedZ)
                  {
                    continue;
                  }

                  const int64 neighIdx = nz * sliceStride + ny * dimX + nx;
                  // Process each symmetric pair only from its smaller flat index.
                  if(neighIdx <= index)
                  {
                    continue;
                  }

                  // Select the current or wrapped label slice from the neighbor Z index.
                  int32 labelNeigh = 0;
                  if(nz == iz)
                  {
                    labelNeigh = featureIdsSliceCur[static_cast<usize>(ny * dimX + nx)];
                  }
                  else if(nz == wrappedPartnerZ && wrappedSlicePtr != nullptr)
                  {
                    labelNeigh = wrappedSlicePtr[static_cast<usize>(ny * dimX + nx)];
                  }

                  if(labelNeigh > 0 && areNeighborsSimilar(index, neighIdx))
                  {
                    auto uniteResult = equivalences->unite(static_cast<uint64>(labelCurrent), static_cast<uint64>(labelNeigh), m_ShouldCancel);
                    if(uniteResult.invalid())
                    {
                      return uniteResult;
                    }
                    hasNonContiguousFeature = true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if(hasNonContiguousFeature)
  {
    m_MessageHelper.sendMessage("Non-contiguous Features were found: at least one Feature wraps across a periodic boundary.");
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Resolve roots and write dense final IDs in one slice-sequential pass.
  // First voxel appearance determines final Feature-ID order.
  auto prepareFinalLabelsResult = equivalences->prepareFinalLabels(static_cast<uint64>(nextLabel));
  if(prepareFinalLabelsResult.invalid())
  {
    return prepareFinalLabelsResult;
  }
  int32 finalFeatureCount = 0;

  std::vector<int32> sliceData(sliceSize);

  for(int64 iz = 0; iz < dimZ; iz++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    auto readResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(iz) * sliceSize, nonstd::span<int32>(sliceData.data(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(int64 iy = 0; iy < dimY; iy++)
    {
      for(int64 ix = 0; ix < dimX; ix++)
      {
        const usize inSlice = static_cast<usize>(iy * dimX + ix);
        int32 label = sliceData[inSlice];
        if(label > 0)
        {
          auto finalLabelResult = equivalences->resolveFinalLabel(static_cast<uint64>(label), finalFeatureCount, m_ShouldCancel);
          if(finalLabelResult.invalid())
          {
            return ConvertResult(std::move(finalLabelResult));
          }
          sliceData[inSlice] = finalLabelResult.value();
        }
      }
    }

    auto writeResult = featureIdsStore.copyFromBuffer(static_cast<usize>(iz) * sliceSize, nonstd::span<const int32>(sliceData.data(), sliceSize));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  auto flushResult = equivalences->flush(m_ShouldCancel);
  if(flushResult.invalid())
  {
    return flushResult;
  }

  m_FoundFeatures = finalFeatureCount;
  m_MessageHelper.sendMessage(fmt::format("Total Features Found: {}", m_FoundFeatures));

  return {};
}

Result<> SegmentFeatures::prepareForSlice(int64 /*iz*/, int64 /*dimX*/, int64 /*dimY*/, int64 /*dimZ*/)
{
  return {};
}

bool SegmentFeatures::isValidVoxel(int64 point) const
{
  return true;
}

bool SegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  return false;
}

void SegmentFeatures::randomizeFeatureIds(nx::core::Int32Array* featureIds, uint64 totalFeatures)
{
  m_MessageHelper.sendMessage("Randomizing Feature Ids");
  ClusterUtilities::RandomizeFeatureIds(featureIds->getDataStoreRef(), totalFeatures);
}
