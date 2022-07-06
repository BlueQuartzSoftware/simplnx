#include "ExtractTripleLinesFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ExtractTripleLinesFromTriangleGeometry::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                    ExtractTripleLinesFromTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractTripleLinesFromTriangleGeometry::~ExtractTripleLinesFromTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractTripleLinesFromTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractTripleLinesFromTriangleGeometry::operator()()
{

  return {};
}
