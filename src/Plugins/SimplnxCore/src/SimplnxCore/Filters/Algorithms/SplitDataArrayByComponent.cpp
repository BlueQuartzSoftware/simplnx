#include "SplitDataArrayByComponent.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
// Target input values per transfer. One complete tuple can exceed this target.
constexpr usize k_TargetChunkValues = 65536;

/**
 * @brief Builds the output path for one selected component.
 * @param inputValues Supplies the source path and suffix.
 * @param component Source component index.
 * @return Source sibling path with suffix and component index appended.
 */
DataPath GetOutputArrayPath(const SplitDataArrayByComponentInputValues& inputValues, usize component)
{
  const std::string arrayName = inputValues.InputArrayPath.getTargetName() + inputValues.SplitArraysSuffix + StringUtilities::number(component);
  return inputValues.InputArrayPath.replaceName(arrayName);
}

/**
 * @brief Splits selected components through complete-tuple bulk buffers.
 * @tparam T Specifies the source and output value type.
 * @param dataStructure Contains input and output arrays.
 * @param inputIDataArray Supplies interleaved source tuples.
 * @param inputValues Selects suffix and component indexes.
 * @param shouldCancel Signals cancellation between chunks and component writes.
 * @return Input or output bulk-I/O result.
 * @pre Selected components and output paths are valid.
 *
 * One input buffer is reused for all selected components in a chunk. One scalar
 * output buffer is reused sequentially. Cancellation can leave earlier component
 * outputs ahead of later outputs for the same chunk.
 */
template <typename T>
Result<> SplitArraysInChunks(DataStructure& dataStructure, const IDataArray& inputIDataArray, const SplitDataArrayByComponentInputValues& inputValues, const std::atomic_bool& shouldCancel)
{
  using StoreType = AbstractDataStore<T>;

  const auto& inputStore = inputIDataArray.template getIDataStoreRefAs<StoreType>();
  const usize numTuples = inputStore.getNumberOfTuples();
  const usize numComponents = inputStore.getNumberOfComponents();
  if(numTuples == 0 || inputValues.ExtractComponents.empty())
  {
    return {};
  }

  std::vector<StoreType*> outputStores;
  outputStores.reserve(inputValues.ExtractComponents.size());
  for(const usize component : inputValues.ExtractComponents)
  {
    auto& outputArray = dataStructure.getDataRefAs<DataArray<T>>(GetOutputArrayPath(inputValues, component));
    outputStores.push_back(&outputArray.getDataStoreRef());
  }

  const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComponents);
  auto inputBuffer = std::make_unique<T[]>(chunkTuples * numComponents);
  auto outputBuffer = std::make_unique<T[]>(chunkTuples);

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

    for(usize outputIndex = 0; outputIndex < outputStores.size(); outputIndex++)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize component = inputValues.ExtractComponents[outputIndex];
      for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
      {
        outputBuffer[tupleIndex] = inputBuffer[tupleIndex * numComponents + component];
      }

      result = outputStores[outputIndex]->copyFromBuffer(tupleOffset, nonstd::span<const T>(outputBuffer.get(), tupleCount));
      if(result.invalid())
      {
        return result;
      }
    }
  }

  return {};
}

/**
 * @class SplitContiguousData
 * @brief Copies selected components through disjoint raw-pointer ranges.
 * @tparam T Specifies the source and output value type.
 */
template <typename T>
class SplitContiguousData
{
public:
  /**
   * @brief Initializes one parallel direct worker.
   * @param inputData Supplies interleaved source values.
   * @param outputData Supplies one scalar destination pointer per component.
   * @param components Supplies source component indexes.
   * @param numComponents Number of source components per tuple.
   * @param shouldCancel Signals cancellation between 65,536-tuple blocks.
   * @pre All pointers and spans remain valid through worker completion.
   */
  SplitContiguousData(const T* inputData, nonstd::span<T*> outputData, nonstd::span<const usize> components, usize numComponents, const std::atomic_bool& shouldCancel)
  : m_InputData(inputData)
  , m_OutputData(outputData)
  , m_Components(components)
  , m_NumComponents(numComponents)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Copies selected components for one tuple range.
   * @param range Half-open tuple range.
   */
  void operator()(const Range& range) const
  {
    usize blockStart = range.min();
    while(blockStart < range.max())
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize blockEnd = blockStart + std::min(k_TargetChunkValues, range.max() - blockStart);
      for(usize tupleIndex = blockStart; tupleIndex < blockEnd; tupleIndex++)
      {
        const usize inputOffset = tupleIndex * m_NumComponents;
        for(usize outputIndex = 0; outputIndex < m_OutputData.size(); outputIndex++)
        {
          m_OutputData[outputIndex][tupleIndex] = m_InputData[inputOffset + m_Components[outputIndex]];
        }
      }
      blockStart = blockEnd;
    }
  }

private:
  const T* m_InputData = nullptr;
  nonstd::span<T*> m_OutputData;
  nonstd::span<const usize> m_Components;
  usize m_NumComponents = 0;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @struct SplitArraysDirectFunctor
 * @brief Selects raw-pointer splitting or the checked bulk fallback.
 */
