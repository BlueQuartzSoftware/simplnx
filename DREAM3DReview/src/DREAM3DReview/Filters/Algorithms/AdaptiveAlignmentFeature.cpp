#include "AdaptiveAlignmentFeature.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AdaptiveAlignmentFeature::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AdaptiveAlignmentFeatureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AdaptiveAlignmentFeature::~AdaptiveAlignmentFeature() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AdaptiveAlignmentFeature::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AdaptiveAlignmentFeature::operator()()
{

  return {};
}
