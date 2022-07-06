#include "ItkMultiOtsuThreshold.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ItkMultiOtsuThreshold::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkMultiOtsuThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ItkMultiOtsuThreshold::~ItkMultiOtsuThreshold() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ItkMultiOtsuThreshold::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ItkMultiOtsuThreshold::operator()()
{

  return {};
}
