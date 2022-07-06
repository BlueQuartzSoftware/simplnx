#include "AdaptiveAlignmentMutualInformation.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AdaptiveAlignmentMutualInformation::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                AdaptiveAlignmentMutualInformationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AdaptiveAlignmentMutualInformation::~AdaptiveAlignmentMutualInformation() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AdaptiveAlignmentMutualInformation::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AdaptiveAlignmentMutualInformation::operator()()
{

  return {};
}
