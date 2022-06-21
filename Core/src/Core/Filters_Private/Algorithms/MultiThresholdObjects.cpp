#include "MultiThresholdObjects.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
MultiThresholdObjects::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MultiThresholdObjects::~MultiThresholdObjects() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MultiThresholdObjects::getCancel()
{
  return m_ShouldCancel;
}


// -----------------------------------------------------------------------------
Result<> MultiThresholdObjects::operator()()
{


  return {};
}
