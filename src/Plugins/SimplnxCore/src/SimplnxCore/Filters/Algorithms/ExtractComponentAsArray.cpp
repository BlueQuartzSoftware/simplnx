#include "ExtractComponentAsArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;
namespace
{
// Keep OOC scratch independent of the selected array's tuple count.
constexpr usize k_TargetChunkValues = 65536;

/**
 * @brief Transfers selected and remaining components through fixed-value chunks.
 * @tparam T Array value type.
 * @param inputArray Supplies complete tuples.
 * @param extractedArray Receives the selected scalar, or is null.
 * @param reducedArray Receives remaining components, or is null.
 * @param componentIndex Selects the source component.
 * @param shouldCancel Signals cancellation between chunks.
 * @return Success, or the first bulk-transfer error.
 */
template <typename T>
Result<> TransferComponentsInChunks(const IDataArray& inputArray, IDataArray* extractedArray, IDataArray* reducedArray, usize componentIndex, const std::atomic_bool& shouldCancel)
{
  using StoreType = AbstractDataStore<T>;

  const auto& inputStore = inputArray.template getIDataStoreRefAs<StoreType>();
  const usize numTuples = inputStore.getNumberOfTuples();
  const usize numComponents = inputStore.getNumberOfComponents();
  if(numTuples == 0 || shouldCancel)
  {
    return {};
  }

  StoreType* extractedStorePtr = nullptr;
  if(extractedArray != nullptr)
  {
    extractedStorePtr = &extractedArray->template getIDataStoreRefAs<StoreType>();
  }

  StoreType* reducedStorePtr = nullptr;
  if(reducedArray != nullptr)
  {
    reducedStorePtr = &reducedArray->template getIDataStoreRefAs<StoreType>();
  }

  const usize reducedComponents = numComponents - 1;
  const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComponents);
  auto inputBuffer = std::make_unique<T[]>(chunkTuples * numComponents);
  std::unique_ptr<T[]> extractedBuffer;
  std::unique_ptr<T[]> reducedBuffer;
  if(extractedStorePtr != nullptr)
  {
    extractedBuffer = std::make_unique<T[]>(chunkTuples);
  }
  if(reducedStorePtr != nullptr)
  {
    reducedBuffer = std::make_unique<T[]>(chunkTuples * reducedComponents);
  }

  const usize totalChunks = ((numTuples - 1) / chunkTuples) + 1;
  for(usize chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize tupleOffset = chunkIndex * chunkTuples;
    const usize tupleCount = std::min(chunkTuples, numTuples - tupleOffset);
    const usize inputValueCount = tupleCount * numComponents;
    Result<> result = inputStore.copyIntoBuffer(tupleOffset * numComponents, nonstd::span<T>(inputBuffer.get(), inputValueCount));
    if(result.invalid())
    {
      return result;
    }

    for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
    {
      const T* inputTuplePtr = inputBuffer.get() + tupleIndex * numComponents;
      if(extractedStorePtr != nullptr)
      {
        extractedBuffer[tupleIndex] = inputTuplePtr[componentIndex];
      }
      if(reducedStorePtr != nullptr)
      {
        T* reducedTuplePtr = reducedBuffer.get() + tupleIndex * reducedComponents;
        std::copy_n(inputTuplePtr, componentIndex, reducedTuplePtr);
        std::copy_n(inputTuplePtr + componentIndex + 1, reducedComponents - componentIndex, reducedTuplePtr + componentIndex);
      }
    }

    if(extractedStorePtr != nullptr)
    {
      result = extractedStorePtr->copyFromBuffer(tupleOffset, nonstd::span<const T>(extractedBuffer.get(), tupleCount));
      if(result.invalid())
      {
        return result;
      }
    }
    if(reducedStorePtr != nullptr)
    {
      result = reducedStorePtr->copyFromBuffer(tupleOffset * reducedComponents, nonstd::span<const T>(reducedBuffer.get(), tupleCount * reducedComponents));
      if(result.invalid())
      {
        return result;
      }
    }
  }

  return {};
}

/**
 * @brief Transfers components with contiguous access when all stores permit it.
 * @tparam T Array value type.
 * @param inputArray Supplies complete tuples.
 * @param extractedArray Receives the selected scalar, or is null.
 * @param reducedArray Receives remaining components, or is null.
 * @param componentIndex Selects the source component.
 * @param shouldCancel Signals cancellation between chunks.
 * @return Success, or a fallback bulk-transfer error.
 *
 * A non-contiguous participant routes the complete operation to the chunked
 * implementation. This avoids mixing direct and abstract access.
 */
