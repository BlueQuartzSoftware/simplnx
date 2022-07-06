#include "FindTriangleGeomSizes.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindTriangleGeomSizes::FindTriangleGeomSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             FindTriangleGeomSizesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindTriangleGeomSizes::~FindTriangleGeomSizes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindTriangleGeomSizes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindTriangleGeomSizes::operator()()
{

  return {};
}
