#include "CreateDataArrayAdvanced.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreateDataArrayAdvanced::CreateDataArrayAdvanced(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 CreateDataArrayAdvancedInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateDataArrayAdvanced::~CreateDataArrayAdvanced() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateDataArrayAdvanced::operator()()
{

  return {};
}
