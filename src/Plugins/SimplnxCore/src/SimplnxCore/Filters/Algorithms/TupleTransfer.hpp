#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <array>
#include <nonstd/span.hpp>

namespace nx::core
{
/**
 * @brief Record holding all data needed to perform one QuickSurfaceMesh face transfer.
 *
 * Batching many of these records into a single quickSurfaceTransferBatch() call
 * allows the OOC-optimized implementation to compute the bounding range of source
 * cell indices and face indices, then perform just two bulk I/O operations
 * (one copyIntoBuffer for the source range, one copyFromBuffer for the destination
 * range) instead of 2*N per-element accesses.
 */
struct QuickSurfaceTransferData
{
  usize faceIndex = 0;    ///< Index of the triangle face in the destination (face-level) array.
  usize firstcIndex = 0;  ///< Cell index on side 0 of the face (used when faceLabel0 != -1).
  usize secondcIndex = 0; ///< Cell index on side 1 of the face (used when faceLabel1 != -1).
  int32 faceLabel0 = 0;   ///< Face label for side 0; -1 indicates exterior (skip copy).
  int32 faceLabel1 = 0;   ///< Face label for side 1; -1 indicates exterior (skip copy).
};

/**
 * @brief Record holding all data needed to perform one SurfaceNets face transfer.
 *
 * Similar to QuickSurfaceTransferData but uses the SurfaceNets convention where
 * each face has two associated NX-array indices (or max sentinel if exterior).
 */
struct SurfaceNetsTransferData
{
  usize faceIndex = 0; ///< Index of the quad face in the destination (face-level) array.
  /// Pair of NX-array cell indices for the two sides of the quad face.
  /// A value of std::numeric_limits<usize>::max() indicates an exterior face (skip).
  std::array<usize, 2> quadNxArrayIndices = {std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max()};
};

/**
 * @brief Abstract base class for transferring tuple data from cell-level DataArrays
 * to face-level DataArrays during surface mesh generation.
 *
 * @section overview Overview
 * When QuickSurfaceMesh or SurfaceNets generates a triangle/quad mesh from a
 * voxelized volume, each face sits between two cells. The face-level output
 * arrays need to store the data from both adjacent cells (stored interleaved:
 * [side0_comp0, side0_comp1, ..., side1_comp0, side1_comp1, ...]).
 *
 * @section ooc_optimization OOC Optimization: Batch Transfer Methods
 * The original per-element transfer methods (quickSurfaceTransfer, surfaceNetsTransfer)
 * use operator[] on the source and destination DataStore references. When the DataStore
 * is backed by OOC chunked storage, each operator[] call may trigger a chunk load/evict
 * cycle, making mesh generation extremely slow on large datasets.
 *
 * The batch methods (quickSurfaceTransferBatch, surfaceNetsTransferBatch) solve this by:
 *   1. Scanning all records to find the bounding range of source cell indices and
 *      destination face indices.
 *   2. Performing a single copyIntoBuffer() to bulk-read the source range into a
 *      local heap buffer.
 *   3. Performing all tuple copies in-memory against the local buffers.
 *   4. Performing a single copyFromBuffer() to bulk-write the destination range back.
 *
 * This reduces I/O from O(N) random accesses to O(1) sequential bulk operations per batch.
 * Callers (e.g., QuickSurfaceMesh) accumulate records for a batch of faces and flush
 * them periodically (every ~65K faces) to bound memory usage.
 */
class SIMPLNXCORE_EXPORT AbstractTupleTransfer
{
public:
  virtual ~AbstractTupleTransfer() = default;

  AbstractTupleTransfer(const AbstractTupleTransfer&) = delete;
  AbstractTupleTransfer(AbstractTupleTransfer&&) noexcept = delete;
  AbstractTupleTransfer& operator=(const AbstractTupleTransfer&) = delete;
  AbstractTupleTransfer& operator=(AbstractTupleTransfer&&) noexcept = delete;

