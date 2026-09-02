#include "ComputeVertexToTriangleDistances.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/RTree.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;

namespace
{
using RTreeType = RTree<size_t, float, 3, float>;
using SharedTriListT = AbstractDataStore<IGeometry::SharedTriList::value_type>;
using SharedVertexListT = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

/**
 * @brief Take from https://github.com/embree/embree/blob/master/tutorials/common/math/closest_point.h
 * Which has an apache license.
 * @param p
 * @param a
 * @param b
 * @param c
 * @return
 */
Matrix3X1f closestPointTriangle(const Matrix3X1f& p, const Matrix3X1f& a, const Matrix3X1f& b, const Matrix3X1f& c)
{
  const Matrix3X1f ab = b - a;
  const Matrix3X1f ac = c - a;
  const Matrix3X1f ap = p - a;

  const float d1 = ab.dot(ap); // dot(ab, ap);
  const float d2 = ac.dot(ap); // dot(ac, ap);
  if(d1 <= 0.f && d2 <= 0.f)
  {
    return a;
  }

  const Matrix3X1f bp = p - b;
  const float d3 = ab.dot(bp); // dot(ab, bp);
  const float d4 = ac.dot(bp); // dot(ac, bp);
  if(d3 >= 0.f && d4 <= d3)
  {
    return b;
  }

  const Matrix3X1f cp = p - c;
  const float d5 = ab.dot(cp); // dot(ab, cp);
  const float d6 = ac.dot(cp); // dot(ac, cp);
  if(d6 >= 0.f && d5 <= d6)
  {
    return c;
  }

  const float vc = d1 * d4 - d3 * d2;
  if(vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
  {
    const float v = d1 / (d1 - d3);
    return a + v * ab;
  }

  const float vb = d5 * d2 - d1 * d6;
  if(vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
  {
    const float v = d2 / (d2 - d6);
    return a + v * ac;
  }

  const float va = d3 * d6 - d5 * d4;
  if(va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
  {
    const float v = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return b + v * (c - b);
  }

  const float denominator = 1.f / (va + vb + vc);
  const float v = vb * denominator;
  const float w = vc * denominator;
  const Matrix3X1f pointInTriangle = a + v * ab + w * ac;

  return pointInTriangle;
}

float32 PointTriangleDistance(const Matrix3X1f& point, const Matrix3X1f& vert0, const Matrix3X1f& vert1, const Matrix3X1f& vert2, const int64 triangle, const Float64AbstractDataStore& normals)
{

  Matrix3X1f closestPointInTriangle = closestPointTriangle(point, vert0, vert1, vert2);

  auto diffPoint = point - closestPointInTriangle; // Gives a vector pointing from the closest point in triangle to point
  // Only do the dot-product of the vector with itself, so we don't incur the penalty of a square root that we might not need
  float dist = diffPoint.dot(diffPoint);

  Matrix3X1f normal = {static_cast<float32>(normals[3 * triangle + 0]), static_cast<float32>(normals[3 * triangle + 1]), static_cast<float32>(normals[3 * triangle + 2])};

  float32 cosTheta = normal.cosTheta(diffPoint);

  if(cosTheta < 0.0f)
  {
    dist *= -1.0f;
  }

  return dist;
}

class ComputeVertexToTriangleDistancesImpl
{
public:
  ComputeVertexToTriangleDistancesImpl(ComputeVertexToTriangleDistances* filter, const SharedTriListT& triangles, const SharedVertexListT& vertices, SharedVertexListT& sourcePoints,
                                       Float32AbstractDataStore& distances, Int64AbstractDataStore& closestTri, const Float64AbstractDataStore& normals, const RTreeType rtree)
  : m_Filter(filter)
  , m_SharedTriangleList(triangles)
  , m_TriangleVertices(vertices)
  , m_SourcePoints(sourcePoints)
  , m_Distances(distances)
  , m_ClosestTri(closestTri)
  , m_Normals(normals)
  , m_RTree(rtree)
  {
  }
  virtual ~ComputeVertexToTriangleDistancesImpl() = default;

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

  void compute(usize start, usize end) const
  {

    int64 counter = 0;
    auto progIncrement = static_cast<int64>((end - start) / 100);

    size_t numTuples = m_SharedTriangleList.getNumberOfTuples(); // allocate vector of all possible indexes
    for(usize v = start; v < end; v++)
    {
      Matrix3X1f sourcePoint(m_SourcePoints[3 * v], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]);

      std::vector<size_t> hitTriangleIds;
      std::function<bool(size_t)> func = [&](size_t triangleIndex) {
        hitTriangleIds.push_back(triangleIndex);
        return true; // keep going
      };

      int32 nhits = m_RTree.Search(sourcePoint.data(), sourcePoint.data(), func);
      if(nhits > 0) // Point is within the RTree bounding box so just loop over those triangles that are in the RTree
      {
        for(const auto t : hitTriangleIds)
        {
          if(m_Filter->getCancel())
          {
            return;
          }

          auto p = static_cast<int64>(m_SharedTriangleList[t * 3 + 0]);
          auto q = static_cast<int64>(m_SharedTriangleList[t * 3 + 1]);
          auto r = static_cast<int64>(m_SharedTriangleList[t * 3 + 2]);
          const Matrix3X1f point = {m_SourcePoints[3 * v + 0], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]};
          const Matrix3X1f v0(m_TriangleVertices[p * 3 + 0], m_TriangleVertices[p * 3 + 1], m_TriangleVertices[p * 3 + 2]);
          const Matrix3X1f v1(m_TriangleVertices[q * 3 + 0], m_TriangleVertices[q * 3 + 1], m_TriangleVertices[q * 3 + 2]);
          const Matrix3X1f v2(m_TriangleVertices[r * 3 + 0], m_TriangleVertices[r * 3 + 1], m_TriangleVertices[r * 3 + 2]);

          float32 d = PointTriangleDistance(point, v0, v1, v2, static_cast<int64>(t), m_Normals);

          if(std::abs(d) < std::abs(m_Distances[v]))
          {
            m_Distances[v] = d;
            m_ClosestTri[v] = static_cast<int64>(t);
          }
        }
      }
      else // Point was not in the RTree, so we need to search against every triangle
      {
        for(size_t t = 0; t < numTuples; t++)
        {
          if(m_Filter->getCancel())
          {
            return;
          }

          auto p = static_cast<int64>(m_SharedTriangleList[t * 3 + 0]);
          auto q = static_cast<int64>(m_SharedTriangleList[t * 3 + 1]);
          auto r = static_cast<int64>(m_SharedTriangleList[t * 3 + 2]);
          const Matrix3X1f point = {m_SourcePoints[3 * v + 0], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]};
          const Matrix3X1f v0(m_TriangleVertices[p * 3 + 0], m_TriangleVertices[p * 3 + 1], m_TriangleVertices[p * 3 + 2]);
          const Matrix3X1f v1(m_TriangleVertices[q * 3 + 0], m_TriangleVertices[q * 3 + 1], m_TriangleVertices[q * 3 + 2]);
          const Matrix3X1f v2(m_TriangleVertices[r * 3 + 0], m_TriangleVertices[r * 3 + 1], m_TriangleVertices[r * 3 + 2]);

          float32 d = PointTriangleDistance(point, v0, v1, v2, static_cast<int64>(t), m_Normals);

          if(std::abs(d) < std::abs(m_Distances[v]))
          {
            m_Distances[v] = d;
            m_ClosestTri[v] = static_cast<int64>(t);
          }
        }
      }

      if(m_Distances[v] >= 0.0f)
      {
        m_Distances[v] = std::sqrt(m_Distances[v]);
      }
      else
      {
        m_Distances[v] *= -1.0f;
        m_Distances[v] = std::sqrt(m_Distances[v]);
        m_Distances[v] *= -1.0f;
      }

      if(counter > progIncrement)
      {
        m_Filter->sendThreadSafeProgressMessage(counter);
        counter = 0;
      }
      counter++;
    }
    m_Filter->sendThreadSafeProgressMessage(counter);
  }

private:
  ComputeVertexToTriangleDistances* m_Filter;
  const SharedTriListT& m_SharedTriangleList;
  const SharedVertexListT& m_TriangleVertices;
  SharedVertexListT& m_SourcePoints;
  Float32AbstractDataStore& m_Distances;
  Int64AbstractDataStore& m_ClosestTri;
  const Float64AbstractDataStore& m_Normals;
  const RTreeType m_RTree;
};

void GetBoundingBoxAtTri(const SharedTriListT& triList, const SharedVertexListT& vertList, size_t triId, nonstd::span<float> bounds)
{
  size_t v0Index = triList[triId * 3 + 0] * 3;
  size_t v1Index = triList[triId * 3 + 1] * 3;
  size_t v2Index = triList[triId * 3 + 2] * 3;

  auto xMinMax = std::minmax({vertList[v0Index + 0], vertList[v1Index + 0], vertList[v2Index + 0]});
  auto yMinMax = std::minmax({vertList[v0Index + 1], vertList[v1Index + 1], vertList[v2Index + 1]});
  auto zMinMax = std::minmax({vertList[v0Index + 2], vertList[v1Index + 2], vertList[v2Index + 2]});
  bounds[0] = xMinMax.first;
  bounds[1] = yMinMax.first;
  bounds[2] = zMinMax.first;
  bounds[3] = xMinMax.second;
  bounds[4] = yMinMax.second;
  bounds[5] = zMinMax.second;
}
} // namespace

