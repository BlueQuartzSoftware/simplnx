#include "UncertainRegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
UncertainRegularGridSampleSurfaceMesh::UncertainRegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             UncertainRegularGridSampleSurfaceMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
UncertainRegularGridSampleSurfaceMesh::~UncertainRegularGridSampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& UncertainRegularGridSampleSurfaceMesh::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> UncertainRegularGridSampleSurfaceMesh::operator()()
{

  return {};
}
