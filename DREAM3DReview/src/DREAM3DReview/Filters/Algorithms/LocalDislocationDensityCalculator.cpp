#include "LocalDislocationDensityCalculator.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
LocalDislocationDensityCalculator::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               LocalDislocationDensityCalculatorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LocalDislocationDensityCalculator::~LocalDislocationDensityCalculator() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LocalDislocationDensityCalculator::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LocalDislocationDensityCalculator::operator()()
{

  return {};
}
