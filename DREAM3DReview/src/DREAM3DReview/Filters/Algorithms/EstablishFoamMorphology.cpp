#include "EstablishFoamMorphology.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
EstablishFoamMorphology::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EstablishFoamMorphologyInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EstablishFoamMorphology::~EstablishFoamMorphology() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& EstablishFoamMorphology::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> EstablishFoamMorphology::operator()()
{

  return {};
}
