#include "EstablishShapeTypes.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
EstablishShapeTypes::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EstablishShapeTypesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EstablishShapeTypes::~EstablishShapeTypes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& EstablishShapeTypes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> EstablishShapeTypes::operator()()
{

  return {};
}
