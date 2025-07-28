#include "CreateDataGroup.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreateDataGroup::CreateDataGroup(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataGroupInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateDataGroup::~CreateDataGroup() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateDataGroup::operator()()
{

  return {};
}
