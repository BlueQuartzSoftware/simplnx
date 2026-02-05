#include "CreateDataArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreateDataArray::CreateDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateDataArray::~CreateDataArray() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateDataArray::operator()()
{

  return {};
}
