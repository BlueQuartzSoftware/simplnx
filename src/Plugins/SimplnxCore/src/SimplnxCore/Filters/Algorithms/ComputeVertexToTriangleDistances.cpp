#include "ComputeVertexToTriangleDistances.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/RTree.hpp"

using RTreeType = RTree<size_t, float, 3, float>;

using namespace nx::core;

namespace
{

/**
 * @brief 3x1 Matrix as a row.
 * @tparam
 */
template <typename T>
class Matrix3X1

{
public:
  using SelfType = Matrix3X1<T>;

  /**
   * @brief Default constructor will create [0,0,0] matrix.
   */
  Matrix3X1() = default;

  /**
   * @brief Copies the values into the matrix
   * @param v0
   * @param v1
   * @param v2
   */
  Matrix3X1(T v0, T v1, T v2)
  : m_Data(std::array<T, 3>{v0, v1, v2})
  {
  }

  Matrix3X1(const Matrix3X1&) = default;                // Copy Constructor Default Implemented
  Matrix3X1(Matrix3X1&&) noexcept = default;            // Move Constructor Default Implemented
  Matrix3X1& operator=(const Matrix3X1&) = default;     // Copy Assignment Default Implemented
  Matrix3X1& operator=(Matrix3X1&&) noexcept = default; // Move Assignment Default Implemented

  ~Matrix3X1() = default;

  /**
   * @brief Returns a reference to the value at index
   * @param index
   * @return
   */
  T& operator[](size_t index)
  {
    return m_Data[index]; // No bounds checking.
  }

  /**
   * @brief Returns a reference to the value at index
   * @param index
   * @return
   */
  const T& operator[](size_t index) const
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the pointer to the underlying array
   * @return
   */
  T* data()
  {
    return m_Data.data();
  }

  /**
   * @brief Performs the Matrix Addition.
   * @param rhs
   * @return result
   */
  SelfType operator+(const SelfType& rhs) const
  {
    return {m_Data[0] + rhs[0], m_Data[1] + rhs[1], m_Data[2] + rhs[2]};
  }

  /**
   * @brief Performs the Matrix Subtraction
   * @param rhs
   * @return outMat result
   */
  SelfType operator-(const SelfType& rhs) const
  {
    return {m_Data[0] - rhs[0], m_Data[1] - rhs[1], m_Data[2] - rhs[2]};
  }

  /**
   * @brief Multiplies each element of a 3x1 matrix by the value scalar.
   * @param scalar Value to multiply each element by.
   */
  SelfType operator*(T scalar) const
  {
    return {m_Data[0] * scalar, m_Data[1] * scalar, m_Data[2] * scalar};
  }

  /**
   * @brief Performs an "in place" normalization of the 3x1 vector.
   * @param g
   */
  SelfType normalize()
  {
    SelfType outMat = this;

    T denominator = outMat[0] * outMat[0] + outMat[1] * outMat[1] + outMat[2] * outMat[2];
    denominator = sqrt(denominator);
    outMat[0] = outMat[0] / denominator;
    if(outMat[0] > 1.0)
    {
      outMat[0] = 1.0;
    }
    outMat[1] = outMat[1] / denominator;
    if(outMat[1] > 1.0)
    {
      outMat[1] = 1.0;
    }
    outMat[2] = outMat[2] / denominator;
    if(outMat[2] > 1.0)
    {
      outMat[2] = 1.0;
    }
    return outMat;
  }

  /**
   * @brief Performs an "in place" normalization of the 3x1 vector
   * @param i
   * @param j
   * @param k
   */
  static bool normalize(T& i, T& j, T& k)
  {
    T denominator;
    denominator = std::sqrt(((i * i) + (j * j) + (k * k)));
    if(denominator == 0)
    {
      return false;
    }
    i = i / denominator;
    j = j / denominator;
    k = k / denominator;
  }

  /**
   * @brief The dot product of 2 vectors a & b
   * @param a 1x3 Vector
   * @param b 1x3 Vector
   * @return
   */
  T dot(const SelfType& b) const
  {
    return (m_Data[0] * b[0] + m_Data[1] * b[1] + m_Data[2] * b[2]);
  }

  /**
   * @brief Performs a Cross Product of "this into b" and returns ths result.
   * A X B = C
   * @param b
   * @return
   */