template <typename T>
Result<> TransferComponentsDirect(const IDataArray& inputArray, IDataArray* extractedArray, IDataArray* reducedArray, usize componentIndex, const std::atomic_bool& shouldCancel)
{
  const auto& inputStore = inputArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto* contiguousInputStorePtr = dynamic_cast<const DataStore<T>*>(&inputStore);
  if(contiguousInputStorePtr == nullptr)
  {
    return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
  }

  DataStore<T>* contiguousExtractedStorePtr = nullptr;
  if(extractedArray != nullptr)
  {
    contiguousExtractedStorePtr = dynamic_cast<DataStore<T>*>(&extractedArray->template getIDataStoreRefAs<AbstractDataStore<T>>());
    if(contiguousExtractedStorePtr == nullptr)
    {
      return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
    }
  }

  DataStore<T>* contiguousReducedStorePtr = nullptr;
  if(reducedArray != nullptr)
  {
    contiguousReducedStorePtr = dynamic_cast<DataStore<T>*>(&reducedArray->template getIDataStoreRefAs<AbstractDataStore<T>>());
    if(contiguousReducedStorePtr == nullptr)
    {
      return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
    }
  }

  const usize numTuples = contiguousInputStorePtr->getNumberOfTuples();
  const usize numComponents = contiguousInputStorePtr->getNumberOfComponents();
  if(numTuples == 0 || shouldCancel)
  {
    return {};
  }

  const usize reducedComponents = numComponents - 1;
  const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComponents);
  const usize totalChunks = ((numTuples - 1) / chunkTuples) + 1;
  const T* inputDataPtr = contiguousInputStorePtr->data();
  T* extractedDataPtr = contiguousExtractedStorePtr == nullptr ? nullptr : contiguousExtractedStorePtr->data();
  T* reducedDataPtr = contiguousReducedStorePtr == nullptr ? nullptr : contiguousReducedStorePtr->data();

  for(usize chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize tupleOffset = chunkIndex * chunkTuples;
    const usize tupleCount = std::min(chunkTuples, numTuples - tupleOffset);
    for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
    {
      const usize inputOffset = (tupleOffset + tupleIndex) * numComponents;
      if(extractedDataPtr != nullptr)
      {
        extractedDataPtr[tupleOffset + tupleIndex] = inputDataPtr[inputOffset + componentIndex];
      }
      if(reducedDataPtr != nullptr)
      {
        T* reducedTuplePtr = reducedDataPtr + (tupleOffset + tupleIndex) * reducedComponents;
        std::copy_n(inputDataPtr + inputOffset, componentIndex, reducedTuplePtr);
        std::copy_n(inputDataPtr + inputOffset + componentIndex + 1, reducedComponents - componentIndex, reducedTuplePtr + componentIndex);
      }
    }
  }

  return {};
}

/**
 * @struct TransferComponentsDirectFunctor
 * @brief Dispatches the preferred direct implementation by runtime value type.
 */
struct TransferComponentsDirectFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray& inputArray, IDataArray* extractedArray, IDataArray* reducedArray, usize componentIndex, const std::atomic_bool& shouldCancel) const
  {
    return TransferComponentsDirect<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
  }
};

/**
 * @struct TransferComponentsScanlineFunctor
 * @brief Dispatches the bounded implementation by runtime value type.
 */
struct TransferComponentsScanlineFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray& inputArray, IDataArray* extractedArray, IDataArray* reducedArray, usize componentIndex, const std::atomic_bool& shouldCancel) const
  {
    return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
  }
};

/**
 * @class ExtractComponentAsArrayDirect
 * @brief Resolves arrays and requests the preferred direct transfer.
 */
class ExtractComponentAsArrayDirect
{
public:
  ExtractComponentAsArrayDirect(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const ExtractComponentAsArrayInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  Result<> operator()() const
  {
    const bool removingComponents = m_InputValues->RemoveComponentsFromArray || !m_InputValues->MoveComponentsToNewArray;
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(removingComponents ? m_InputValues->TempArrayPath : m_InputValues->BaseArrayPath);
    auto* extractedArrayPtr = m_InputValues->MoveComponentsToNewArray ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath) : nullptr;
    auto* reducedArrayPtr = removingComponents ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath) : nullptr;
    const usize componentIndex = static_cast<usize>(abs(m_InputValues->CompNumber));
    return ExecuteDataFunction(TransferComponentsDirectFunctor{}, inputArray.getDataType(), inputArray, extractedArrayPtr, reducedArrayPtr, componentIndex, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const ExtractComponentAsArrayInputValues* m_InputValues = nullptr;
};

/**
 * @class ExtractComponentAsArrayScanline
 * @brief Resolves arrays and requests bounded bulk transfer.
 */
class ExtractComponentAsArrayScanline
{
public:
  ExtractComponentAsArrayScanline(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const ExtractComponentAsArrayInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  Result<> operator()() const
  {
    const bool removingComponents = m_InputValues->RemoveComponentsFromArray || !m_InputValues->MoveComponentsToNewArray;
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(removingComponents ? m_InputValues->TempArrayPath : m_InputValues->BaseArrayPath);
    auto* extractedArrayPtr = m_InputValues->MoveComponentsToNewArray ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath) : nullptr;
    auto* reducedArrayPtr = removingComponents ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath) : nullptr;
    const usize componentIndex = static_cast<usize>(abs(m_InputValues->CompNumber));
    return ExecuteDataFunction(TransferComponentsScanlineFunctor{}, inputArray.getDataType(), inputArray, extractedArrayPtr, reducedArrayPtr, componentIndex, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const ExtractComponentAsArrayInputValues* m_InputValues = nullptr;
};
} // namespace

// -----------------------------------------------------------------------------
ExtractComponentAsArray::ExtractComponentAsArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ExtractComponentAsArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractComponentAsArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractComponentAsArray::operator()()
{
  const bool removingComponents = m_InputValues->RemoveComponentsFromArray || !m_InputValues->MoveComponentsToNewArray;
  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(removingComponents ? m_InputValues->TempArrayPath : m_InputValues->BaseArrayPath);
  const auto* extractedArrayPtr = m_InputValues->MoveComponentsToNewArray ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath) : nullptr;
  const auto* reducedArrayPtr = removingComponents ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath) : nullptr;
  return DispatchAlgorithm<ExtractComponentAsArrayDirect, ExtractComponentAsArrayScanline>({&inputArray, extractedArrayPtr, reducedArrayPtr}, m_DataStructure, m_ShouldCancel, m_InputValues);
}
