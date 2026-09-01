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

  StoreType* extractedStore = nullptr;
  if(extractedArray != nullptr)
  {
    extractedStore = &extractedArray->template getIDataStoreRefAs<StoreType>();
  }

  StoreType* reducedStore = nullptr;
  if(reducedArray != nullptr)
  {
    reducedStore = &reducedArray->template getIDataStoreRefAs<StoreType>();
  }

  const usize reducedComponents = numComponents - 1;
  const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComponents);
  auto inputBuffer = std::make_unique<T[]>(chunkTuples * numComponents);
  std::unique_ptr<T[]> extractedBuffer;
  std::unique_ptr<T[]> reducedBuffer;
  if(extractedStore != nullptr)
  {
    extractedBuffer = std::make_unique<T[]>(chunkTuples);
  }
  if(reducedStore != nullptr)
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
      const T* inputTuple = inputBuffer.get() + tupleIndex * numComponents;
      if(extractedStore != nullptr)
      {
        extractedBuffer[tupleIndex] = inputTuple[componentIndex];
      }
      if(reducedStore != nullptr)
      {
        T* reducedTuple = reducedBuffer.get() + tupleIndex * reducedComponents;
        std::copy_n(inputTuple, componentIndex, reducedTuple);
        std::copy_n(inputTuple + componentIndex + 1, reducedComponents - componentIndex, reducedTuple + componentIndex);
      }
    }

    if(extractedStore != nullptr)
    {
      result = extractedStore->copyFromBuffer(tupleOffset, nonstd::span<const T>(extractedBuffer.get(), tupleCount));
      if(result.invalid())
      {
        return result;
      }
    }
    if(reducedStore != nullptr)
    {
      result = reducedStore->copyFromBuffer(tupleOffset * reducedComponents, nonstd::span<const T>(reducedBuffer.get(), tupleCount * reducedComponents));
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
  const auto* contiguousInputStore = dynamic_cast<const DataStore<T>*>(&inputStore);
  if(contiguousInputStore == nullptr)
  {
    return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
  }

  DataStore<T>* contiguousExtractedStore = nullptr;
  if(extractedArray != nullptr)
  {
    contiguousExtractedStore = dynamic_cast<DataStore<T>*>(&extractedArray->template getIDataStoreRefAs<AbstractDataStore<T>>());
    if(contiguousExtractedStore == nullptr)
    {
      return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
    }
  }

  DataStore<T>* contiguousReducedStore = nullptr;
  if(reducedArray != nullptr)
  {
    contiguousReducedStore = dynamic_cast<DataStore<T>*>(&reducedArray->template getIDataStoreRefAs<AbstractDataStore<T>>());
    if(contiguousReducedStore == nullptr)
    {
      return TransferComponentsInChunks<T>(inputArray, extractedArray, reducedArray, componentIndex, shouldCancel);
    }
  }

  const usize numTuples = contiguousInputStore->getNumberOfTuples();
  const usize numComponents = contiguousInputStore->getNumberOfComponents();
  if(numTuples == 0 || shouldCancel)
  {
    return {};
  }

  const usize reducedComponents = numComponents - 1;
  const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComponents);
  const usize totalChunks = ((numTuples - 1) / chunkTuples) + 1;
  const T* inputData = contiguousInputStore->data();
  T* extractedData = contiguousExtractedStore == nullptr ? nullptr : contiguousExtractedStore->data();
  T* reducedData = contiguousReducedStore == nullptr ? nullptr : contiguousReducedStore->data();

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
      if(extractedData != nullptr)
      {
        extractedData[tupleOffset + tupleIndex] = inputData[inputOffset + componentIndex];
      }
      if(reducedData != nullptr)
      {
        T* reducedTuple = reducedData + (tupleOffset + tupleIndex) * reducedComponents;
        std::copy_n(inputData + inputOffset, componentIndex, reducedTuple);
        std::copy_n(inputData + inputOffset + componentIndex + 1, reducedComponents - componentIndex, reducedTuple + componentIndex);
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
    auto* extractedArray = m_InputValues->MoveComponentsToNewArray ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath) : nullptr;
    auto* reducedArray = removingComponents ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath) : nullptr;
    const usize componentIndex = static_cast<usize>(abs(m_InputValues->CompNumber));
    return ExecuteDataFunction(TransferComponentsDirectFunctor{}, inputArray.getDataType(), inputArray, extractedArray, reducedArray, componentIndex, m_ShouldCancel);
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
    auto* extractedArray = m_InputValues->MoveComponentsToNewArray ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath) : nullptr;
    auto* reducedArray = removingComponents ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath) : nullptr;
    const usize componentIndex = static_cast<usize>(abs(m_InputValues->CompNumber));
    return ExecuteDataFunction(TransferComponentsScanlineFunctor{}, inputArray.getDataType(), inputArray, extractedArray, reducedArray, componentIndex, m_ShouldCancel);
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
ExtractComponentAsArray::~ExtractComponentAsArray() noexcept = default;

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
  const auto* extractedArray = m_InputValues->MoveComponentsToNewArray ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath) : nullptr;
  const auto* reducedArray = removingComponents ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath) : nullptr;
  return DispatchAlgorithm<ExtractComponentAsArrayDirect, ExtractComponentAsArrayScanline>({&inputArray, extractedArray, reducedArray}, m_DataStructure, m_ShouldCancel, m_InputValues);
}
