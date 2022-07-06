#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ExtractInternalSurfacesFromTriangleGeometry::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ExtractInternalSurfacesFromTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractInternalSurfacesFromTriangleGeometry::~ExtractInternalSurfacesFromTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractInternalSurfacesFromTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometry::operator()()
{

  return {};
}
