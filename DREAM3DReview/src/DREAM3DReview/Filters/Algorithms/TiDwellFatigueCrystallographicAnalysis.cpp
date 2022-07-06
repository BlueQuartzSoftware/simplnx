#include "TiDwellFatigueCrystallographicAnalysis.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
TiDwellFatigueCrystallographicAnalysis::TiDwellFatigueCrystallographicAnalysis(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                               TiDwellFatigueCrystallographicAnalysisInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TiDwellFatigueCrystallographicAnalysis::~TiDwellFatigueCrystallographicAnalysis() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& TiDwellFatigueCrystallographicAnalysis::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> TiDwellFatigueCrystallographicAnalysis::operator()()
{

  return {};
}