  SelfType cross(const SelfType& b)
  {
    SelfType c;
    c[0] = m_Data[1] * b[2] - m_Data[2] * b[1];
    c[1] = m_Data[2] * b[0] - m_Data[0] * b[2];
    c[2] = m_Data[0] * b[1] - m_Data[1] * b[0];
    return c;
  }

  /**
   * @brief Finds the cosine of the angle Theta between this vector and another vector
   * @param vectorB
   * @return
   */
  float cosThetaBetweenVectors(const SelfType& vectorB) const
  {
    float32 norm1 = sqrtf(m_Data[0] * m_Data[0] + m_Data[1] * m_Data[1] + m_Data[2] * m_Data[2]);
    float32 norm2 = sqrtf(vectorB[0] * vectorB[0] + vectorB[1] * vectorB[1] + vectorB[2] * vectorB[2]);
    if(norm1 == 0 || norm2 == 0)
    {
      return 1.0;
    }
    return (m_Data[0] * vectorB[0] + m_Data[1] * vectorB[1] + m_Data[2] * vectorB[2]) / (norm1 * norm2);
  }

private:
  std::array<T, 3> m_Data = {0.0, 0.0, 0.0};
};

using Vec3fa = Matrix3X1<float>;

Vec3fa operator*(const float scalar, const Vec3fa& rhs)
{
  return {rhs[0] * scalar, rhs[1] * scalar, rhs[2] * scalar};
}

Vec3fa operator*(const Vec3fa& rhs, const float scalar)
{
  return {rhs[0] * scalar, rhs[1] * scalar, rhs[2] * scalar};
}

/**
 * @brief Take from https://github.com/embree/embree/blob/master/tutorials/common/math/closest_point.h
 * Which has an apache license.
 * @param p
 * @param a
 * @param b
 * @param c
 * @return
 */
Vec3fa closestPointTriangle(const Vec3fa& p, const Vec3fa& a, const Vec3fa& b, const Vec3fa& c)
{
  const Vec3fa ab = b - a;
  const Vec3fa ac = c - a;
  const Vec3fa ap = p - a;

  const float d1 = ab.dot(ap); // dot(ab, ap);
  const float d2 = ac.dot(ap); // dot(ac, ap);
  if(d1 <= 0.f && d2 <= 0.f)
  {
    return a;
  }

  const Vec3fa bp = p - b;
  const float d3 = ab.dot(bp); // dot(ab, bp);
  const float d4 = ac.dot(bp); // dot(ac, bp);
  if(d3 >= 0.f && d4 <= d3)
  {
    return b;
  }

  const Vec3fa cp = p - c;
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
  const Vec3fa pointInTriangle = a + v * ab + w * ac;

  return pointInTriangle;
}

float32 PointTriangleDistance(const Vec3fa& point, const Vec3fa& vert0, const Vec3fa& vert1, const Vec3fa& vert2, const int64 triangle, const Float64Array& normals)
{

  Vec3fa closestPointInTriangle = closestPointTriangle(point, vert0, vert1, vert2);

  auto diffPoint = point - closestPointInTriangle; // Gives a vector pointing from the closest point in triangle to point
  // Only do the dot-product of the vector with itself, so we don't incur the penalty of a square root that we might not need
  float dist = diffPoint.dot(diffPoint);

  Vec3fa normal = {static_cast<float32>(normals[3 * triangle + 0]), static_cast<float32>(normals[3 * triangle + 1]), static_cast<float32>(normals[3 * triangle + 2])};

  float32 cosTheta = normal.cosThetaBetweenVectors(diffPoint);

  if(cosTheta < 0.0f)
  {
    dist *= -1.0f;
  }

  return dist;
}

class ComputeVertexToTriangleDistancesImpl
{
public:
  ComputeVertexToTriangleDistancesImpl(ComputeVertexToTriangleDistances* filter, const IGeometry::SharedTriList& triangles, const IGeometry::SharedVertexList& vertices,
                                       IGeometry::SharedVertexList& sourcePoints, Float32Array& distances, Int64Array& closestTri, const Float64Array& normals, const RTreeType rtree)
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
      Vec3fa sourcePoint(m_SourcePoints[3 * v], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]);

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
          const Vec3fa point = {m_SourcePoints[3 * v + 0], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]};
          const Vec3fa v0(m_TriangleVertices[p * 3 + 0], m_TriangleVertices[p * 3 + 1], m_TriangleVertices[p * 3 + 2]);
          const Vec3fa v1(m_TriangleVertices[q * 3 + 0], m_TriangleVertices[q * 3 + 1], m_TriangleVertices[q * 3 + 2]);
          const Vec3fa v2(m_TriangleVertices[r * 3 + 0], m_TriangleVertices[r * 3 + 1], m_TriangleVertices[r * 3 + 2]);

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
          const Vec3fa point = {m_SourcePoints[3 * v + 0], m_SourcePoints[3 * v + 1], m_SourcePoints[3 * v + 2]};
          const Vec3fa v0(m_TriangleVertices[p * 3 + 0], m_TriangleVertices[p * 3 + 1], m_TriangleVertices[p * 3 + 2]);
          const Vec3fa v1(m_TriangleVertices[q * 3 + 0], m_TriangleVertices[q * 3 + 1], m_TriangleVertices[q * 3 + 2]);
          const Vec3fa v2(m_TriangleVertices[r * 3 + 0], m_TriangleVertices[r * 3 + 1], m_TriangleVertices[r * 3 + 2]);

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
  const IGeometry::SharedTriList& m_SharedTriangleList;
  const IGeometry::SharedVertexList& m_TriangleVertices;
  IGeometry::SharedVertexList& m_SourcePoints;
  Float32Array& m_Distances;
  Int64Array& m_ClosestTri;
  const Float64Array& m_Normals;
  const RTreeType m_RTree;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeVertexToTriangleDistances::ComputeVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   ComputeVertexToTriangleDistancesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
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

  m_ProgressCounter += counter;
  auto progressInt = static_cast<size_t>((static_cast<float32>(m_ProgressCounter) / static_cast<float32>(m_TotalElements)) * 100.0f);

  size_t progIncrement = m_TotalElements / 100;

  if(m_ProgressCounter > 1 && m_LastProgressInt != progressInt)
  {
    std::string ss = fmt::format("Finding Distances || {}% Completed", progressInt);
    m_MessageHandler(IFilter::Message::Type::Info, ss);
  }

  m_LastProgressInt = progressInt;
}

