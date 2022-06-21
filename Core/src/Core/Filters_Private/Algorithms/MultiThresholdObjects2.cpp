#include "MultiThresholdObjects2.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
MultiThresholdObjects2::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjects2InputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MultiThresholdObjects2::~MultiThresholdObjects2() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MultiThresholdObjects2::getCancel()
{
  return m_ShouldCancel;
}


// -----------------------------------------------------------------------------
Result<> MultiThresholdObjects2::operator()()
{


  return {};
}
