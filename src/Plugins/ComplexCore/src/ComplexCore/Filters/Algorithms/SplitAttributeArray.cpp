#include "SplitAttributeArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

namespace
{
template <typename T>
void SplitArrays(DataStructure& dataStructure, const SplitAttributeArrayInputValues* inputValues)
{
  DataArray<T>& inputArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->InputArrayPath);
  usize numTuples = inputArray.getNumberOfTuples();
  usize numComponents = inputArray.getNumberOfComponents();
  for(const auto& j : inputValues->ExtractComponents)
  {
    std::string arrayName = inputValues->InputArrayPath.getTargetName() + inputValues->SplitArraysSuffix + StringUtilities::number(j);
    DataPath splitArrayPath = inputValues->InputArrayPath.getParent().createChildPath(arrayName);
    DataArray<T>& splitArray = dataStructure.getDataRefAs<DataArray<T>>(splitArrayPath);

    for(usize i = 0; i < numTuples; i++)
    {
      splitArray[i] = inputArray[numComponents * i + j];
    }
  }
}
} // namespace

// -----------------------------------------------------------------------------
SplitAttributeArray::SplitAttributeArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitAttributeArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SplitAttributeArray::~SplitAttributeArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SplitAttributeArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SplitAttributeArray::operator()()
{
  auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);
  switch(inputArray.getDataType())
  {
  case DataType::int8: {
    SplitArrays<int8>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::uint8: {
    SplitArrays<uint8>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::int16: {
    SplitArrays<int16>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::uint16: {
    SplitArrays<uint16>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::int32: {
    SplitArrays<int32>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::uint32: {
    SplitArrays<uint32>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::int64: {
    SplitArrays<int64>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::uint64: {
    SplitArrays<uint64>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::float32: {
    SplitArrays<float32>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::float64: {
    SplitArrays<float64>(m_DataStructure, m_InputValues);
    break;
  }
  case DataType::boolean: {
    SplitArrays<bool>(m_DataStructure, m_InputValues);
    break;
  }
  default: {
    return MakeErrorResult(-84601, fmt::format("Invalid DataType for data array at path '{}'", m_InputValues->InputArrayPath.toString()));
  }
  }

  return {};
}