  /**
   * @brief Transfers one tuple from a source cell to a destination face (point sampling).
   *
   * Used by PointSampleTriangleGeom. Copies m_NumComps values from cellRef[firstcIndex...]
   * to faceRef[faceIndex...].
   *
   * @param faceIndex Starting value index in the destination face array.
   * @param firstcIndex Starting value index in the source cell array.
   */
  virtual void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) = 0;

  /**
   * @brief Transfers cell data to both sides of a triangle face (per-element, non-batched).
   *
   * Copies cell data for side 0 and side 1 of a face, checking faceLabels to skip
   * exterior faces (label == -1). The destination layout is interleaved:
   * [face * numComps * 2 + 0..numComps-1] = side 0, [+ numComps..2*numComps-1] = side 1.
   *
   * @note This method uses per-element operator[] access. For OOC data, prefer
   *   accumulating QuickSurfaceTransferData records and calling quickSurfaceTransferBatch().
   *
   * @param faceIndex Index of the face.
   * @param firstcIndex Cell index for side 0 of the face.
   * @param secondcIndex Cell index for side 1 of the face.
   * @param faceLabels FaceLabels array; exterior faces have label -1.
   */
  virtual void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) = 0;

  /**
   * @brief Transfers cell data to both sides of a quad face for SurfaceNets (per-element).
   *
   * Same interleaved layout as quickSurfaceTransfer. Exterior sides are indicated
   * by quadNxArrayIndices[i] == std::numeric_limits<usize>::max().
   *
   * @note For OOC data, prefer accumulating SurfaceNetsTransferData records and
   *   calling surfaceNetsTransferBatch().
   *
   * @param faceIndex Index of the quad face.
   * @param quadNxArrayIndices Cell indices for the two sides of the quad; max sentinel = exterior.
   */
  virtual void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) = 0;

  /**
   * @brief OOC-optimized batch transfer for QuickSurfaceMesh faces.
   *
   * Processes a span of QuickSurfaceTransferData records in one bulk I/O round-trip.
   * Default implementation is a no-op; subclasses override with typed bulk copy logic.
   *
   * @param records Span of transfer records to process in one batch.
   */
  virtual void quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> /*records*/)
  {
  }

  /**
   * @brief OOC-optimized batch transfer for SurfaceNets faces.
   *
   * Processes a span of SurfaceNetsTransferData records in one bulk I/O round-trip.
   * Default implementation is a no-op; subclasses override with typed bulk copy logic.
   *
   * @param records Span of transfer records to process in one batch.
   */
  virtual void surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> /*records*/)
  {
  }

protected:
  AbstractTupleTransfer() = default;

  DataPath m_SourceDataPath;      ///< Path to the source (cell-level) DataArray in the DataStructure.
  DataPath m_DestinationDataPath; ///< Path to the destination (face-level) DataArray in the DataStructure.
  size_t m_NumComps = 0;          ///< Number of components per tuple in both source and destination arrays.
};

/**
 * @brief Typed implementation of AbstractTupleTransfer for direct cell-to-face data transfer.
 *
 * Copies data directly from cell-level DataArrays to face-level DataArrays.
 * The source array is indexed by cell index and the destination array stores two
 * sides per face in interleaved layout.
 *
 * @section ooc_batch OOC Batch Methods
 * The quickSurfaceTransferBatch() and surfaceNetsTransferBatch() overrides implement
 * the bulk I/O strategy described in AbstractTupleTransfer: they scan the batch of
 * records to find the [minSrc, maxSrc] cell index range and [minFace, maxFace] face
 * index range, perform a single copyIntoBuffer to read the source cell data, execute
 * all tuple copies in-memory, and perform a single copyFromBuffer to write the
 * destination face data. This converts O(N) random OOC accesses per batch into
 * exactly 2 bulk I/O operations.
 *
 * @tparam T The element type of both the source cell DataArray and destination face DataArray.
 */
