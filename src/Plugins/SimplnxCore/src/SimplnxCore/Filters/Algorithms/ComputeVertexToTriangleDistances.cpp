#include "ComputeVertexToTriangleDistances.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/RTree.hpp"

#include <algorithm>
#include <array>
#include <numeric>

using namespace nx::core;

namespace
{
using RTreeType = RTree<size_t, float, 3, float>;
using SharedTriListT = AbstractDataStore<IGeometry::SharedTriList::value_type>;
using SharedVertexListT = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

/**
 * @brief Caps candidate-box expansion attempts.
 *
 * Exhaustion indicates degenerate geometry, such as nonfinite coordinates,
 * because 64 doublings exceed any practical finite triangle bound.
 */
constexpr int32 k_MaxBoxExpansions = 64;

/**
 * @brief Returns the closest point on a triangle.
 * @param p Specifies the query point.
 * @param a Specifies the first triangle vertex.
 * @param b Specifies the second triangle vertex.
 * @param c Specifies the third triangle vertex.
 * @return Point on the triangle nearest to p.
 *
 * This implementation derives from
 * https://github.com/embree/embree/blob/master/tutorials/common/math/closest_point.h,
 * which has an Apache license.
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

/**
 * @brief Computes the signed squared distance from a point to a triangle.
 * @param point Specifies the query point.
 * @param vert0 Specifies the first triangle vertex.
 * @param vert1 Specifies the second triangle vertex.
 * @param vert2 Specifies the third triangle vertex.
 * @param triangle Identifies the triangle normal.
 * @param normals Provides three normal components per triangle.
 * @return Squared distance, negative on the normal's back side.
 *
 * The caller takes the square root after it chooses the closest triangle.
 */
float32 PointTriangleDistance(const Matrix3X1f& point, const Matrix3X1f& vert0, const Matrix3X1f& vert1, const Matrix3X1f& vert2, const int64 triangle, const Float64AbstractDataStore& normals)
{
  Matrix3X1f closestPointInTriangle = closestPointTriangle(point, vert0, vert1, vert2);

  auto diffPoint = point - closestPointInTriangle;
  // The squared form avoids square roots for nonwinning candidates.
  float dist = diffPoint.dot(diffPoint);

  Matrix3X1f normal = {static_cast<float32>(normals[3 * triangle + 0]), static_cast<float32>(normals[3 * triangle + 1]), static_cast<float32>(normals[3 * triangle + 2])};

  float32 cosTheta = normal.cosTheta(diffPoint);

  if(cosTheta < 0.0f)
  {
    dist *= -1.0f;
  }

  return dist;
}

/**
 * @brief Finds triangle bounds that overlap a cubic query box.
 * @param rtree Provides triangle bounds.
 * @param center Specifies the box center.
 * @param halfExtent Specifies the box half extent.
 * @return Overlapping triangle indexes in RTree traversal order.
 */
std::vector<size_t> FindTrianglesWithinBox(const RTreeType& rtree, const Matrix3X1f& center, float32 halfExtent)
{
  std::vector<size_t> candidateIds;
  std::function<bool(size_t)> collect = [&candidateIds](size_t triangleIndex) {
    candidateIds.push_back(triangleIndex);
    return true;
  };

  const std::array<float32, 3> minCorner = {center.getX() - halfExtent, center.getY() - halfExtent, center.getZ() - halfExtent};
  const std::array<float32, 3> maxCorner = {center.getX() + halfExtent, center.getY() + halfExtent, center.getZ() + halfExtent};
  rtree.Search(minCorner.data(), maxCorner.data(), collect);
  return candidateIds;
}

/**
 * @brief Finds an initial triangle candidate set with an expanding query box.
 * @param rtree Provides triangle bounds.
 * @param center Specifies the source point.
 * @param initialHalfExtent Specifies the initial box half extent.
 * @return Candidate indexes in RTree traversal order, or empty for degenerate geometry.
 *
 * A real box finds triangles near a point because a zero-volume query rarely
 * overlaps a tight triangle bound. A later radius query provides the exact result.
 */
std::vector<size_t> FindCandidateTrianglesByExpandingBox(const RTreeType& rtree, const Matrix3X1f& center, float32 initialHalfExtent)
{
  float32 halfExtent = initialHalfExtent;
  for(int32 attempt = 0; attempt < k_MaxBoxExpansions; attempt++)
  {
    std::vector<size_t> candidateIds = FindTrianglesWithinBox(rtree, center, halfExtent);
    if(!candidateIds.empty())
    {
      return candidateIds;
    }
    halfExtent *= 2.0f;
  }
  return {};
}

/**
 * @brief Retains the closest triangle from sorted candidates.
 * @param candidateIds Specifies ascending triangle indexes to test.
 * @param point Specifies the source vertex position.
 * @param triangleList Provides three vertex indexes per triangle.
 * @param triangleVertices Provides triangle vertex positions.
 * @param normals Provides normals that sign distances.
 * @param bestSignedSquaredDistance Receives the best signed squared distance.
 * @param bestTriangleId Receives the triangle index, or -1 when none exists.
 *
 * Ascending indexes and strict comparison retain the lowest index for an exact
 * distance tie. This matches an ascending brute-force scan.
 */
void EvaluateClosestCandidate(nonstd::span<const size_t> candidateIds, const Matrix3X1f& point, const SharedTriListT& triangleList, const SharedVertexListT& triangleVertices,
                              const Float64AbstractDataStore& normals, float32& bestSignedSquaredDistance, int64& bestTriangleId)
{
  for(const size_t t : candidateIds)
  {
    const auto p = static_cast<int64>(triangleList[t * 3 + 0]);
    const auto q = static_cast<int64>(triangleList[t * 3 + 1]);
    const auto r = static_cast<int64>(triangleList[t * 3 + 2]);
    const Matrix3X1f v0(triangleVertices[p * 3 + 0], triangleVertices[p * 3 + 1], triangleVertices[p * 3 + 2]);
    const Matrix3X1f v1(triangleVertices[q * 3 + 0], triangleVertices[q * 3 + 1], triangleVertices[q * 3 + 2]);
    const Matrix3X1f v2(triangleVertices[r * 3 + 0], triangleVertices[r * 3 + 1], triangleVertices[r * 3 + 2]);

    const float32 d = PointTriangleDistance(point, v0, v1, v2, static_cast<int64>(t), normals);
    if(std::abs(d) < std::abs(bestSignedSquaredDistance))
    {
      bestSignedSquaredDistance = d;
      bestTriangleId = static_cast<int64>(t);
    }
  }
}

/**
 * @class ComputeVertexToTriangleDistancesImpl
 * @brief Calculates distances for a parallel vertex range.
 *
 * The current worker concurrently accesses shared DataStore instances. DataStore
 * does not generally guarantee concurrent access. This is an existing limitation.
 */
class ComputeVertexToTriangleDistancesImpl
{
public:
  /**
   * @brief Creates a parallel distance worker.
   * @param filter Provides cancellation state.
   * @param triangles Provides triangle vertex indexes.
   * @param vertices Provides triangle vertex positions.
   * @param sourcePoints Provides source vertex positions.
   * @param distances Receives signed distances.
   * @param closestTri Receives closest triangle indexes.
   * @param normals Provides triangle normals.
   * @param rtree Provides triangle bounds.
   * @param initialSearchHalfExtent Seeds candidate-box expansion.
   * @param progressMessageHelper Creates range-local progress messengers.
   */
  ComputeVertexToTriangleDistancesImpl(ComputeVertexToTriangleDistances* filter, const SharedTriListT& triangles, const SharedVertexListT& vertices, SharedVertexListT& sourcePoints,
                                       Float32AbstractDataStore& distances, Int64AbstractDataStore& closestTri, const Float64AbstractDataStore& normals, const RTreeType rtree,
                                       float32 initialSearchHalfExtent, ProgressMessageHelper& progressMessageHelper)
  : m_Filter(filter)
  , m_SharedTriangleList(triangles)
  , m_TriangleVertices(vertices)
  , m_SourcePoints(sourcePoints)
  , m_Distances(distances)
  , m_ClosestTri(closestTri)
  , m_Normals(normals)
  , m_RTree(rtree)
  , m_InitialSearchHalfExtent(initialSearchHalfExtent)
  , m_ProgressMessageHelper(progressMessageHelper)
  {
  }
  /**
   * @brief Destroys the non-owning parallel worker.
   */
  virtual ~ComputeVertexToTriangleDistancesImpl() = default;

