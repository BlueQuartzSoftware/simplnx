#include "ComputeTriangleGeomCentroids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeTriangleGeomCentroids::ComputeTriangleGeomCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ComputeTriangleGeomCentroidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeTriangleGeomCentroids::~ComputeTriangleGeomCentroids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeTriangleGeomCentroids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeTriangleGeomCentroids::operator()()
{
  using MeshIndexType = IGeometry::MeshIndexType;
  using SharedVertexListType = IGeometry::SharedVertexList;
  using SharedFaceListType = IGeometry::SharedFaceList;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  const SharedVertexListType& vertexCoords = triangleGeom.getVerticesRef();
  const SharedFaceListType& triangles = triangleGeom.getFacesRef();
  IGeometry::MeshIndexType numTriangles = triangleGeom.getNumberOfFaces();
  auto& featAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);

  MeshIndexType numFeatures = featAttrMat.getNumTuples();
  auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  std::vector<std::set<MeshIndexType>> vertexSets(numFeatures);
  const Int32Array& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath);

  for(MeshIndexType i = 0; i < numTriangles; i++)
  {
    if(faceLabels[2 * i + 0] > 0)
    {
      vertexSets[faceLabels[2 * i + 0]].insert(triangles[3 * i + 0]);
      vertexSets[faceLabels[2 * i + 0]].insert(triangles[3 * i + 1]);
      vertexSets[faceLabels[2 * i + 0]].insert(triangles[3 * i + 2]);
    }
    if(faceLabels[2 * i + 1] > 0)
    {
      vertexSets[faceLabels[2 * i + 1]].insert(triangles[3 * i + 0]);
      vertexSets[faceLabels[2 * i + 1]].insert(triangles[3 * i + 1]);
      vertexSets[faceLabels[2 * i + 1]].insert(triangles[3 * i + 2]);
    }
  }

  for(MeshIndexType i = 0; i < numFeatures; i++)
  {
    for(const auto& vert : vertexSets[i])
    {
      centroids[3 * i + 0] += vertexCoords[3 * vert + 0];
      centroids[3 * i + 1] += vertexCoords[3 * vert + 1];
      centroids[3 * i + 2] += vertexCoords[3 * vert + 2];
    }
    if(!vertexSets[i].empty())
    {
      centroids[3 * i + 0] /= static_cast<float32>(vertexSets[i].size());
      centroids[3 * i + 1] /= static_cast<float32>(vertexSets[i].size());
      centroids[3 * i + 2] /= static_cast<float32>(vertexSets[i].size());
    }
    vertexSets[i].clear();
  }
  return {};
}