template <typename T>
class TransferTuple : public AbstractTupleTransfer
{
public:
  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  /**
   * @brief Constructs a TransferTuple for direct cell-to-face data transfer.
   * @param dataStructure Current DataStructure containing both arrays.
   * @param selectedDataPath Path to the source (cell-level) DataArray.
   * @param createdArrayPath Path to the destination (face-level) DataArray.
   */
  TransferTuple(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdArrayPath)
  : m_CellRef(dataStructure.template getDataRefAs<DataArrayType>(selectedDataPath).getDataStoreRef())
  , m_FaceRef(dataStructure.template getDataRefAs<DataArrayType>(createdArrayPath).getDataStoreRef())
  {
    m_SourceDataPath = selectedDataPath;
    m_DestinationDataPath = createdArrayPath;

    IDataArray* cellArrayPtr = dataStructure.template getDataAs<IDataArray>(m_SourceDataPath);
    m_NumComps = cellArrayPtr->getNumberOfComponents();
  }

  ~TransferTuple() override = default;
  TransferTuple(const TransferTuple&) = delete;
  TransferTuple(TransferTuple&&) noexcept = delete;
  TransferTuple& operator=(const TransferTuple&) = delete;
  TransferTuple& operator=(TransferTuple&&) noexcept = delete;

