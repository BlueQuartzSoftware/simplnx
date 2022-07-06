#include "PackPrimaryPhases.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
PackPrimaryPhases::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PackPrimaryPhasesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
PackPrimaryPhases::~PackPrimaryPhases() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& PackPrimaryPhases::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> PackPrimaryPhases::operator()()
{

  return {};
}
