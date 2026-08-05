#include "ComputeTriangleGeomCentroids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

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
  using SharedVertexListType = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using SharedFaceListType = AbstractDataStore<IGeometry::SharedFaceList::value_type>;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  const SharedVertexListType& vertexCoords = triangleGeom.getVertices()->getDataStoreRef();
  const SharedFaceListType& triangles = triangleGeom.getFaces()->getDataStoreRef();
  IGeometry::MeshIndexType numTriangles = triangleGeom.getNumberOfFaces();
  const BoundingBox3Df boundingBox = triangleGeom.getBoundingBox();

  // Get the faceLabels array and then get the min and max values
  const Int32AbstractDataStore& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsArrayPath)->getDataStoreRef();
  auto [minFeatureId, maxFeatureId] = std::minmax_element(faceLabels.begin(), faceLabels.end());

  auto& featAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);
  if(featAttrMat.getNumberOfTuples() < *maxFeatureId + 1)
  {
    m_MessageHandler.sendInfoMessage("Increasing Number of tuples in target feature attribute matrix...");
    featAttrMat.resizeTuples(ShapeType{static_cast<usize>(*maxFeatureId + 1)});
  }
  MeshIndexType numFeatures = featAttrMat.getNumberOfTuples();
  auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();
  std::vector<std::set<MeshIndexType>> vertexSets(numFeatures);

  m_MessageHandler.sendInfoMessage(fmt::format("Gathering unique vertices for {} triangles", numTriangles));

  for(MeshIndexType i = 0; i < numTriangles; i++)
  {
    const int32 faceLabel0 = faceLabels[2 * i + 0];
    const int32 faceLabel1 = faceLabels[2 * i + 1];
    if(faceLabel0 > 0)
    {
      vertexSets[faceLabel0].insert(triangles[3 * i + 0]);
      vertexSets[faceLabel0].insert(triangles[3 * i + 1]);
      vertexSets[faceLabel0].insert(triangles[3 * i + 2]);
    }
    if(faceLabel1 > 0)
    {
      vertexSets[faceLabel1].insert(triangles[3 * i + 0]);
      vertexSets[faceLabel1].insert(triangles[3 * i + 1]);
      vertexSets[faceLabel1].insert(triangles[3 * i + 2]);
    }
  }

  m_MessageHandler.sendInfoMessage(fmt::format("Computing centroids for {} features", numFeatures));

  for(MeshIndexType i = 0; i < numFeatures; i++)
  {
    std::set<MeshIndexType> vertexSet = vertexSets[i];
    auto periodicFaces = GeometryHelpers::Topology::FindElementPeriodicFaces(boundingBox, vertexCoords, vertexSet);

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

      if(m_InputValues->IsPeriodic)
      {
        if(GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces(boundingBox, periodicFaces, centroids, i))
        {
          IFilter::Message warningMsg{IFilter::Message::Type::Info, fmt::format("Feature ID {} may be periodic. Manual review may be necessary.", i)};
          m_MessageHandler.m_Callback(warningMsg);
        }
      }
    }
    vertexSets[i].clear();
  }
  return {};
}