  /**
   * @brief
   * @param faceIndex
   * @param firstcIndex
   */
  void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) override
  {
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex + i] = m_CellRef[firstcIndex + i];
    }
  }

  /**
   * @brief
   * @param faceIndex
   * @param firstcIndex
   * @param secondcIndex
   * @param faceLabels
   */
  void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) override
  {
    // Only copy the data if the FaceLabel is NOT -1, indicating that the data is NOT on the exterior
    if(faceLabels[faceIndex * 2] != -1)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_CellRef[firstcIndex * m_NumComps + i];
      }
    }

    if(faceLabels[faceIndex * 2 + 1] != -1)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_CellRef[secondcIndex * m_NumComps + i];
      }
    }
  }

  /**
   * @brief
   * @param faceIndex
   * @param quadNxArrayIndices
   */
  void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) override
  {
    // Only copy the data if the quadNxArrayIndices is NOT UINT64_MAX, indicating that the data is NOT on the exterior
    if(quadNxArrayIndices[0] != std::numeric_limits<usize>::max())
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_CellRef[quadNxArrayIndices[0] * m_NumComps + i];
      }
    }

    if(quadNxArrayIndices[1] != std::numeric_limits<usize>::max())
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_CellRef[quadNxArrayIndices[1] * m_NumComps + i];
      }
    }
  }

  /**
   * @brief OOC-optimized batch transfer for QuickSurfaceMesh.
   *
   * Replaces N per-element operator[] calls with 2 bulk I/O operations:
   *   - 1x copyIntoBuffer: reads contiguous source cell range [minSrc..maxSrc]
   *   - 1x copyFromBuffer: writes contiguous destination face range [minFace..maxFace]
   *
   * All tuple copies happen in-memory between heap-allocated local buffers.
   * Exterior faces (faceLabel == -1) are skipped during the copy phase.
   *
   * @param records Span of QuickSurfaceTransferData records for this batch.
   */
  void quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> records) override
  {
    if(records.empty())
    {
      return;
    }

    // Phase 1: Scan records to find the bounding range of source cell indices and face indices.
    // This determines the minimum contiguous region we need to bulk-read/write.
    usize minSrc = std::numeric_limits<usize>::max();
    usize maxSrc = 0;
    usize minFace = std::numeric_limits<usize>::max();
    usize maxFace = 0;
    for(const auto& r : records)
    {
      if(r.faceLabel0 != -1)
      {
        minSrc = std::min(minSrc, r.firstcIndex);
        maxSrc = std::max(maxSrc, r.firstcIndex);
      }
      if(r.faceLabel1 != -1)
      {
        minSrc = std::min(minSrc, r.secondcIndex);
        maxSrc = std::max(maxSrc, r.secondcIndex);
      }
      minFace = std::min(minFace, r.faceIndex);
      maxFace = std::max(maxFace, r.faceIndex);
    }

    if(minSrc > maxSrc)
    {
      return; // All exterior faces in this batch; nothing to copy from source
    }

    // Phase 2: Bulk-read the source cell data range into a local buffer.
    // This is the key OOC optimization -- one I/O call instead of N element reads.
    usize srcTupleCount = maxSrc - minSrc + 1;
    auto srcBuf = std::make_unique<T[]>(srcTupleCount * m_NumComps);
    m_CellRef.copyIntoBuffer(minSrc * m_NumComps, nonstd::span<T>(srcBuf.get(), srcTupleCount * m_NumComps));

    // Allocate destination buffer for the face range (2 sides per face, each with m_NumComps)
    usize faceCount = maxFace - minFace + 1;
    auto destBuf = std::make_unique<T[]>(faceCount * m_NumComps * 2);
    std::fill_n(destBuf.get(), faceCount * m_NumComps * 2, T{});

    // Phase 3: Process all records using local buffers (pure in-memory copies)
    for(const auto& r : records)
    {
      usize localFace = r.faceIndex - minFace;
      if(r.faceLabel0 != -1)
      {
        usize srcOff = (r.firstcIndex - minSrc) * m_NumComps;
        usize destOff = localFace * m_NumComps * 2;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = srcBuf[srcOff + c];
        }
      }
      if(r.faceLabel1 != -1)
      {
        usize srcOff = (r.secondcIndex - minSrc) * m_NumComps;
        usize destOff = localFace * m_NumComps * 2 + m_NumComps;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = srcBuf[srcOff + c];
        }
      }
    }

    // Phase 4: Bulk-write the destination face data back.
    m_FaceRef.copyFromBuffer(minFace * m_NumComps * 2, nonstd::span<const T>(destBuf.get(), faceCount * m_NumComps * 2));
  }

  /**
   * @brief OOC-optimized batch transfer for SurfaceNets.
   *
   * Same bulk I/O strategy as quickSurfaceTransferBatch but using SurfaceNets
   * conventions: exterior sides are indicated by quadNxArrayIndices[i] == max sentinel.
   *
   * @param records Span of SurfaceNetsTransferData records for this batch.
   */
  void surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> records) override
  {
    if(records.empty())
    {
      return;
    }

    constexpr usize k_MaxIdx = std::numeric_limits<usize>::max();

    usize minSrc = k_MaxIdx;
    usize maxSrc = 0;
    usize minFace = k_MaxIdx;
    usize maxFace = 0;
    for(const auto& r : records)
    {
      if(r.quadNxArrayIndices[0] != k_MaxIdx)
      {
        minSrc = std::min(minSrc, r.quadNxArrayIndices[0]);
        maxSrc = std::max(maxSrc, r.quadNxArrayIndices[0]);
      }
      if(r.quadNxArrayIndices[1] != k_MaxIdx)
      {
        minSrc = std::min(minSrc, r.quadNxArrayIndices[1]);
        maxSrc = std::max(maxSrc, r.quadNxArrayIndices[1]);
      }
      minFace = std::min(minFace, r.faceIndex);
      maxFace = std::max(maxFace, r.faceIndex);
    }

    if(minSrc > maxSrc)
    {
      return;
    }

    usize srcTupleCount = maxSrc - minSrc + 1;
    auto srcBuf = std::make_unique<T[]>(srcTupleCount * m_NumComps);
    m_CellRef.copyIntoBuffer(minSrc * m_NumComps, nonstd::span<T>(srcBuf.get(), srcTupleCount * m_NumComps));

    usize faceCount = maxFace - minFace + 1;
    auto destBuf = std::make_unique<T[]>(faceCount * m_NumComps * 2);
    std::fill_n(destBuf.get(), faceCount * m_NumComps * 2, T{});

    for(const auto& r : records)
    {
      usize localFace = r.faceIndex - minFace;
      if(r.quadNxArrayIndices[0] != k_MaxIdx)
      {
        usize srcOff = (r.quadNxArrayIndices[0] - minSrc) * m_NumComps;
        usize destOff = localFace * m_NumComps * 2;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = srcBuf[srcOff + c];
        }
      }
      if(r.quadNxArrayIndices[1] != k_MaxIdx)
      {
        usize srcOff = (r.quadNxArrayIndices[1] - minSrc) * m_NumComps;
        usize destOff = localFace * m_NumComps * 2 + m_NumComps;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = srcBuf[srcOff + c];
        }
      }
    }

    m_FaceRef.copyFromBuffer(minFace * m_NumComps * 2, nonstd::span<const T>(destBuf.get(), faceCount * m_NumComps * 2));
  }

