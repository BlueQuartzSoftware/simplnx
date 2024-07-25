#include "ComputeTriangleGeomSizes.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr usize k_00 = 0;
constexpr usize k_01 = 1;
constexpr usize k_02 = 2;
constexpr usize k_10 = 3;
constexpr usize k_11 = 4;
constexpr usize k_12 = 5;
constexpr usize k_20 = 6;
constexpr usize k_21 = 7;
constexpr usize k_22 = 8;

// -----------------------------------------------------------------------------
template <typename T>
T FindTetrahedronVolume(const std::array<usize, 3>& vertIds, const AbstractDataStore<T>& vertexCoords)
{
  // This is a 3x3 matrix laid out in typical "C" order where the columns raster the fastest, then the rows
  std::array<T, 9> volumeMatrix = {
      vertexCoords[3 * vertIds[1] + 0] - vertexCoords[3 * vertIds[0] + 0], vertexCoords[3 * vertIds[2] + 0] - vertexCoords[3 * vertIds[0] + 0], 0.0f - vertexCoords[3 * vertIds[0] + 0],
      vertexCoords[3 * vertIds[1] + 1] - vertexCoords[3 * vertIds[0] + 1], vertexCoords[3 * vertIds[2] + 1] - vertexCoords[3 * vertIds[0] + 1], 0.0f - vertexCoords[3 * vertIds[0] + 1],
      vertexCoords[3 * vertIds[1] + 2] - vertexCoords[3 * vertIds[0] + 2], vertexCoords[3 * vertIds[2] + 2] - vertexCoords[3 * vertIds[0] + 2], 0.0f - vertexCoords[3 * vertIds[0] + 2]};

  T determinant = (volumeMatrix[k_00] * (volumeMatrix[k_11] * volumeMatrix[k_22] - volumeMatrix[k_12] * volumeMatrix[k_21])) -
                  (volumeMatrix[k_01] * (volumeMatrix[k_10] * volumeMatrix[k_22] - volumeMatrix[k_12] * volumeMatrix[k_20])) +
                  (volumeMatrix[k_02] * (volumeMatrix[k_10] * volumeMatrix[k_21] - volumeMatrix[k_11] * volumeMatrix[k_20]));
  return determinant / 6.0f;
}
} // namespace

// -----------------------------------------------------------------------------
ComputeTriangleGeomSizes::ComputeTriangleGeomSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   ComputeTriangleGeomSizesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeTriangleGeomSizes::~ComputeTriangleGeomSizes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeTriangleGeomSizes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeTriangleGeomSizes::operator()()
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

  std::array<usize, 3> faceVertexIndices = {0, 0, 0};

  for(MeshIndexType i = 0; i < numTriangles; i++)
  {
    triangleGeom.getFacePointIds(i, faceVertexIndices);
    if(faceLabels[2 * i + 0] == -1)
    {
      std::swap(faceVertexIndices[2], faceVertexIndices[1]);
      volumes[faceLabels[2 * i + 1]] += FindTetrahedronVolume(faceVertexIndices, vertexCoords);
    }
    else if(faceLabels[2 * i + 1] == -1)
    {
      volumes[faceLabels[2 * i + 0]] += FindTetrahedronVolume(faceVertexIndices, vertexCoords);
    }
    else
    {
      volumes[faceLabels[2 * i + 0]] += FindTetrahedronVolume(faceVertexIndices, vertexCoords);
      std::swap(faceVertexIndices[2], faceVertexIndices[1]);
      volumes[faceLabels[2 * i + 1]] += FindTetrahedronVolume(faceVertexIndices, vertexCoords);
    }
  }

  return {};
}