void GetBoundingBoxAtTri(const IGeometry::SharedTriList& triList, const IGeometry::SharedVertexList& vertList, size_t triId, nonstd::span<float> bounds)
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

// -----------------------------------------------------------------------------
Result<> ComputeVertexToTriangleDistances::operator()()
{
  auto& vertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->VertexDataContainer);
  IGeometry::SharedVertexList& sourceVertices = vertexGeom.getVerticesRef();
  m_TotalElements = vertexGeom.getNumberOfVertices();

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleDataContainer);
  auto numTris = static_cast<usize>(triangleGeom.getNumberOfFaces());
  IGeometry::SharedTriList& triangles = triangleGeom.getFacesRef();
  IGeometry::SharedVertexList& vertices = triangleGeom.getVerticesRef();

  RTreeType m_RTree;
  // Populate the RTree
  std::vector<float> triBoundsArray(numTris * 6, 0.0F);
  for(size_t triIndex = 0; triIndex < numTris; triIndex++)
  {
    GetBoundingBoxAtTri(triangles, vertices, triIndex, {triBoundsArray.data() + (6 * triIndex), 6});
    m_RTree.Insert(triBoundsArray.data() + (6 * triIndex), triBoundsArray.data() + (6 * triIndex) + 3, triIndex); // Note, all values including zero are fine in this version
  }

  const auto& normalsArray = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->TriangleNormalsArrayPath);
  auto& distancesArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->DistancesArrayPath);
  distancesArray.fill(std::numeric_limits<float32>::max());
  auto& closestTriangleIdsArray = m_DataStructure.getDataRefAs<Int64Array>(m_InputValues->ClosestTriangleIdArrayPath);
  closestTriangleIdsArray.fill(-1); // -1 means it never found the closest triangle?

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(true);
  dataAlg.setRange(0, m_TotalElements);
  dataAlg.execute(ComputeVertexToTriangleDistancesImpl(this, triangles, vertices, sourceVertices, distancesArray, closestTriangleIdsArray, normalsArray, m_RTree));

  return {};
}
