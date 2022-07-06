#include "IdentifyDislocationSegments.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
IdentifyDislocationSegments::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         IdentifyDislocationSegmentsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifyDislocationSegments::~IdentifyDislocationSegments() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& IdentifyDislocationSegments::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> IdentifyDislocationSegments::operator()()
{

  return {};
}