private:
  DataStoreType& m_CellRef; ///< Reference to the source (cell-level) DataStore.
  DataStoreType& m_FaceRef; ///< Reference to the destination (face-level) DataStore.
};

/**
 * @brief Typed implementation of AbstractTupleTransfer for feature-level to face-level transfer.
 *
 * Unlike TransferTuple which copies cell data directly, this class performs an indirection
 * through a FeatureIds array: cell index -> featureId -> feature-level data -> face output.
 * This is used when surface mesh faces need feature-level attributes (e.g., average
 * orientations, average C-axis values) rather than raw cell-level data.
 *
 * @section ooc_batch OOC Batch Methods
 * The batch methods follow the same 4-phase pattern as TransferTuple but with an
 * additional data source:
 *   - Phase 1: Scan records for cell index range and face index range.
 *   - Phase 2: Bulk-read the FeatureIds for the cell range (1 copyIntoBuffer call).
 *   - Phase 3: Bulk-read the entire feature-level data array (typically small -- tens
 *     of thousands of features vs. millions of cells). This is cached entirely since
 *     feature IDs can map to any feature.
 *   - Phase 4: Process all records in-memory: look up featureId from the cell buffer,
 *     then copy the feature data into the face destination buffer.
 *   - Phase 5: Bulk-write the destination face data (1 copyFromBuffer call).
 *
 * Total OOC I/O: 3 bulk operations instead of 3*N per-element accesses.
 *
 * @tparam T The element type of the feature-level source and face-level destination DataArrays.
 * @tparam K The element type of the FeatureIds array (typically int32).
 */
template <typename T, typename K>
class TransferFeatureTuple : public AbstractTupleTransfer
{
public:
  using DataArrayType = DataArray<T>;
  using FeatureIdsArrayType = DataArray<K>;
  using DataStoreType = AbstractDataStore<T>;
  using FeatureIdsStoreType = AbstractDataStore<K>;

  /**
   * @brief Constructs a TransferFeatureTuple for feature-level to face-level transfer.
   * @param dataStructure Current DataStructure containing all arrays.
   * @param selectedDataPath Path to the source (feature-level) DataArray.
   * @param createdArrayPath Path to the destination (face-level) DataArray.
   * @param featureIdsArrayPath Path to the FeatureIds array that maps cell index -> feature ID.
   */
  TransferFeatureTuple(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdArrayPath, const DataPath& featureIdsArrayPath)
  : m_FeatureDataRef(dataStructure.template getDataRefAs<DataArrayType>(selectedDataPath).getDataStoreRef())
  , m_FaceRef(dataStructure.template getDataRefAs<DataArrayType>(createdArrayPath).getDataStoreRef())
  , m_FeatureIdsRef(dataStructure.template getDataRefAs<FeatureIdsArrayType>(featureIdsArrayPath).getDataStoreRef())
  {
    m_SourceDataPath = selectedDataPath;
    m_DestinationDataPath = createdArrayPath;

    IDataArray* cellArrayPtr = dataStructure.template getDataAs<IDataArray>(m_SourceDataPath);
    m_NumComps = cellArrayPtr->getNumberOfComponents();
  }

