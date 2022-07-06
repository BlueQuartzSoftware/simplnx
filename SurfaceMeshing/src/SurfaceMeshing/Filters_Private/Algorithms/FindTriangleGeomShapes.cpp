#include "FindTriangleGeomShapes.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindTriangleGeomShapes::FindTriangleGeomShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               FindTriangleGeomShapesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindTriangleGeomShapes::~FindTriangleGeomShapes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindTriangleGeomShapes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindTriangleGeomShapes::operator()()
{

  return {};
}
