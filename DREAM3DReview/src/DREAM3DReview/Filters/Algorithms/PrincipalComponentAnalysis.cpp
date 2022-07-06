#include "PrincipalComponentAnalysis.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
PrincipalComponentAnalysis::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        PrincipalComponentAnalysisInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
PrincipalComponentAnalysis::~PrincipalComponentAnalysis() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& PrincipalComponentAnalysis::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> PrincipalComponentAnalysis::operator()()
{

  return {};
}
