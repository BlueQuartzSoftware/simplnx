#include "ComputeTriangleGeomVolumes.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeTriangleGeomVolumes::ComputeTriangleGeomVolumes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeTriangleGeomVolumesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeTriangleGeomVolumes::~ComputeTriangleGeomVolumes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeTriangleGeomVolumes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeTriangleGeomVolumes::operator()()
{
  using MeshIndexType = AbstractGeometry::MeshIndexType;
  using SharedVertexListType = AbstractDataStore<AbstractGeometry::SharedVertexList::value_type>;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  AbstractGeometry::MeshIndexType numTriangles = triangleGeom.getNumberOfFaces();
  const SharedVertexListType& vertexCoords = triangleGeom.getVertices()->getDataStoreRef();
  const auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsArrayPath)->getDataStoreRef();

  auto maxFaceLabel = std::max_element(faceLabels.begin(), faceLabels.end()); // Ensure the max value is set.

  ShapeType tDims = {static_cast<usize>(*maxFaceLabel) + 1ULL};
  auto& featAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);
  featAttrMat.resizeTuples(tDims);
  auto& volumes = m_DataStructure.getDataAs<Float32Array>(m_InputValues->VolumesArrayPath)->getDataStoreRef();
  volumes.fill(0.0f); // Initialize all volumes to ZERO

  // Ensure any FeatureId/FaceLabel that is used will be a valid index into the volumes array.
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->VolumesArrayPath,
                                                                                  m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath), true, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  auto result = MeshingUtilities::CalculateFeatureVolumes(triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getVertices()->getDataStoreRef(), faceLabels, volumes, m_ShouldCancel);
  if(result.invalid())
  {
    return result;
  }

  for(usize i = 0; i < volumes.size(); i++)
  {
    volumes[i] = std::abs(volumes[i]);
  }

  return {};
}