  ~TransferFeatureTuple() override = default;
  TransferFeatureTuple(const TransferFeatureTuple&) = delete;
  TransferFeatureTuple(TransferFeatureTuple&&) noexcept = delete;
  TransferFeatureTuple& operator=(const TransferFeatureTuple&) = delete;
  TransferFeatureTuple& operator=(TransferFeatureTuple&&) noexcept = delete;

  /**
   * @brief
   * @param faceIndex
   * @param firstcIndex
   */
  void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) override
  {
    // FeatureIds is assumed to be an Int32 array with a single component.
    K firstFeatureId = m_FeatureIdsRef[firstcIndex];
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex + i] = m_FeatureDataRef[firstFeatureId + i];
    }
  }

  /**
   * @brief
   * @param faceIndex
   * @param firstcIndex
   * @param secondcIndex
   * @param faceLabels
   */
  void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) override
  {
    // FeatureIds is assumed to be an Int32 array with a single component.
    // Only copy the data if the FaceLabel is NOT -1, indicating that the data is NOT on the exterior
    if(faceLabels[faceIndex * 2] != -1)
    {
      K firstFeatureId = m_FeatureIdsRef[firstcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_FeatureDataRef[firstFeatureId * m_NumComps + i];
      }
    }

    if(faceLabels[faceIndex * 2 + 1] != -1)
    {
      K secondFeatureId = m_FeatureIdsRef[secondcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_FeatureDataRef[secondFeatureId * m_NumComps + i];
      }
    }
  }

  /**
   * @brief
   * @param faceIndex
   * @param quadNxArrayIndices
   */
  void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) override
  {
    // FeatureIds is assumed to be an Int32 array with a single component.
    // Only copy the data if the quadNxArrayIndices is NOT UINT64_MAX, indicating that the data is NOT on the exterior
    if(quadNxArrayIndices[0] != std::numeric_limits<usize>::max())
    {
      usize firstcIndex = quadNxArrayIndices[0];
      K firstFeatureId = m_FeatureIdsRef[firstcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_FeatureDataRef[firstFeatureId * m_NumComps + i];
      }
    }

    if(quadNxArrayIndices[1] != std::numeric_limits<usize>::max())
    {
      usize secondcIndex = quadNxArrayIndices[1];
      K secondFeatureId = m_FeatureIdsRef[secondcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_FeatureDataRef[secondFeatureId * m_NumComps + i];
      }
    }
  }

  /**
   * @brief OOC-optimized batch transfer for QuickSurfaceMesh (feature-level variant).
   *
   * Unlike the TransferTuple version, this performs a two-level indirection:
   *   1. Bulk-read FeatureIds for the cell range (cell index -> feature ID).
   *   2. Bulk-read the entire feature-level data array (small, typically thousands of features).
   *   3. For each record: look up the feature ID, copy feature data into the face buffer.
   *   4. Bulk-write the face buffer.
   *
   * The feature data array is read in its entirety because feature IDs can be arbitrary
   * (not necessarily contiguous with the cell indices), and the array is small enough
   * that caching it all is more efficient than computing the feature ID range.
   *
   * @param records Span of QuickSurfaceTransferData records for this batch.
   */
  void quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> records) override
  {
    if(records.empty())
    {
      return;
    }

    // Phase 1: Scan records for bounding cell index range and face index range
    usize minSrc = std::numeric_limits<usize>::max();
    usize maxSrc = 0;
    usize minFace = std::numeric_limits<usize>::max();
    usize maxFace = 0;
    for(const auto& r : records)
    {
      if(r.faceLabel0 != -1)
      {
        minSrc = std::min(minSrc, r.firstcIndex);
        maxSrc = std::max(maxSrc, r.firstcIndex);
      }
      if(r.faceLabel1 != -1)
      {
        minSrc = std::min(minSrc, r.secondcIndex);
        maxSrc = std::max(maxSrc, r.secondcIndex);
      }
      minFace = std::min(minFace, r.faceIndex);
      maxFace = std::max(maxFace, r.faceIndex);
    }

    if(minSrc > maxSrc)
    {
      return;
    }

    // Phase 2: Bulk-read FeatureIds for the cell range (cell-level, contiguous range)
    usize srcTupleCount = maxSrc - minSrc + 1;
    auto featureIdBuf = std::make_unique<K[]>(srcTupleCount);
    m_FeatureIdsRef.copyIntoBuffer(minSrc, nonstd::span<K>(featureIdBuf.get(), srcTupleCount));

    // Phase 3: Cache the entire feature-level data array locally.
    // Feature arrays are small (one tuple per grain, typically thousands) so reading
    // the full array is both simpler and faster than computing the needed feature ID range.
    usize featureTuples = m_FeatureDataRef.getNumberOfTuples();
    auto featureDataBuf = std::make_unique<T[]>(featureTuples * m_NumComps);
    m_FeatureDataRef.copyIntoBuffer(0, nonstd::span<T>(featureDataBuf.get(), featureTuples * m_NumComps));

    // Allocate destination face buffer
    usize faceCount = maxFace - minFace + 1;
    auto destBuf = std::make_unique<T[]>(faceCount * m_NumComps * 2);
    std::fill_n(destBuf.get(), faceCount * m_NumComps * 2, T{});

    // Phase 4: Process records in-memory: cell index -> featureId -> feature data -> face buffer
    for(const auto& r : records)
    {
      usize localFace = r.faceIndex - minFace;
      if(r.faceLabel0 != -1)
      {
        K featureId = featureIdBuf[r.firstcIndex - minSrc];
        usize srcOff = featureId * m_NumComps;
        usize destOff = localFace * m_NumComps * 2;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = featureDataBuf[srcOff + c];
        }
      }
      if(r.faceLabel1 != -1)
      {
        K featureId = featureIdBuf[r.secondcIndex - minSrc];
        usize srcOff = featureId * m_NumComps;
        usize destOff = localFace * m_NumComps * 2 + m_NumComps;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = featureDataBuf[srcOff + c];
        }
      }
    }

    // Phase 5: Bulk-write the destination face data
    m_FaceRef.copyFromBuffer(minFace * m_NumComps * 2, nonstd::span<const T>(destBuf.get(), faceCount * m_NumComps * 2));
  }

  /**
   * @brief OOC-optimized batch transfer for SurfaceNets (feature-level variant).
   *
   * Same two-level indirection as quickSurfaceTransferBatch (cell -> featureId -> feature data)
   * but using SurfaceNets conventions for exterior-face detection.
   *
   * @param records Span of SurfaceNetsTransferData records for this batch.
   */
  void surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> records) override
  {
    if(records.empty())
    {
      return;
    }

    constexpr usize k_MaxIdx = std::numeric_limits<usize>::max();

    usize minSrc = k_MaxIdx;
    usize maxSrc = 0;
    usize minFace = k_MaxIdx;
    usize maxFace = 0;
    for(const auto& r : records)
    {
      if(r.quadNxArrayIndices[0] != k_MaxIdx)
      {
        minSrc = std::min(minSrc, r.quadNxArrayIndices[0]);
        maxSrc = std::max(maxSrc, r.quadNxArrayIndices[0]);
      }
      if(r.quadNxArrayIndices[1] != k_MaxIdx)
      {
        minSrc = std::min(minSrc, r.quadNxArrayIndices[1]);
        maxSrc = std::max(maxSrc, r.quadNxArrayIndices[1]);
      }
      minFace = std::min(minFace, r.faceIndex);
      maxFace = std::max(maxFace, r.faceIndex);
    }

    if(minSrc > maxSrc)
    {
      return;
    }

    usize srcTupleCount = maxSrc - minSrc + 1;
    auto featureIdBuf = std::make_unique<K[]>(srcTupleCount);
    m_FeatureIdsRef.copyIntoBuffer(minSrc, nonstd::span<K>(featureIdBuf.get(), srcTupleCount));

    usize featureTuples = m_FeatureDataRef.getNumberOfTuples();
    auto featureDataBuf = std::make_unique<T[]>(featureTuples * m_NumComps);
    m_FeatureDataRef.copyIntoBuffer(0, nonstd::span<T>(featureDataBuf.get(), featureTuples * m_NumComps));

    usize faceCount = maxFace - minFace + 1;
    auto destBuf = std::make_unique<T[]>(faceCount * m_NumComps * 2);
    std::fill_n(destBuf.get(), faceCount * m_NumComps * 2, T{});

    for(const auto& r : records)
    {
      usize localFace = r.faceIndex - minFace;
      if(r.quadNxArrayIndices[0] != k_MaxIdx)
      {
        K featureId = featureIdBuf[r.quadNxArrayIndices[0] - minSrc];
        usize srcOff = featureId * m_NumComps;
        usize destOff = localFace * m_NumComps * 2;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = featureDataBuf[srcOff + c];
        }
      }
      if(r.quadNxArrayIndices[1] != k_MaxIdx)
      {
        K featureId = featureIdBuf[r.quadNxArrayIndices[1] - minSrc];
        usize srcOff = featureId * m_NumComps;
        usize destOff = localFace * m_NumComps * 2 + m_NumComps;
        for(usize c = 0; c < m_NumComps; c++)
        {
          destBuf[destOff + c] = featureDataBuf[srcOff + c];
        }
      }
    }

    m_FaceRef.copyFromBuffer(minFace * m_NumComps * 2, nonstd::span<const T>(destBuf.get(), faceCount * m_NumComps * 2));
  }

