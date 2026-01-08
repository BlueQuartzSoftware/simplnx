#include "RenameDataObject.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
RenameDataObject::RenameDataObject(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RenameDataObjectInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RenameDataObject::~RenameDataObject() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RenameDataObject::operator()()
{

  return {};
}
