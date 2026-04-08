#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <array>
#include <nonstd/span.hpp>

namespace nx::core
{
struct QuickSurfaceTransferData
{
  usize faceIndex = 0;
  usize firstcIndex = 0;
  usize secondcIndex = 0;
  int32 faceLabel0 = 0;
  int32 faceLabel1 = 0;
};

struct SurfaceNetsTransferData
{
  usize faceIndex = 0;
  std::array<usize, 2> quadNxArrayIndices = {std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max()};
};

/**
 * @brief This is the base class that is used to transfer cell data to triangle face data
 * but could be used generally to copy the tuple value from one Data Array to another
 * DataArray of the same type.
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
   * @brief
   * @param faceIndex
   * @param firstcIndex
   */
  virtual void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) = 0;

  /**
   * @brief
   * @param faceIndex
   * @param firstcIndex
   * @param secondcIndex
   * @param faceLabels
   */
  virtual void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) = 0;

  /**
   * @brief
   * @param faceIndex
   * @param quadNxArrayIndices
   */
  virtual void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) = 0;

  virtual void quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> /*records*/)
  {
  }

  virtual void surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> /*records*/)
  {
  }

protected:
  AbstractTupleTransfer() = default;

  DataPath m_SourceDataPath;
  DataPath m_DestinationDataPath;
  size_t m_NumComps = 0;
};

template <typename T>
class TransferTuple : public AbstractTupleTransfer
{
public:
  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  /**
   * @brief
   * @param dataStructure Current DataStructure
   * @param selectedDataPath The source data path
   * @param createdArrayPath The destination data path
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

  void quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> records) override
  {
    if(records.empty())
    {
      return;
    }

    // Find source cell index range and face index range
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
      return; // all exterior faces, nothing to copy from source
    }

    // Bulk read source cell data
    usize srcTupleCount = maxSrc - minSrc + 1;
    auto srcBuf = std::make_unique<T[]>(srcTupleCount * m_NumComps);
    m_CellRef.copyIntoBuffer(minSrc * m_NumComps, nonstd::span<T>(srcBuf.get(), srcTupleCount * m_NumComps));

    // Build destination buffer
    usize faceCount = maxFace - minFace + 1;
    auto destBuf = std::make_unique<T[]>(faceCount * m_NumComps * 2);
    std::fill_n(destBuf.get(), faceCount * m_NumComps * 2, T{});

    // Process all records using local buffers
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

    // Bulk write destination face data
    m_FaceRef.copyFromBuffer(minFace * m_NumComps * 2, nonstd::span<const T>(destBuf.get(), faceCount * m_NumComps * 2));
  }


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
  DataStoreType& m_CellRef;
  DataStoreType& m_FaceRef;
};

template <typename T, typename K>
class TransferFeatureTuple : public AbstractTupleTransfer
{
public:
  using DataArrayType = DataArray<T>;
  using FeatureIdsArrayType = DataArray<K>;
  using DataStoreType = AbstractDataStore<T>;
  using FeatureIdsStoreType = AbstractDataStore<K>;

  /**
   * @brief
   * @param dataStructure Current DataStructure
   * @param selectedDataPath The source data path
   * @param createdArrayPath The destination data path
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

  void quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> records) override
  {
    if(records.empty())
    {
      return;
    }

    // Find cell index range and face index range
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

    // Bulk read featureIds for the cell range
    usize srcTupleCount = maxSrc - minSrc + 1;
    auto featureIdBuf = std::make_unique<K[]>(srcTupleCount);
    m_FeatureIdsRef.copyIntoBuffer(minSrc, nonstd::span<K>(featureIdBuf.get(), srcTupleCount));

    // Feature data is small (feature-level, not cell-level) — cache it all
    usize featureTuples = m_FeatureDataRef.getNumberOfTuples();
    auto featureDataBuf = std::make_unique<T[]>(featureTuples * m_NumComps);
    m_FeatureDataRef.copyIntoBuffer(0, nonstd::span<T>(featureDataBuf.get(), featureTuples * m_NumComps));

    // Build destination buffer
    usize faceCount = maxFace - minFace + 1;
    auto destBuf = std::make_unique<T[]>(faceCount * m_NumComps * 2);
    std::fill_n(destBuf.get(), faceCount * m_NumComps * 2, T{});

    // Process records
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

    // Bulk write destination
    m_FaceRef.copyFromBuffer(minFace * m_NumComps * 2, nonstd::span<const T>(destBuf.get(), faceCount * m_NumComps * 2));
  }


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
  DataStoreType& m_FeatureDataRef;
  DataStoreType& m_FaceRef;
  FeatureIdsStoreType& m_FeatureIdsRef;
};

/**
 *
 * @param dataStructure
 * @param selectedDataPath
 * @param createdDataPath
 * @param tupleTransferFunctions
 */
SIMPLNXCORE_EXPORT void AddTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath,
                                                 std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

/**
 *
 * @param dataStructure
 * @param selectedDataPath
 * @param createdDataPath
 * @param featureIdsArrayPath
 * @param tupleTransferFunctions
 */
SIMPLNXCORE_EXPORT void AddFeatureTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath, const DataPath& featureIdsArrayPath,
                                                        std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

} // namespace nx::core
