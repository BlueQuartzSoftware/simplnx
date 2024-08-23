#include "CombineAttributeArrays.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

/**
 * @brief The CombineAttributeArraysImpl class is a templated private implementation that deals with
 * combining the various input arrays into one contiguous array
 */
namespace
{
struct CombineAttributeArraysImpl
{

  template <typename DataType>
  Result<> operator()(bool normalize, std::vector<DataObject*>& inputArraysVec, DataObject* outputArrayPtr)
  {
    using OutputArrayType = DataArray<DataType>;
    using InputArrayType = DataArray<DataType>;
    using InputDataStoreType = AbstractDataStore<DataType>;
    using OutputDataStoreType = AbstractDataStore<DataType>;

    OutputDataStoreType& outputDataStore = dynamic_cast<OutputArrayType*>(outputArrayPtr)->getDataStoreRef();
    usize numArrays = inputArraysVec.size();
    if(numArrays == 0)
    {
      return MakeWarningVoidResult(1, "No arrays were selected to combine.");
    }

    std::vector<InputArrayType*> inputArrays;

    for(usize i = 0; i < numArrays; i++)
    {
      inputArrays.push_back(dynamic_cast<InputArrayType*>(inputArraysVec[i]));
    }

    usize numTuples = inputArrays[0]->getNumberOfTuples();
    usize stackedDims = outputDataStore.getNumberOfComponents();
    usize arrayOffset = 0;
    usize numComps = 0;

    if(normalize)
    {
      std::vector<DataType> maxValues(stackedDims, std::numeric_limits<DataType>::lowest());
      std::vector<DataType> minValues(stackedDims, std::numeric_limits<DataType>::max());

      for(usize i = 0; i < numArrays; i++)
      {
        const InputDataStoreType& inputDataStore = inputArrays[i]->getDataStoreRef(); // Get a reference var to the current input array

        numComps = inputDataStore.getNumberOfComponents();
        if(i > 0)
        {
          arrayOffset += inputArrays[i - 1]->getNumberOfComponents();
        }
        for(usize j = 0; j < numTuples; j++)
        {
          for(usize k = 0; k < numComps; k++)
          {
            if(inputDataStore[numComps * j + k] > maxValues[arrayOffset + k])
            {
              maxValues[arrayOffset + k] = inputDataStore[numComps * j + k];
            }
            if(inputDataStore[numComps * j + k] < minValues[arrayOffset + k])
            {
              minValues[arrayOffset + k] = inputDataStore[numComps * j + k];
            }
          }
        }
      }

      arrayOffset = 0;

      for(usize i = 0; i < numTuples; i++)
      {
        for(usize j = 0; j < numArrays; j++)
        {
          const InputDataStoreType& inputDataStore = inputArrays[j]->getDataStoreRef(); // Get a reference var to the current input array

          numComps = inputDataStore.getNumberOfComponents();
          if(j > 0)
          {
            arrayOffset += inputArrays[j - 1]->getNumberOfComponents();
          }
          for(usize k = 0; k < numComps; k++)
          {
            if(maxValues[arrayOffset + k] == minValues[arrayOffset + k])
            {
              outputDataStore[stackedDims * i + (arrayOffset) + k] = static_cast<DataType>(0);
            }
            else
            {
              outputDataStore[stackedDims * i + (arrayOffset) + k] = (inputDataStore[numComps * i + k] - minValues[arrayOffset + k]) / (maxValues[arrayOffset + k] - minValues[arrayOffset + k]);
            }
          }
        }
        arrayOffset = 0;
      }
    }
    else
    {
      usize outputNumComps = outputDataStore.getNumberOfComponents();
      usize compsWritten = 0;
      for(const auto* inputArrayPtr : inputArrays)
      {
        const InputDataStoreType& inputDataStore = inputArrayPtr->getDataStoreRef(); // Get a reference var to the current input array
        usize numInputComps = inputDataStore.getNumberOfComponents();

        for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
        {
          for(usize compIndex = 0; compIndex < numInputComps; compIndex++)
          {
            outputDataStore[tupleIndex * outputNumComps + compsWritten + compIndex] = inputDataStore[tupleIndex * numInputComps + compIndex];
          }
        }
        compsWritten += numInputComps;
      }
    }
    return {};
  }
};

} // namespace

// -----------------------------------------------------------------------------
CombineAttributeArrays::CombineAttributeArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               CombineAttributeArraysInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CombineAttributeArrays::~CombineAttributeArrays() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineAttributeArrays::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
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

  return ExecuteDataFunction(CombineAttributeArraysImpl{}, outputArray.getDataType(), m_InputValues->NormalizeData, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath));
}
