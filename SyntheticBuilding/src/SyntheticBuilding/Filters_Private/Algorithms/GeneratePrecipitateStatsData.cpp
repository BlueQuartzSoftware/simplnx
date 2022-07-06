#include "GeneratePrecipitateStatsData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GeneratePrecipitateStatsData::GeneratePrecipitateStatsData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           GeneratePrecipitateStatsDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GeneratePrecipitateStatsData::~GeneratePrecipitateStatsData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GeneratePrecipitateStatsData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GeneratePrecipitateStatsData::operator()()
{

  return {};
}
