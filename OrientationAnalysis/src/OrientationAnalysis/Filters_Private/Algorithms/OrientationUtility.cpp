#include "OrientationUtility.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
OrientationUtility::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, OrientationUtilityInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
OrientationUtility::~OrientationUtility() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& OrientationUtility::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> OrientationUtility::operator()()
{

  return {};
}
