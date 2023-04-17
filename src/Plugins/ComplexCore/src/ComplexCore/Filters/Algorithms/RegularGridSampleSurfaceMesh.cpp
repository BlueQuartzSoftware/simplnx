#include "RegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RegularGridSampleSurfaceMeshInputValues* inputValues)
: SampleSurfaceMesh(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::~RegularGridSampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RegularGridSampleSurfaceMesh::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void RegularGridSampleSurfaceMesh::generatePoints(VertexGeom& vertexGeom)
{
  auto& points = vertexGeom.getVerticesRef();
  usize numComp = points.getNumberOfComponents();

  auto dims = m_InputValues->Dimensions;
  auto spacing = m_InputValues->Spacing;
  auto origin = m_InputValues->Origin;

  usize count = 0;
  for(int32_t k = 0; k < dims[2]; k++)
  {
    float f_k = static_cast<float>(k) + 0.5f;
    for(int32_t j = 0; j < dims[1]; j++)
    {
      float f_j = static_cast<float>(j) + 0.5f;
      for(int32_t i = 0; i < dims[0]; i++)
      {
        usize index = count * numComp;
        float f_i = static_cast<float>(i) + 0.5f;
        points[index] = f_i * spacing[0] + origin[0];
        points[index + 1] = f_j * spacing[1] + origin[1];
        points[index + 2] = f_k * spacing[2] + origin[2];
        count++;
      }
    }
  }
}

// -----------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMesh::operator()()
{
  return execute();
}
