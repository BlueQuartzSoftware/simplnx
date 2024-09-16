#include "ConcatenateDataArrays.hpp"

#include "simplnx/DataStructure/IArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ConcatenateDataArrays::ConcatenateDataArrays(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                             ConcatenateDataArraysInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
ConcatenateDataArrays::~ConcatenateDataArrays() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConcatenateDataArrays::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConcatenateDataArrays::operator()()
{
  const auto& outputDataArray = m_DataStructure.getDataRefAs<IArray>(m_InputValues->OutputArrayPath);
  switch(outputDataArray.getArrayType())
  {
  case IArray::ArrayType::DataArray: {
    return ConcatenateArrays(m_DataStructure, m_InputValues->InputArrayPaths, m_InputValues->OutputArrayPath, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::StringArray: {
    return ConcatenateArraysImpl<StringArray>(m_DataStructure, m_InputValues->InputArrayPaths, m_InputValues->OutputArrayPath, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::NeighborListArray: {
    return ConcatenateNeighborLists(m_DataStructure, m_InputValues->InputArrayPaths, m_InputValues->OutputArrayPath, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::Any:
    return MakeErrorResult(-3001, "The input arrays list has array type 'Any'.  Only array types 'DataArray', 'StringArray', and NeighborList' are allowed.");
  }
}
