#include "VisualizeGBCDGMT.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
VisualizeGBCDGMT::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VisualizeGBCDGMTInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
VisualizeGBCDGMT::~VisualizeGBCDGMT() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& VisualizeGBCDGMT::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VisualizeGBCDGMT::operator()()
{

  return {};
}
