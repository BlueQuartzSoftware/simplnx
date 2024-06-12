#include "SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"

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
  // geometry will be rotated so that the sectioning direction is always 001 before rotating back
  std::array<float32, 3> n = {0.0f, 0.0f, 1.0f};

  auto& triangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->CADDataContainerName);
  triangle.findEdges();

  INodeGeometry2D::SharedFaceList& tris = triangle.getFacesRef();
  INodeGeometry0D::SharedVertexList& triVerts = triangle.getVerticesRef();
  usize numTris = triangle.getNumberOfFaces();
  usize numTriVerts = triangle.getNumberOfVertices();

  // rotate CAD triangles to get into sectioning orientation
  // rotateVertices(rotForward, n, numTriVerts, triVerts);

  // determine bounds and number of slices needed for CAD geometry
  float32 minDim = std::numeric_limits<float32>::max();
  float32 maxDim = -minDim;
  usize numberOfSlices = determineBoundsAndNumSlices(minDim, maxDim, numTris, tris, triVerts);
  const auto minSlice = static_cast<int64>(minDim / m_InputValues->SliceResolution);
  const auto maxSlice = static_cast<int64>(maxDim / m_InputValues->SliceResolution);

  std::array<float32, 3> q = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> r = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> p = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> corner = {0.0f, 0.0f, 0.0f};
  float32 d = 0;

  std::vector<float32> slicedVerts;
  std::vector<int32> sliceIds;
  std::vector<int32> regionIds;

  // Get an object reference to the pointer
  auto& triRegionId = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->RegionIdArrayPath);

  int32 edgeCounter = 0;
  for(usize i = 0; i < numTris; i++)
  {
    int32 regionId = 0;
    // get region Id of this triangle (if they are available)
    if(m_InputValues->HaveRegionIds)
    {
      regionId = triRegionId[i];
    }
    // determine which slices would hit the triangle
    auto minTriDim = std::numeric_limits<float32>::max();
    float32 maxTriDim = -minTriDim;
    for(usize j = 0; j < 3; j++)
    {
      int64 vert = tris[3 * i + j];
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
    auto firstSlice = static_cast<int64>(minTriDim / m_InputValues->SliceResolution);
    auto lastSlice = static_cast<int64>(maxTriDim / m_InputValues->SliceResolution);
    if(firstSlice < minSlice)
    {
      firstSlice = minSlice;
    }
    if(lastSlice > maxSlice)
    {
      lastSlice = maxSlice;
    }
    // get cross product of triangle vectors to get normals
    float32 vecAB[3];
    float32 vecAC[3];
    float32 triCross[3];
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
    for(int64 j = firstSlice; j <= lastSlice; j++)
    {
      int cut = 0;
      bool cornerHit = false;
      d = (m_InputValues->SliceResolution * static_cast<float32>(j));
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
        const usize size = slicedVerts.size();
        // get delta x for the current ordering of the segment
        const float32 delX = slicedVerts[size - 6] - slicedVerts[size - 3];
        // get cross product of vec with 001 slicing direction
        if((triCross[1] > 0 && delX < 0) || (triCross[1] < 0 && delX > 0))
        {
          const float32 temp[3] = {slicedVerts[size - 3], slicedVerts[size - 2], slicedVerts[size - 1]};
          slicedVerts[size - 3] = slicedVerts[size - 6];
          slicedVerts[size - 2] = slicedVerts[size - 5];
          slicedVerts[size - 1] = slicedVerts[size - 4];
          slicedVerts[size - 6] = temp[0];
          slicedVerts[size - 5] = temp[1];
          slicedVerts[size - 4] = temp[2];
        }
        sliceIds.push_back(j);
        if(m_InputValues->HaveRegionIds)
        {
          regionIds.push_back(regionId);
        }
        edgeCounter++;
      }
    }
  }

  usize numVerts = slicedVerts.size() / 3;
  usize numEdges = slicedVerts.size() / 6;

  if(numVerts != (2 * numEdges))
  {
    return MakeErrorResult(-62101, fmt::format("Number of sectioned vertices and edges do not make sense.  Number of Vertices: {} and Number of Edges: {}", numVerts, numEdges));
  }

  auto& edge = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->SliceDataContainerName);
  edge.resizeVertexList(numVerts);
  edge.resizeEdgeList(numEdges);
  INodeGeometry0D::SharedVertexList& verts = edge.getVerticesRef();
  INodeGeometry1D::SharedEdgeList& edges = edge.getEdgesRef();
  edge.getVertexAttributeMatrix()->resizeTuples({numVerts});
  edge.getEdgeAttributeMatrix()->resizeTuples({numEdges});
  auto& sliceAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->SliceDataContainerName.createChildPath(m_InputValues->SliceAttributeMatrixName));
  sliceAM.resizeTuples({numberOfSlices});

  DataPath edgeAmPath = m_InputValues->SliceDataContainerName.createChildPath(m_InputValues->EdgeAttributeMatrixName);
  auto& sliceId = m_DataStructure.getDataRefAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->SliceIdArrayName));
  sliceId.fill(0);
  Int32Array* triRegionIds = nullptr;
  if(m_InputValues->HaveRegionIds)
  {
    triRegionIds = m_DataStructure.getDataAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->RegionIdArrayPath.getTargetName()));
    triRegionId.fill(0);
  }

  for(usize i = 0; i < numEdges; i++)
  {
    edges[2 * i] = 2 * i;
    edges[2 * i + 1] = 2 * i + 1;
    verts[3 * (2 * i)] = slicedVerts[3 * (2 * i)];
    verts[3 * (2 * i) + 1] = slicedVerts[3 * (2 * i) + 1];
    verts[3 * (2 * i) + 2] = slicedVerts[3 * (2 * i) + 2];
    verts[3 * (2 * i + 1)] = slicedVerts[3 * (2 * i + 1)];
    verts[3 * (2 * i + 1) + 1] = slicedVerts[3 * (2 * i + 1) + 1];
    verts[3 * (2 * i + 1) + 2] = slicedVerts[3 * (2 * i + 1) + 2];
    sliceId[i] = sliceIds[i];
    if(m_InputValues->HaveRegionIds)
    {
      (*triRegionIds)[i] = regionIds[i];
    }
  }

  // rotate all CAD triangles back to original orientation
  // rotateVertices(rotBackward, n, numTriVerts, triVerts);
  // rotate all edges back to original orientation
  // rotateVertices(rotBackward, n, numVerts, verts);

  // m_MessageHandler("Complete");

  Result<> result = GeometryUtilities::EliminateDuplicateNodes<EdgeGeom>(edge);
  return result;
}

