#include "EBSDSegmentFeatures.hpp"


#include <chrono>


using namespace complex;

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::~EBSDSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> EBSDSegmentFeatures::operator()()
{


  return {};
}