  /**
   * @brief Processes one parallel vertex range.
   * @param range Specifies the half-open vertex-index range.
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

  /**
   * @brief Calculates nearest-triangle distances for a vertex range.
   * @param start Specifies the first vertex index.
   * @param end Specifies the exclusive vertex index.
   */
  void compute(usize start, usize end) const
  {
    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

    int64 counter = 0;
    auto progIncrement = static_cast<int64>((end - start) / 100);

    const size_t numTuples = m_SharedTriangleList.getNumberOfTuples();
    for(usize v = start; v < end; v++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      const Matrix3X1f sourcePoint(m_SourcePoints[3 * v], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]);

      float32 bestSignedSquaredDistance = std::numeric_limits<float32>::max();
      int64 bestTriangleId = -1;

      if(numTuples > 0)
      {
        // An expanding box cheaply finds a first candidate set.
        std::vector<size_t> candidateIds = FindCandidateTrianglesByExpandingBox(m_RTree, sourcePoint, m_InitialSearchHalfExtent);
        if(candidateIds.empty())
        {
          // Degenerate geometry has no finite bound hit. The exhaustive fallback
          // still produces a result for this vertex.
          candidateIds.resize(numTuples);
          std::iota(candidateIds.begin(), candidateIds.end(), size_t{0});
        }
        else
        {
          std::sort(candidateIds.begin(), candidateIds.end());
        }
        EvaluateClosestCandidate(candidateIds, sourcePoint, m_SharedTriangleList, m_TriangleVertices, m_Normals, bestSignedSquaredDistance, bestTriangleId);

        // The first candidate distance bounds the exact radius-refine query.
        // Every closer triangle bound must overlap that query box.
        const float32 bestAbsSquaredDistance = std::abs(bestSignedSquaredDistance);
        if(bestTriangleId >= 0 && bestAbsSquaredDistance > 0.0f)
        {
          // The pad prevents square-root rounding from excluding a bound that
          // touches the exact query boundary.
          const float32 radius = std::sqrt(bestAbsSquaredDistance) * (1.0f + 1.0e-4f);
          std::vector<size_t> refinedCandidateIds = FindTrianglesWithinBox(m_RTree, sourcePoint, radius);
          std::sort(refinedCandidateIds.begin(), refinedCandidateIds.end());

          bestSignedSquaredDistance = std::numeric_limits<float32>::max();
          bestTriangleId = -1;
          EvaluateClosestCandidate(refinedCandidateIds, sourcePoint, m_SharedTriangleList, m_TriangleVertices, m_Normals, bestSignedSquaredDistance, bestTriangleId);
        }
      }

      if(bestTriangleId >= 0)
      {
        m_Distances[v] = bestSignedSquaredDistance;
        m_ClosestTri[v] = bestTriangleId;
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
        progressMessenger.sendProgressMessage(counter);
        counter = 0;
      }
      counter++;
    }
    progressMessenger.sendProgressMessage(counter);
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
  const float32 m_InitialSearchHalfExtent;
  ProgressMessageHelper& m_ProgressMessageHelper;
};

/**
 * @brief Stores one triangle's axis-aligned bounds.
 * @param triList Provides triangle vertex indexes.
 * @param vertList Provides vertex positions.
 * @param triId Identifies the triangle.
 * @param bounds Receives [xmin, ymin, zmin, xmax, ymax, zmax].
 */
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

ComputeVertexToTriangleDistances::ComputeVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   ComputeVertexToTriangleDistancesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeVertexToTriangleDistances::~ComputeVertexToTriangleDistances() noexcept = default;

const std::atomic_bool& ComputeVertexToTriangleDistances::getCancel()
{
  return m_ShouldCancel;
}

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
  // The largest bound extent seeds the first candidate box and reduces later
  // expansion attempts.
  std::vector<float> triBoundsArray(numTris * 6, 0.0F);
  float32 initialSearchHalfExtent = 0.0f;
  for(size_t triIndex = 0; triIndex < numTris; triIndex++)
  {
    GetBoundingBoxAtTri(triangles, vertices, triIndex, {triBoundsArray.data() + (6 * triIndex), 6});
    m_RTree.Insert(triBoundsArray.data() + (6 * triIndex), triBoundsArray.data() + (6 * triIndex) + 3, triIndex);

    const float32 extentX = triBoundsArray[6 * triIndex + 3] - triBoundsArray[6 * triIndex + 0];
    const float32 extentY = triBoundsArray[6 * triIndex + 4] - triBoundsArray[6 * triIndex + 1];
    const float32 extentZ = triBoundsArray[6 * triIndex + 5] - triBoundsArray[6 * triIndex + 2];
    initialSearchHalfExtent = std::max({initialSearchHalfExtent, extentX, extentY, extentZ});
  }
  if(initialSearchHalfExtent <= 0.0f)
  {
    // A nonzero seed lets the search start for zero-area triangles.
    initialSearchHalfExtent = 1.0f;
  }

  const auto& normalsArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->TriangleNormalsArrayPath)->getDataStoreRef();
  auto& distancesArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->DistancesArrayPath)->getDataStoreRef();
  distancesArray.fill(std::numeric_limits<float32>::max());
  auto& closestTriangleIdsArray = m_DataStructure.getDataAs<Int64Array>(m_InputValues->ClosestTriangleIdArrayPath)->getDataStoreRef();
  closestTriangleIdsArray.fill(-1); // No closest triangle found.

  MessageHelper messageHelper(m_MessageHandler);
  ProgressMessageHelper progressMessageHelper = messageHelper.createProgressMessageHelper();
  progressMessageHelper.setMaxProgresss(totalElements);
  progressMessageHelper.setProgressMessageTemplate("Finding Distances || {:.2f}% Completed");

  // This remains direct parallel DataStore access. See the worker limitation.
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(true);
  dataAlg.setRange(0, totalElements);
  dataAlg.execute(
      ComputeVertexToTriangleDistancesImpl(this, triangles, vertices, sourceVertices, distancesArray, closestTriangleIdsArray, normalsArray, m_RTree, initialSearchHalfExtent, progressMessageHelper));

  return {};
}
