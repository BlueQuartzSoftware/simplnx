#include "CombineAttributeArrays.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
/// Bounds transient storage while keeping bulk transfers large enough to amortize OOC I/O.
constexpr usize k_ChunkValues = 65536;

/**
 * @struct CombineAttributeArraysImpl
 * @brief Combines one runtime-selected DataArray type through bounded bulk datastore transfers.
 */
struct CombineAttributeArraysImpl
{
  template <typename DataType>
  Result<> operator()(bool normalize, const std::vector<DataObject*>& inputArraysVec, DataObject* outputArrayPtr, const std::atomic_bool& shouldCancel)
  {
    using OutputArrayType = DataArray<DataType>;
    using InputArrayType = DataArray<DataType>;
    using InputDataStoreType = AbstractDataStore<DataType>;
    using OutputDataStoreType = AbstractDataStore<DataType>;

    OutputDataStoreType& outputDataStore = dynamic_cast<OutputArrayType*>(outputArrayPtr)->getDataStoreRef();
    const usize numArrays = inputArraysVec.size();
    if(numArrays == 0)
    {
      return MakeWarningVoidResult(1, "No arrays were selected to combine.");
    }

    std::vector<InputArrayType*> inputArrays;
    inputArrays.reserve(numArrays);
    std::vector<usize> componentCounts;
    componentCounts.reserve(numArrays);
    std::vector<usize> componentOffsets;
    componentOffsets.reserve(numArrays);

    usize maxInputComponents = 0;
    usize componentOffset = 0;
    for(DataObject* inputArrayObject : inputArraysVec)
    {
      auto* inputArray = dynamic_cast<InputArrayType*>(inputArrayObject);
      const usize componentCount = inputArray->getNumberOfComponents();
      inputArrays.push_back(inputArray);
      componentCounts.push_back(componentCount);
      componentOffsets.push_back(componentOffset);
      componentOffset += componentCount;
      maxInputComponents = std::max(maxInputComponents, componentCount);
    }

    const usize numTuples = inputArrays[0]->getNumberOfTuples();
    const usize stackedDims = outputDataStore.getNumberOfComponents();
    const usize tuplesPerChunk = std::max<usize>(1, k_ChunkValues / std::max<usize>(1, stackedDims));
    auto inputBuffer = std::make_unique<DataType[]>(tuplesPerChunk * maxInputComponents);
    auto outputBuffer = std::make_unique<DataType[]>(tuplesPerChunk * stackedDims);

    std::unique_ptr<DataType[]> maxValues;
    std::unique_ptr<DataType[]> minValues;

    if(normalize)
    {
      maxValues = std::make_unique<DataType[]>(stackedDims);
      minValues = std::make_unique<DataType[]>(stackedDims);
      std::fill_n(maxValues.get(), stackedDims, std::numeric_limits<DataType>::lowest());
      std::fill_n(minValues.get(), stackedDims, std::numeric_limits<DataType>::max());

      for(usize i = 0; i < numArrays; i++)
      {
        const InputDataStoreType& inputDataStore = inputArrays[i]->getDataStoreRef();
        const usize numComps = componentCounts[i];
        const usize arrayOffset = componentOffsets[i];
        for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += tuplesPerChunk)
        {
          if(shouldCancel)
          {
            return {};
          }

          const usize tupleCount = std::min(tuplesPerChunk, numTuples - tupleOffset);
          auto readResult = inputDataStore.copyIntoBuffer(tupleOffset * numComps, nonstd::span<DataType>(inputBuffer.get(), tupleCount * numComps));
          if(readResult.invalid())
          {
            return readResult;
          }

          // Preserve tuple and component traversal order. NaN values do not
          // update either bound through these ordered comparisons.
          for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
          {
            for(usize compIndex = 0; compIndex < numComps; compIndex++)
            {
              const DataType value = inputBuffer[tupleIndex * numComps + compIndex];
              const usize outputCompIndex = arrayOffset + compIndex;
              if(value > maxValues[outputCompIndex])
              {
                maxValues[outputCompIndex] = value;
              }
              if(value < minValues[outputCompIndex])
              {
                minValues[outputCompIndex] = value;
              }
            }
          }
        }
      }
    }

    for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += tuplesPerChunk)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(tuplesPerChunk, numTuples - tupleOffset);
      for(usize arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
      {
        const InputDataStoreType& inputDataStore = inputArrays[arrayIndex]->getDataStoreRef();
        const usize numComps = componentCounts[arrayIndex];
        const usize arrayOffset = componentOffsets[arrayIndex];
        auto readResult = inputDataStore.copyIntoBuffer(tupleOffset * numComps, nonstd::span<DataType>(inputBuffer.get(), tupleCount * numComps));
        if(readResult.invalid())
        {
          return readResult;
        }

        for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
        {
          for(usize compIndex = 0; compIndex < numComps; compIndex++)
          {
            const usize outputCompIndex = arrayOffset + compIndex;
            const DataType value = inputBuffer[tupleIndex * numComps + compIndex];
            if(normalize)
            {
              if(maxValues[outputCompIndex] == minValues[outputCompIndex])
              {
                outputBuffer[stackedDims * tupleIndex + outputCompIndex] = static_cast<DataType>(0);
              }
              else
              {
                outputBuffer[stackedDims * tupleIndex + outputCompIndex] = (value - minValues[outputCompIndex]) / (maxValues[outputCompIndex] - minValues[outputCompIndex]);
              }
            }
            else
            {
              outputBuffer[stackedDims * tupleIndex + outputCompIndex] = value;
            }
          }
        }
      }

      auto writeResult = outputDataStore.copyFromBuffer(tupleOffset * stackedDims, nonstd::span<const DataType>(outputBuffer.get(), tupleCount * stackedDims));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }

    return {};
  }
};

} // namespace

CombineAttributeArrays::CombineAttributeArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               CombineAttributeArraysInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

CombineAttributeArrays::~CombineAttributeArrays() noexcept = default;

const std::atomic_bool& CombineAttributeArrays::getCancel()
{
  return m_ShouldCancel;
}

Result<> CombineAttributeArrays::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  std::vector<DataObject*> inputArrays;
  for(const auto& dataPath : m_InputValues->SelectedDataArrayPaths)
  {
    inputArrays.push_back(m_DataStructure.getData(dataPath));
  }

  auto& outputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->StackedDataArrayPath);

  return ExecuteDataFunction(CombineAttributeArraysImpl{}, outputArray.getDataType(), m_InputValues->NormalizeData, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath),
                             m_ShouldCancel);
}
