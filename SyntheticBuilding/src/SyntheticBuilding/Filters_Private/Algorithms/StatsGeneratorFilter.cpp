#include "StatsGeneratorFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
StatsGeneratorFilter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, StatsGeneratorFilterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
StatsGeneratorFilter::~StatsGeneratorFilter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& StatsGeneratorFilter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> StatsGeneratorFilter::operator()()
{

  return {};
}
