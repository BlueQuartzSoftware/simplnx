#include "InsertPrecipitatePhases.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
InsertPrecipitatePhases::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InsertPrecipitatePhasesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InsertPrecipitatePhases::~InsertPrecipitatePhases() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& InsertPrecipitatePhases::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InsertPrecipitatePhases::operator()()
{

  return {};
}
