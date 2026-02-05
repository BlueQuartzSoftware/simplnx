#include "RobustAutomaticThreshold.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
RobustAutomaticThreshold::RobustAutomaticThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   RobustAutomaticThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RobustAutomaticThreshold::~RobustAutomaticThreshold() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RobustAutomaticThreshold::operator()()
{

  return {};
}
