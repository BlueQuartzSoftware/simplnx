#include "CombineAttributeArrays.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

/**
 * @brief The CombineAttributeArraysImpl class is a templated private implementation that deals with
 * combining the various input arrays into one contiguous array
 */
template <typename DataType>
class CombineAttributeArraysImpl
{
private:
  const CombineAttributeArraysInputValues* m_InputValues = nullptr;
  const std::vector<DataObject*> m_InputArrays;
  DataObject* m_OutputArray = nullptr;

public:
  typedef DataArray<DataType> DataArrayType;

  CombineAttributeArraysImpl(const CombineAttributeArraysInputValues* inputValues, std::vector<DataObject*>& inputArrays, DataObject* outputArray)
  : m_InputValues(inputValues)
  , m_InputArrays(inputArrays)
  , m_OutputArray(outputArray)
  {
  }

  virtual ~CombineAttributeArraysImpl() = default;

  // -----------------------------------------------------------------------------
  Result<> operator()()
  {
    using OutputArrayType = DataArray<DataType>;
    using InputArrayType = DataArray<DataType>;

    auto& outputArray = dynamic_cast<OutputArrayType&>(*m_OutputArray);
    int32_t numArrays = m_InputArrays.size();

    std::vector<InputArrayType*> inputArrays;

    for(size_t i = 0; i < numArrays; i++)
    {
      inputArrays.push_back(dynamic_cast<InputArrayType*>(m_InputArrays[i]));
    }

    size_t numTuples = inputArrays[0]->getNumberOfTuples();
    size_t stackedDims = outputArray.getNumberOfComponents();
    size_t arrayOffset = 0;
    size_t numComps = 0;

    if(m_InputValues->NormalizeData)
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

public:
  CombineAttributeArraysImpl(const CombineAttributeArraysImpl&) = delete; // Copy Constructor Not Implemented
  void operator=(const CombineAttributeArraysImpl&) = delete;             // Move assignment Not Implemented
};

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
  auto dataType = outputArray.getDataType();
  switch(dataType)
  {
  case DataType::int8: {
    return CombineAttributeArraysImpl<int8>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::int16: {
    return CombineAttributeArraysImpl<int16>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::int32: {
    return CombineAttributeArraysImpl<int32>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::int64: {
    return CombineAttributeArraysImpl<int64>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::uint8: {
    return CombineAttributeArraysImpl<uint8>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::uint16: {
    return CombineAttributeArraysImpl<uint16>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::uint32: {
    return CombineAttributeArraysImpl<uint32>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::uint64: {
    return CombineAttributeArraysImpl<uint64>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::float32: {
    return CombineAttributeArraysImpl<float32>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::float64: {
    return CombineAttributeArraysImpl<float64>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  case DataType::boolean: {
    return CombineAttributeArraysImpl<bool>(m_InputValues, inputArrays, m_DataStructure.getData(m_InputValues->StackedDataArrayPath))();
  }
  }

  return {};
}
