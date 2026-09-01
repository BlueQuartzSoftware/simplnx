#include "UncertainRegularGridSampleSurfaceMesh.hpp"

using namespace nx::core;

UncertainRegularGridSampleSurfaceMesh::UncertainRegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             UncertainRegularGridSampleSurfaceMeshInputValues* inputValues)
: SampleSurfaceMesh(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Generator(inputValues->SeedValue)
{
}

UncertainRegularGridSampleSurfaceMesh::~UncertainRegularGridSampleSurfaceMesh() noexcept = default;

const std::atomic_bool& UncertainRegularGridSampleSurfaceMesh::getCancel()
{
  return m_ShouldCancel;
}

SizeVec3 UncertainRegularGridSampleSurfaceMesh::getGridDimensions() const
{
  const auto& dims = m_InputValues->Dimensions;
  return {static_cast<usize>(dims[0]), static_cast<usize>(dims[1]), static_cast<usize>(dims[2])};
}

void UncertainRegularGridSampleSurfaceMesh::generateSlicePoints(usize zSlice, std::vector<Point3Df>& slicePoints)
{
  const auto& dims = m_InputValues->Dimensions;
  const auto& spacing = m_InputValues->Spacing;
  const auto& origin = m_InputValues->Origin;
  const auto& uncertainty = m_InputValues->Uncertainty;

  const usize xDim = dims[0];
  const usize yDim = dims[1];

  // One Z draw per slice preserves monolithic full-volume draw order.
  const float32 randomZ = 2.0f * m_Distribution(m_Generator) - 1.0f;
  const float32 zCoord = ((static_cast<float32>(zSlice) + 0.5f) * spacing[2]) + (uncertainty[2] * randomZ) + origin[2];

  usize outIdx = 0;
  for(usize j = 0; j < yDim; j++)
  {
    // One Y draw applies to the complete row.
    const float32 randomY = 2.0f * m_Distribution(m_Generator) - 1.0f;
    const float32 yCoord = ((static_cast<float32>(j) + 0.5f) * spacing[1]) + (uncertainty[1] * randomY) + origin[1];
    for(usize i = 0; i < xDim; i++)
    {
      const float32 randomX = 2.0f * m_Distribution(m_Generator) - 1.0f;
      const float32 xCoord = ((static_cast<float32>(i) + 0.5f) * spacing[0]) + (uncertainty[0] * randomX) + origin[0];
      slicePoints[outIdx++] = Point3Df(xCoord, yCoord, zCoord);
    }
  }
}

Result<> UncertainRegularGridSampleSurfaceMesh::operator()()
{
  SampleSurfaceMeshInputValues inputs;
  inputs.TriangleGeometryPath = m_InputValues->TriangleGeometryPath;
  inputs.SurfaceMeshFaceLabelsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath;
  inputs.FeatureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;
  return execute(inputs);
}
