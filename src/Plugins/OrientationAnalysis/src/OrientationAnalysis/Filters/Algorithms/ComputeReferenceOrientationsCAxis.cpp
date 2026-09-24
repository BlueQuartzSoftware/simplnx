#include "ComputeReferenceOrientationsCAxis.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeReferenceOrientationsCAxis::ComputeReferenceOrientationsCAxis(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                     ComputeReferenceOrientationsCAxisInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeReferenceOrientationsCAxis::~ComputeReferenceOrientationsCAxis() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeReferenceOrientationsCAxis::operator()()
{
  return {};
}
