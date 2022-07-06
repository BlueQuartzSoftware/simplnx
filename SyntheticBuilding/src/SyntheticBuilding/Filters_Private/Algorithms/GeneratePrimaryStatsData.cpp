#include "GeneratePrimaryStatsData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GeneratePrimaryStatsData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GeneratePrimaryStatsDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GeneratePrimaryStatsData::~GeneratePrimaryStatsData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GeneratePrimaryStatsData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GeneratePrimaryStatsData::operator()()
{

  return {};
}
