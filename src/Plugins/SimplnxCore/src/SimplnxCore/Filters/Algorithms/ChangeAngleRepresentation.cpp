#include "ChangeAngleRepresentation.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ChangeAngleRepresentation::ChangeAngleRepresentation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ChangeAngleRepresentationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ChangeAngleRepresentation::~ChangeAngleRepresentation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ChangeAngleRepresentation::operator()()
{

  return {};
}
