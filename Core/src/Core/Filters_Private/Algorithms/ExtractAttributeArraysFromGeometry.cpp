#include "ExtractAttributeArraysFromGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ExtractAttributeArraysFromGeometry::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                ExtractAttributeArraysFromGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractAttributeArraysFromGeometry::~ExtractAttributeArraysFromGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractAttributeArraysFromGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractAttributeArraysFromGeometry::operator()()
{

  return {};
}
