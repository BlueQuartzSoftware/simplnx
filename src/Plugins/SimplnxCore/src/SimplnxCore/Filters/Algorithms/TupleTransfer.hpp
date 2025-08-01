#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"

namespace nx::core
{
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
   * @param secondcIndex
   * @param forceSecondToZero
   */
  virtual void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, bool forceSecondToZero) = 0;

  virtual void transfer(size_t faceIndex, size_t firstcIndex) = 0;

  virtual void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) = 0;

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
   * @brief This method does the actual copying of the Tuple values from one DataArray to the other DataArray
   * @param faceIndex
   * @param firstcIndex
   * @param secondcIndex
   * @param forceSecondToZero
   */
  void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, bool forceSecondToZero) override
  {
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex * m_NumComps + i] = m_CellRef[firstcIndex * m_NumComps + i];
    }

    if(!forceSecondToZero)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex + i + m_NumComps] = m_CellRef[secondcIndex + i];
      }
    }
  }

  void transfer(size_t faceIndex, size_t firstcIndex) override
  {
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex + i] = m_CellRef[firstcIndex + i];
    }
  }

  void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) override
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
   * @brief This method does the actual copying of the Tuple values from one DataArray to the other DataArray
   * @param faceIndex
   * @param firstcIndex
   * @param secondcIndex
   * @param forceSecondToZero
   */
  void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, bool forceSecondToZero) override
  {
    // FeatureIds is assumed to be an Int32 array with a single component.
    K firstFeatureId = m_FeatureIdsRef[firstcIndex];
    K secondFeatureId = m_FeatureIdsRef[secondcIndex];

    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex * m_NumComps + i] = m_FeatureDataRef[firstFeatureId * m_NumComps + i];
    }

    if(!forceSecondToZero)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex + i + m_NumComps] = m_FeatureDataRef[secondFeatureId + i];
      }
    }
  }

  void transfer(size_t faceIndex, size_t firstcIndex) override
  {
    // FeatureIds is assumed to be an Int32 array with a single component.
    K firstFeatureId = m_FeatureIdsRef[firstcIndex];
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex + i] = m_FeatureDataRef[firstFeatureId + i];
    }
  }

  void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) override
  {
    // FeatureIds is assumed to be an Int32 array with a single component.
    K firstFeatureId = m_FeatureIdsRef[firstcIndex];
    K secondFeatureId = m_FeatureIdsRef[secondcIndex];

    // Only copy the data if the FaceLabel is NOT -1, indicating that the data is NOT on the exterior
    if(faceLabels[faceIndex * 2] != -1)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_FeatureDataRef[firstFeatureId * m_NumComps + i];
      }
    }

    if(faceLabels[faceIndex * 2 + 1] != -1)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_FeatureDataRef[secondFeatureId * m_NumComps + i];
      }
    }
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
