#include "VMFindDistanceFieldFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
VMFindDistanceFieldFromTriangleGeometry::VMFindDistanceFieldFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                 VMFindDistanceFieldFromTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
VMFindDistanceFieldFromTriangleGeometry::~VMFindDistanceFieldFromTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& VMFindDistanceFieldFromTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VMFindDistanceFieldFromTriangleGeometry::operator()()
{

  return {};
}
