#include "ChangeAngleRepresentation.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

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
const std::atomic_bool& ChangeAngleRepresentation::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ChangeAngleRepresentation::operator()()
{

  return {};
}
