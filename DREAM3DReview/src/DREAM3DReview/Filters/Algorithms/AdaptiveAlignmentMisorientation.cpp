#include "AdaptiveAlignmentMisorientation.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AdaptiveAlignmentMisorientation::AdaptiveAlignmentMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 AdaptiveAlignmentMisorientationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AdaptiveAlignmentMisorientation::~AdaptiveAlignmentMisorientation() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AdaptiveAlignmentMisorientation::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AdaptiveAlignmentMisorientation::operator()()
{

  return {};
}
