#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"

namespace complex
{
/**
 * @brief This is the base class that is used to transfer cell data to triangle face data
 * but could be used generally to copy the tuple value from one Data Array to another
 * DataArray of the same type.
 */
class COMPLEXCORE_EXPORT AbstractTupleTransfer
{
public:
  ~AbstractTupleTransfer() = default;

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
  virtual void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, bool forceSecondToZero = false) = 0;

  virtual void transfer(size_t faceIndex, size_t firstcIndex) = 0;

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

  /**
   * @brief
   * @param dataStructure Current DataStructure
   * @param selectedDataPath The source data path
   * @param createdArrayPath The destination data path
   */
  TransferTuple(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdArrayPath)
  {
    m_SourceDataPath = selectedDataPath;
    m_DestinationDataPath = createdArrayPath;
    IDataArray* cellArray = dataStructure.template getDataAs<IDataArray>(m_SourceDataPath);
    IDataArray* faceArray = dataStructure.template getDataAs<IDataArray>(m_DestinationDataPath);

    m_CellPtr = dynamic_cast<DataArrayType*>(cellArray);
    m_FacePtr = dynamic_cast<DataArrayType*>(faceArray);

    m_NumComps = m_CellPtr->getNumberOfComponents();
  }

  ~TransferTuple() = default;
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
  void transfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, bool forceSecondToZero = false) override
  {
    for(size_t i = 0; i < m_NumComps; i++)
    {
      (*m_FacePtr)[faceIndex + i] = (*m_CellPtr)[firstcIndex + i];
    }

    if(!forceSecondToZero)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        (*m_FacePtr)[faceIndex + i + m_NumComps] = (*m_CellPtr)[secondcIndex + i];
      }
    }
  }

  void transfer(size_t faceIndex, size_t firstcIndex) override
  {
    for(size_t i = 0; i < m_NumComps; i++)
    {
      (*m_FacePtr)[faceIndex + i] = (*m_CellPtr)[firstcIndex + i];
    }
  }

private:
  DataArrayType* m_CellPtr = nullptr;
  DataArrayType* m_FacePtr = nullptr;
};

/**
 *
 * @param dataStructure
 * @param selectedDataPath
 * @param createdDataPath
 * @param tupleTransferFunctions
 */
COMPLEXCORE_EXPORT void AddTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath,
                                                 std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

} // namespace complex
