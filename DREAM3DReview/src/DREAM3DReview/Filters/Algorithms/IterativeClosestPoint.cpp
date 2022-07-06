#include "IterativeClosestPoint.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
IterativeClosestPoint::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IterativeClosestPointInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IterativeClosestPoint::~IterativeClosestPoint() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& IterativeClosestPoint::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> IterativeClosestPoint::operator()()
{

  return {};
}
