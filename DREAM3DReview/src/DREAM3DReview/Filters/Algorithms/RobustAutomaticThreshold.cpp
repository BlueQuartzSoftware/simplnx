#include "RobustAutomaticThreshold.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
RobustAutomaticThreshold::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RobustAutomaticThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RobustAutomaticThreshold::~RobustAutomaticThreshold() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RobustAutomaticThreshold::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RobustAutomaticThreshold::operator()()
{

  return {};
}