// -----------------------------------------------------------------------------
ComputeVertexToTriangleDistances::ComputeVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   ComputeVertexToTriangleDistancesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeVertexToTriangleDistances::~ComputeVertexToTriangleDistances() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeVertexToTriangleDistances::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void ComputeVertexToTriangleDistances::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementPercent(counter);
}

// -----------------------------------------------------------------------------
Result<> ComputeVertexToTriangleDistances::operator()()
{
  auto& vertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->VertexDataContainer);
  SharedVertexListT& sourceVertices = vertexGeom.getVertices()->getDataStoreRef();
  usize totalElements = vertexGeom.getNumberOfVertices();

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleDataContainer);
  auto numTris = static_cast<usize>(triangleGeom.getNumberOfFaces());
  const SharedTriListT& triangles = triangleGeom.getFaces()->getDataStoreRef();
  const SharedVertexListT& vertices = triangleGeom.getVertices()->getDataStoreRef();

  RTreeType m_RTree;
  // Populate the RTree
  std::vector<float> triBoundsArray(numTris * 6, 0.0F);
  for(size_t triIndex = 0; triIndex < numTris; triIndex++)
  {
    GetBoundingBoxAtTri(triangles, vertices, triIndex, {triBoundsArray.data() + (6 * triIndex), 6});
    m_RTree.Insert(triBoundsArray.data() + (6 * triIndex), triBoundsArray.data() + (6 * triIndex) + 3, triIndex); // Note, all values including zero are fine in this version
  }

  const auto& normalsArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->TriangleNormalsArrayPath)->getDataStoreRef();
  auto& distancesArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->DistancesArrayPath)->getDataStoreRef();
  distancesArray.fill(std::numeric_limits<float32>::max());
  auto& closestTriangleIdsArray = m_DataStructure.getDataAs<Int64Array>(m_InputValues->ClosestTriangleIdArrayPath)->getDataStoreRef();
  closestTriangleIdsArray.fill(-1); // -1 means it never found the closest triangle?

  m_Throttle.reset(totalElements, "Finding Vertex To Triangle Distances");

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(true);
  dataAlg.setRange(0, totalElements);
  dataAlg.execute(ComputeVertexToTriangleDistancesImpl(this, triangles, vertices, sourceVertices, distancesArray, closestTriangleIdsArray, normalsArray, m_RTree));

  return {};
}
