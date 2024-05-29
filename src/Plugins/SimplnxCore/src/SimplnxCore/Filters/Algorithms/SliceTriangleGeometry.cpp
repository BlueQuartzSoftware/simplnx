#include "SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
SliceTriangleGeometry::SliceTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             SliceTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SliceTriangleGeometry::~SliceTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SliceTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SliceTriangleGeometry::operator()()
{
#if 0
  // geometry will be rotated so that the sectioning direction is always 001 before rotating back
  float n[3] = {0.0f, 0.0f, 0.0f};
  n[0] = 0.0f;
  n[1] = 0.0f;
  n[2] = 1.0f;

  TriangleGeom::Pointer triangle = getDataContainerArray()->getDataContainer(getCADDataContainerName())->getGeometryAs<TriangleGeom>();
  triangle->findEdges();

  MeshIndexType* tris = triangle->getTriPointer(0);
  float* triVerts = triangle->getVertexPointer(0);
  MeshIndexType numTris = triangle->getNumberOfTris();
  MeshIndexType numTriVerts = triangle->getNumberOfVertices();

  // rotate CAD tiangles to get into sectioning orientation
  rotateVertices(rotForward, n, numTriVerts, triVerts);

  // determine bounds and number of slices needed for CAD geometry
  float minDim = std::numeric_limits<float>::max();
  float maxDim = -minDim;
  determineBoundsAndNumSlices(minDim, maxDim, numTris, tris, triVerts);
  int64_t minSlice = static_cast<int64_t>(minDim / m_SliceResolution);
  int64_t maxSlice = static_cast<int64_t>(maxDim / m_SliceResolution);

  float q[3] = {0.0f, 0.0f, 0.0f};
  float r[3] = {0.0f, 0.0f, 0.0f};
  float p[3] = {0.0f, 0.0f, 0.0f};
  float corner[3] = {0.0f, 0.0f, 0.0f};
  float d = 0;

  std::vector<float> slicedVerts;
  std::vector<int32_t> sliceIds;
  std::vector<int32_t> regionIds;

  // Get an object reference to the pointer
  Int32ArrayType& m_TriRegionId = *(m_TriRegionIdPtr.lock());

  int32_t edgeCounter = 0;
  for(MeshIndexType i = 0; i < numTris; i++)
  {
    int32_t regionId = 0;
    // get region Id of this triangle (if they are available)
    if(m_HaveRegionIds)
    {
      regionId = m_TriRegionId[i];
    }
    // determine which slices would hit the triangle
    float minTriDim = std::numeric_limits<float>::max();
    float maxTriDim = -minTriDim;
    for(size_t j = 0; j < 3; j++)
    {
      int64_t vert = tris[3 * i + j];
      if(minTriDim > triVerts[3 * vert + 2])
      {
        minTriDim = triVerts[3 * vert + 2];
      }
      if(maxTriDim < triVerts[3 * vert + 2])
      {
        maxTriDim = triVerts[3 * vert + 2];
      }
    }
    if(minTriDim > maxDim || maxTriDim < minDim)
    {
      continue;
    }
    if(minTriDim < minDim)
    {
      minTriDim = minDim;
    }
    if(maxTriDim > maxDim)
    {
      maxTriDim = maxDim;
    }
    int64_t firstSlice = static_cast<int64_t>(minTriDim / m_SliceResolution);
    int64_t lastSlice = static_cast<int64_t>(maxTriDim / m_SliceResolution);
    if(firstSlice < minSlice)
    {
      firstSlice = minSlice;
    }
    if(lastSlice > maxSlice)
    {
      lastSlice = maxSlice;
    }
    // get cross product of triangle vectors to get normals
    float vecAB[3];
    float vecAC[3];
    float triCross[3];
    char val;
    vecAB[0] = triVerts[3 * tris[3 * i + 1]] - triVerts[3 * tris[3 * i]];
    vecAB[1] = triVerts[3 * tris[3 * i + 1] + 1] - triVerts[3 * tris[3 * i] + 1];
    vecAB[2] = triVerts[3 * tris[3 * i + 1] + 2] - triVerts[3 * tris[3 * i] + 2];
    vecAC[0] = triVerts[3 * tris[3 * i + 2]] - triVerts[3 * tris[3 * i]];
    vecAC[1] = triVerts[3 * tris[3 * i + 2] + 1] - triVerts[3 * tris[3 * i] + 1];
    vecAC[2] = triVerts[3 * tris[3 * i + 2] + 2] - triVerts[3 * tris[3 * i] + 2];
    triCross[0] = vecAB[1] * vecAC[2] - vecAB[2] * vecAC[1];
    triCross[1] = vecAB[2] * vecAC[0] - vecAB[0] * vecAC[2];
    triCross[2] = vecAB[0] * vecAC[1] - vecAB[1] * vecAC[0];
    for(int64_t j = firstSlice; j <= lastSlice; j++)
    {
      int cut = 0;
      bool cornerHit = false;
      d = (m_SliceResolution * float(j));
      q[0] = triVerts[3 * tris[3 * i]];
      q[1] = triVerts[3 * tris[3 * i] + 1];
      q[2] = triVerts[3 * tris[3 * i] + 2];
      r[0] = triVerts[3 * tris[3 * i + 1]];
      r[1] = triVerts[3 * tris[3 * i + 1] + 1];
      r[2] = triVerts[3 * tris[3 * i + 1] + 2];
      if(q[2] > r[2])
      {
        val = rayIntersectsPlane(d, r, q, p);
      }
      else
      {
        val = rayIntersectsPlane(d, q, r, p);
      }
      if(val == '1')
      {
        slicedVerts.push_back(p[0]);
        slicedVerts.push_back(p[1]);
        slicedVerts.push_back(p[2]);
        cut++;
      }
      else if(val == 'q' || val == 'r')
      {
        cornerHit = true;
        corner[0] = p[0];
        corner[1] = p[1];
        corner[2] = p[2];
      }
      r[0] = triVerts[3 * tris[3 * i + 2]];
      r[1] = triVerts[3 * tris[3 * i + 2] + 1];
      r[2] = triVerts[3 * tris[3 * i + 2] + 2];
      if(q[2] > r[2])
      {
        val = rayIntersectsPlane(d, r, q, p);
      }
      else
      {
        val = rayIntersectsPlane(d, q, r, p);
      }
      if(val == '1')
      {
        slicedVerts.push_back(p[0]);
        slicedVerts.push_back(p[1]);
        slicedVerts.push_back(p[2]);
        cut++;
      }
      else if(val == 'q' || val == 'r')
      {
        cornerHit = true;
        corner[0] = p[0];
        corner[1] = p[1];
        corner[2] = p[2];
      }
      q[0] = triVerts[3 * tris[3 * i + 1]];
      q[1] = triVerts[3 * tris[3 * i + 1] + 1];
      q[2] = triVerts[3 * tris[3 * i + 1] + 2];
      if(q[2] > r[2])
      {
        val = rayIntersectsPlane(d, r, q, p);
      }
      else
      {
        val = rayIntersectsPlane(d, q, r, p);
      }
      if(val == '1')
      {
        slicedVerts.push_back(p[0]);
        slicedVerts.push_back(p[1]);
        slicedVerts.push_back(p[2]);
        cut++;
      }
      else if(val == 'q' || val == 'r')
      {
        cornerHit = true;
        corner[0] = p[0];
        corner[1] = p[1];
        corner[2] = p[2];
      }
      if(cut == 1 && !cornerHit)
      {
        for(int k = 0; k < 3; k++)
        {
          slicedVerts.pop_back();
        }
      }
      if(cut == 1 && cornerHit)
      {
        slicedVerts.push_back(corner[0]);
        slicedVerts.push_back(corner[1]);
        slicedVerts.push_back(corner[2]);
        cut++;
      }
      if(cut == 3)
      {
        for(int k = 0; k < 9; k++)
        {
          slicedVerts.pop_back();
        }
      }
      if(cut == 2)
      {
        size_t size = slicedVerts.size();
        // get delta x for the current ordering of the segment
        float delX = slicedVerts[size - 6] - slicedVerts[size - 3];
        // get cross product of vec with 001 slicing direction
        if((triCross[1] > 0 && delX < 0) || (triCross[1] < 0 && delX > 0))
        {
          float temp[3] = {slicedVerts[size - 3], slicedVerts[size - 2], slicedVerts[size - 1]};
          slicedVerts[size - 3] = slicedVerts[size - 6];
          slicedVerts[size - 2] = slicedVerts[size - 5];
          slicedVerts[size - 1] = slicedVerts[size - 4];
          slicedVerts[size - 6] = temp[0];
          slicedVerts[size - 5] = temp[1];
          slicedVerts[size - 4] = temp[2];
        }
        sliceIds.push_back(j);
        if(m_HaveRegionIds)
        {
          regionIds.push_back(regionId);
        }
        edgeCounter++;
      }
    }
  }

  size_t numVerts = slicedVerts.size() / 3;
  size_t numEdges = slicedVerts.size() / 6;

  if(numVerts != (2 * numEdges))
  {
    QString message = QObject::tr("Number of sectioned vertices and edges do not make sense.  Number of Vertices: %1 and Number of Edges: %2").arg(numVerts).arg(numEdges);
    setErrorCondition(-13003, message);
    return;
  }

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getSliceDataContainerName());
  SharedVertexList::Pointer vertices = EdgeGeom::CreateSharedVertexList(numVerts);
  EdgeGeom::Pointer edge = EdgeGeom::CreateGeometry(numEdges, vertices, SIMPL::Geometry::EdgeGeometry, !getInPreflight());
  float* verts = edge->getVertexPointer(0);
  MeshIndexType* edges = edge->getEdgePointer(0);

  std::vector<size_t> tDims(1, numEdges);
  m->getAttributeMatrix(getEdgeAttributeMatrixName())->resizeAttributeArrays(tDims);

  tDims[0] = m_NumberOfSlices;
  m->getAttributeMatrix(getSliceAttributeMatrixName())->resizeAttributeArrays(tDims);

  // Weak pointers are still good because the resize operations are affecting the internal structure of the DataArray<T>
  // and not the actual pointer to the DataArray<T> object itself.
  Int32ArrayType& m_SliceId = *(m_SliceIdPtr.lock());
  Int32ArrayType& m_RegionId = *(m_RegionIdPtr.lock());

  for(size_t i = 0; i < numEdges; i++)
  {
    edges[2 * i] = 2 * i;
    edges[2 * i + 1] = 2 * i + 1;
    verts[3 * (2 * i)] = slicedVerts[3 * (2 * i)];
    verts[3 * (2 * i) + 1] = slicedVerts[3 * (2 * i) + 1];
    verts[3 * (2 * i) + 2] = slicedVerts[3 * (2 * i) + 2];
    verts[3 * (2 * i + 1)] = slicedVerts[3 * (2 * i + 1)];
    verts[3 * (2 * i + 1) + 1] = slicedVerts[3 * (2 * i + 1) + 1];
    verts[3 * (2 * i + 1) + 2] = slicedVerts[3 * (2 * i + 1) + 2];
    m_SliceId[i] = sliceIds[i];
    if(m_HaveRegionIds)
    {
      m_RegionId[i] = regionIds[i];
    }
  }

  // rotate all CAD triangles back to original orientation
  rotateVertices(rotBackward, n, numTriVerts, triVerts);

  // rotate all edges back to original orientation
  rotateVertices(rotBackward, n, numVerts, verts);

  m->setGeometry(edge);

  notifyStatusMessage("Complete");
#endif
  return {};
}
