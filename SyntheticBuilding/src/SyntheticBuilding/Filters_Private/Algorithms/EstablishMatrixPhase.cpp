#include "EstablishMatrixPhase.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
EstablishMatrixPhase::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EstablishMatrixPhaseInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EstablishMatrixPhase::~EstablishMatrixPhase() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& EstablishMatrixPhase::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> EstablishMatrixPhase::operator()()
{

  return {};
}
