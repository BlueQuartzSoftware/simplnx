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
  Result<> operator()(const CombineAttributeArraysInputValues* inputValues, std::vector<DataObject*>& inputArraysVec, DataObject* outputArrayPtr)
  {
    using OutputArrayType = DataArray<DataType>;
    using InputArrayType = DataArray<DataType>;

    auto& outputArray = dynamic_cast<OutputArrayType&>(*outputArrayPtr);
    int32_t numArrays = inputArraysVec.size();
    if(numArrays == 0)
    {
      return MakeWarningVoidResult(1, "No arrays were selected to combine.");
    }

    std::vector<InputArrayType*> inputArrays;

    for(size_t i = 0; i < numArrays; i++)
    {
      inputArrays.push_back(dynamic_cast<InputArrayType*>(inputArraysVec[i]));
    }

    size_t numTuples = inputArrays[0]->getNumberOfTuples();
    size_t stackedDims = outputArray.getNumberOfComponents();
    size_t arrayOffset = 0;
    size_t numComps = 0;

    if(inputValues->NormalizeData)
    {
      std::vector<DataType> maxValues(stackedDims, std::numeric_limits<DataType>::lowest());
      std::vector<DataType> minValues(stackedDims, std::numeric_limits<DataType>::max());

      for(size_t i = 0; i < numArrays; i++)
      {
        InputArrayType& inputArray = *inputArrays[i]; // Get a reference var to the current input array

        numComps = inputArray.getNumberOfComponents();
        if(i > 0)
        {
          arrayOffset += inputArrays[i - 1]->getNumberOfComponents();
        }
        for(size_t j = 0; j < numTuples; j++)
        {
          for(size_t k = 0; k < numComps; k++)
          {
            if(inputArray[numComps * j + k] > maxValues[arrayOffset + k])
            {
              maxValues[arrayOffset + k] = inputArray[numComps * j + k];
            }
            if(inputArray[numComps * j + k] < minValues[arrayOffset + k])
            {
              minValues[arrayOffset + k] = inputArray[numComps * j + k];
            }
          }
        }
      }

      arrayOffset = 0;

      for(size_t i = 0; i < numTuples; i++)
      {
        for(size_t j = 0; j < numArrays; j++)
        {
          InputArrayType& inputArray = *inputArrays[j]; // Get a reference var to the current input array

          numComps = inputArray.getNumberOfComponents();
          if(j > 0)
          {
            arrayOffset += inputArrays[j - 1]->getNumberOfComponents();
          }
          for(size_t k = 0; k < numComps; k++)
          {
            if(maxValues[arrayOffset + k] == minValues[arrayOffset + k])
            {
              outputArray[stackedDims * i + (arrayOffset) + k] = static_cast<DataType>(0);
            }
            else
            {
              outputArray[stackedDims * i + (arrayOffset) + k] = (inputArray[numComps * i + k] - minValues[arrayOffset + k]) / (maxValues[arrayOffset + k] - minValues[arrayOffset + k]);
            }
          }
        }
        arrayOffset = 0;
      }
    }
    else
    {
      size_t outputNumComps = outputArray.getNumberOfComponents();
      size_t compsWritten = 0;
      for(const auto* inputArrayPtr : inputArrays)
      {
        const InputArrayType& inputArray = *inputArrayPtr; // Get a reference var to the current input array
        size_t numInputComps = inputArray.getNumberOfComponents();

        for(size_t tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
        {
          for(size_t compIndex = 0; compIndex < numInputComps; compIndex++)
          {
            outputArray[tupleIndex * outputNumComps + compsWritten + compIndex] = inputArray[tupleIndex * numInputComps + compIndex];
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

  return ExecuteDataFunction(CombineAttributeArraysImpl{}, outputArray.getDataType(), m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath));
}