private:
  DataStoreType& m_FeatureDataRef;      ///< Reference to the source (feature-level) DataStore.
  DataStoreType& m_FaceRef;             ///< Reference to the destination (face-level) DataStore.
  FeatureIdsStoreType& m_FeatureIdsRef; ///< Reference to the FeatureIds DataStore (cell index -> feature ID).
};

/**
 * @brief Factory function that creates a type-appropriate TransferTuple instance and appends it
 * to the provided vector of tuple transfer functions.
 *
 * Inspects the DataType of the selected DataArray and instantiates TransferTuple<T> with
 * the matching type. This is the primary way callers set up direct cell-to-face transfers.
 *
 * @param dataStructure Current DataStructure.
 * @param selectedDataPath Path to the source (cell-level) DataArray.
 * @param createdDataPath Path to the destination (face-level) DataArray.
 * @param tupleTransferFunctions Vector to append the new instance to.
 */
SIMPLNXCORE_EXPORT void AddTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath,
                                                 std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

/**
 * @brief Factory function that creates a type-appropriate TransferFeatureTuple instance and
 * appends it to the provided vector of tuple transfer functions.
 *
 * Inspects the DataType of the selected DataArray and the FeatureIds array, then
 * instantiates TransferFeatureTuple<T,K> with the matching types. This sets up
 * the two-level indirection path: cell -> featureId -> feature data -> face output.
 *
 * @param dataStructure Current DataStructure.
 * @param selectedDataPath Path to the source (feature-level) DataArray.
 * @param createdDataPath Path to the destination (face-level) DataArray.
 * @param featureIdsArrayPath Path to the FeatureIds array (cell index -> feature ID).
 * @param tupleTransferFunctions Vector to append the new instance to.
 */
SIMPLNXCORE_EXPORT void AddFeatureTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath, const DataPath& featureIdsArrayPath,
                                                        std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

} // namespace nx::core
