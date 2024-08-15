#include "UncertainRegularGridSampleSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <random>

using namespace nx::core;

// -----------------------------------------------------------------------------
UncertainRegularGridSampleSurfaceMesh::UncertainRegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             UncertainRegularGridSampleSurfaceMeshInputValues* inputValues)
: SampleSurfaceMesh(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
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
void UncertainRegularGridSampleSurfaceMesh::generatePoints(std::vector<Point3Df>& points)
{
  auto dims = m_InputValues->Dimensions;
  auto spacing = m_InputValues->Spacing;
  auto origin = m_InputValues->Origin;
  auto uncertainty = m_InputValues->Uncertainty;

  points.reserve(dims[0] * dims[1] * dims[2]);

  std::mt19937 generator(m_InputValues->SeedValue); // Standard mersenne_twister_engine seeded
  std::uniform_real_distribution<float32> distribution(0.0F, 1.0F);

  for(usize k = 0; k < dims[2]; k++)
  {
    float randomZ = 2.0f * distribution(generator) - 1.0f;
    for(usize j = 0; j < dims[1]; j++)
    {
      float randomY = 2.0f * distribution(generator) - 1.0f;
      for(usize i = 0; i < dims[0]; i++)
      {
        float randomX = 2.0f * distribution(generator) - 1.0f;
        points.emplace_back(((static_cast<float>(i) + 0.5f) * spacing[0]) + (uncertainty[0] * randomX) + origin[0],
                            ((static_cast<float>(j) + 0.5f) * spacing[1]) + (uncertainty[1] * randomY) + origin[1],
                            ((static_cast<float>(k) + 0.5f) * spacing[2]) + (uncertainty[2] * randomZ) + origin[2]);
      }
    }
  }
}

// -----------------------------------------------------------------------------
Result<> UncertainRegularGridSampleSurfaceMesh::operator()()
{
  SampleSurfaceMeshInputValues inputs;
  inputs.TriangleGeometryPath = m_InputValues->TriangleGeometryPath;
  inputs.SurfaceMeshFaceLabelsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath;
  inputs.FeatureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;
  return execute(inputs);
}