struct SplitArraysDirectFunctor
{
  /**
   * @brief Splits one runtime-selected value type.
   * @tparam T Specifies the source and output value type.
   * @param dataStructure Contains input and output arrays.
   * @param inputIDataArray Supplies source tuples.
   * @param inputValues Selects suffix and components.
   * @param shouldCancel Signals cancellation.
   * @return Bulk fallback result, or success after direct work.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputIDataArray, const SplitDataArrayByComponentInputValues& inputValues, const std::atomic_bool& shouldCancel) const
  {
    using StoreType = AbstractDataStore<T>;

    const auto& inputStore = inputIDataArray.template getIDataStoreRefAs<StoreType>();
    const auto* contiguousInputStore = dynamic_cast<const DataStore<T>*>(&inputStore);
    if(contiguousInputStore == nullptr)
    {
      return SplitArraysInChunks<T>(dataStructure, inputIDataArray, inputValues, shouldCancel);
    }

    std::vector<T*> outputData;
    outputData.reserve(inputValues.ExtractComponents.size());
    for(const usize component : inputValues.ExtractComponents)
    {
      auto& outputArray = dataStructure.getDataRefAs<DataArray<T>>(GetOutputArrayPath(inputValues, component));
      auto* contiguousOutputStore = dynamic_cast<DataStore<T>*>(&outputArray.getDataStoreRef());
      if(contiguousOutputStore == nullptr)
      {
        return SplitArraysInChunks<T>(dataStructure, inputIDataArray, inputValues, shouldCancel);
      }
      outputData.push_back(contiguousOutputStore->data());
    }

    const usize numTuples = contiguousInputStore->getNumberOfTuples();
    if(numTuples == 0 || outputData.empty() || shouldCancel)
    {
      return {};
    }

    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, numTuples);
    parallelAlgorithm.execute(SplitContiguousData<T>(contiguousInputStore->data(), nonstd::span<T*>(outputData.data(), outputData.size()),
                                                     nonstd::span<const usize>(inputValues.ExtractComponents.data(), inputValues.ExtractComponents.size()),
                                                     contiguousInputStore->getNumberOfComponents(), shouldCancel));
    if(shouldCancel)
    {
      return {};
    }
    return {};
  }
};

/**
 * @struct SplitArraysScanlineFunctor
 * @brief Adapts runtime value types to bulk component splitting.
 */
struct SplitArraysScanlineFunctor
{
  /**
   * @brief Splits one runtime-selected value type through bulk buffers.
   * @tparam T Specifies the source and output value type.
   * @param dataStructure Contains input and output arrays.
   * @param inputIDataArray Supplies source tuples.
   * @param inputValues Selects suffix and components.
   * @param shouldCancel Signals cancellation.
   * @return Input or output bulk-I/O result.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputIDataArray, const SplitDataArrayByComponentInputValues& inputValues, const std::atomic_bool& shouldCancel) const
  {
    return SplitArraysInChunks<T>(dataStructure, inputIDataArray, inputValues, shouldCancel);
  }
};

/**
 * @class SplitDataArrayByComponentDirect
 * @brief Dispatches the resident component-split implementation.
 */
class SplitDataArrayByComponentDirect
{
public:
  /**
   * @brief Initializes the resident dispatch target.
   * @param dataStructure Contains input and output arrays.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects source and components.
   * @pre All arguments outlive this target.
   */
  SplitDataArrayByComponentDirect(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const SplitDataArrayByComponentInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  /**
   * @brief Runs the typed resident split.
   * @return Typed split result.
   */
  Result<> operator()() const
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);
    return ExecuteDataFunction(SplitArraysDirectFunctor{}, inputArray.getDataType(), m_DataStructure, inputArray, *m_InputValues, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const SplitDataArrayByComponentInputValues* m_InputValues = nullptr;
};

/**
 * @class SplitDataArrayByComponentScanline
 * @brief Dispatches the bulk-I/O component-split implementation.
 */
class SplitDataArrayByComponentScanline
{
public:
  /**
   * @brief Initializes the bulk-I/O dispatch target.
   * @param dataStructure Contains input and output arrays.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects source and components.
   * @pre All arguments outlive this target.
   */
  SplitDataArrayByComponentScanline(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const SplitDataArrayByComponentInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  /**
   * @brief Runs the typed bulk split.
   * @return Typed split result.
   */
  Result<> operator()() const
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);
    return ExecuteDataFunction(SplitArraysScanlineFunctor{}, inputArray.getDataType(), m_DataStructure, inputArray, *m_InputValues, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const SplitDataArrayByComponentInputValues* m_InputValues = nullptr;
};
} // namespace

SplitDataArrayByComponent::SplitDataArrayByComponent(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                     SplitDataArrayByComponentInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

SplitDataArrayByComponent::~SplitDataArrayByComponent() noexcept = default;

const std::atomic_bool& SplitDataArrayByComponent::getCancel()
{
  return m_ShouldCancel;
}

Result<> SplitDataArrayByComponent::operator()()
{
  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);
  return DispatchAlgorithm<SplitDataArrayByComponentDirect, SplitDataArrayByComponentScanline>({&inputArray}, m_DataStructure, m_ShouldCancel, m_InputValues);
}