// -----------------------------------------------------------------------------
char SliceTriangleGeometry::rayIntersectsPlane(const float d, const std::array<float32, 3>& q, const std::array<float32, 3>& r, std::array<float32, 3>& p)
{
  const float64 rqDelZ = r[2] - q[2];
  const float64 dqDelZ = d - q[2];
  const float64 t = dqDelZ / rqDelZ;
  for(int i = 0; i < 3; i++)
  {
    p[i] = q[i] + (t * (r[i] - q[i]));
  }
  if(t > 0.0 && t < 1.0)
  {
    return '1';
  }
  if(t == 0.0)
  {
    return 'q';
  }
  if(t == 1.0)
  {
    return 'r';
  }

  return '0';
}

// -----------------------------------------------------------------------------
usize SliceTriangleGeometry::determineBoundsAndNumSlices(float32& minDim, float32& maxDim, usize numTris, INodeGeometry2D::SharedFaceList& tris, INodeGeometry0D::SharedVertexList& triVerts)
{
  for(usize i = 0; i < numTris; i++)
  {
    for(usize j = 0; j < 3; j++)
    {
      const usize vert = tris[3 * i + j];
      if(minDim > triVerts[3 * vert + 2])
      {
        minDim = triVerts[3 * vert + 2];
      }
      if(maxDim < triVerts[3 * vert + 2])
      {
        maxDim = triVerts[3 * vert + 2];
      }
    }
  }

  // adjust sectioning range if user selected a specific range - check that user range is within actual range
  if(m_InputValues->SliceRange == 1)
  {
    if(m_InputValues->Zstart > minDim)
    {
      minDim = m_InputValues->Zstart;
    }
    if(m_InputValues->Zend < maxDim)
    {
      maxDim = m_InputValues->Zend;
    }
  }
  // TODO: Is this still correct? Why have the subtraction at all?
  const auto numberOfSlices = static_cast<usize>((maxDim - 0.0) / m_InputValues->SliceResolution) + 1;
  // const auto numberOfSlices = static_cast<usize>((maxDim - minDim) / m_InputValues->SliceResolution) + 1;
  return numberOfSlices;
}
