#include "ReshapeDataArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ReshapeDataArray::ReshapeDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReshapeDataArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReshapeDataArray::~ReshapeDataArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReshapeDataArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReshapeDataArray::operator()()
{
  auto outputArrayPath = m_InputValues->InputArrayPath.getParent().createChildPath(fmt::format(".{}", m_InputValues->InputArrayPath.getTargetName()));
  auto& outputArray = m_DataStructure.getDataRefAs<IArray>(outputArrayPath);

  std::string arrayTypeName = outputArray.getTypeName();
  switch(outputArray.getArrayType())
  {
  case IArray::ArrayType::DataArray: {
    return ReshapeArray(m_DataStructure, m_InputValues->InputArrayPath, outputArrayPath, m_MessageHandler);
  }
  case IArray::ArrayType::StringArray: {
    return ReshapeArrayImpl<StringArray>(m_DataStructure, m_InputValues->InputArrayPath, outputArrayPath, m_MessageHandler);
  }
  case IArray::ArrayType::NeighborListArray: {
    return ReshapeNeighborList(m_DataStructure, m_InputValues->InputArrayPath, outputArrayPath, m_MessageHandler);
  }
  case IArray::ArrayType::Any: {
    return MakeErrorResult(to_underlying(ReshapeDataArray::ErrorCodes::InputArrayEqualsAny),
                           "Every array in the input arrays list has array type 'Any'.  This SHOULD NOT be possible, so please contact the developers.");
  }
  default: {
    return MakeErrorResult(to_underlying(ReshapeDataArray::ErrorCodes::InputArrayUnsupported),
                           "Every array in the input arrays list has array type '{}'.  This is an array type that is currently not supported by this filter, so please contact the developers.");
  }
  }
}
