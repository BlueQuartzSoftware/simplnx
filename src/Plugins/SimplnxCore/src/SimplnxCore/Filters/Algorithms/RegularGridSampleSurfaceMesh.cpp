#include "RegularGridSampleSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           RegularGridSampleSurfaceMeshInputValues* inputValues)
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
void RegularGridSampleSurfaceMesh::generatePoints(std::vector<Point3Df>& points)
{
  auto dims = m_InputValues->Dimensions;
  auto spacing = m_InputValues->Spacing;
  auto origin = m_InputValues->Origin;

  points.reserve(dims[0] * dims[1] * dims[2]);

  for(int32 k = 0; k < dims[2]; k++)
  {
    float32 f_k = static_cast<float32>(k) + 0.5f;
    for(int32 j = 0; j < dims[1]; j++)
    {
      float32 f_j = static_cast<float32>(j) + 0.5f;
      for(int32 i = 0; i < dims[0]; i++)
      {
        float32 f_i = static_cast<float32>(i) + 0.5f;
        points.emplace_back(f_i * spacing[0] + origin[0], f_j * spacing[1] + origin[1], f_k * spacing[2] + origin[2]);
      }
    }
  }
}

// -----------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMesh::operator()()
{
  SampleSurfaceMeshInputValues inputs;
  inputs.TriangleGeometryPath = m_InputValues->TriangleGeometryPath;
  inputs.SurfaceMeshFaceLabelsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath;
  inputs.FeatureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;
  return execute(inputs);
}
