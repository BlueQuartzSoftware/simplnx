#include "ComputeTriangleGeomVolumes.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
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
  using MeshIndexType = IGeometry::MeshIndexType;
  using SharedVertexListType = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  IGeometry::MeshIndexType numTriangles = triangleGeom.getNumberOfFaces();
  const SharedVertexListType& vertexCoords = triangleGeom.getVertices()->getDataStoreRef();
  const auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsArrayPath)->getDataStoreRef();

  std::set<int32> featureSet;

  for(MeshIndexType i = 0; i < numTriangles; i++)
  {
    if(faceLabels[2 * i + 0] > 0)
    {
      featureSet.insert(faceLabels[2 * i + 0]);
    }
    if(faceLabels[2 * i + 1] > 0)
    {
      featureSet.insert(faceLabels[2 * i + 1]);
    }
  }

  AttributeMatrix::ShapeType tDims = {featureSet.size() + 1};
  auto& featAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);
  featAttrMat.resizeTuples(tDims);
  auto& volumes = m_DataStructure.getDataAs<Float32Array>(m_InputValues->VolumesArrayPath)->getDataStoreRef();
  volumes.fill(0.0f); // Initialize all volumes to ZERO

  auto result = MeshingUtilities::CalculateFeatureVolumes(triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getVertices()->getDataStoreRef(), faceLabels, volumes, m_ShouldCancel);
  if(result.invalid())
  {
    return result;
  }

  for(usize i = 0; i < tDims[0]; i++)
  {
    volumes[i] = std::abs(volumes[i]);
  }

  return {};
}
