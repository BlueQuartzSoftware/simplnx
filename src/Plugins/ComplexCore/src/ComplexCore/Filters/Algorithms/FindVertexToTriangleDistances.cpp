#include "FindVertexToTriangleDistances.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{
float distanceFromVec(const std::vector<float>& vec1, const std::vector<float>& vec2)
{
  // assumption: vectors are of the same length
  float dist = 0.0f;
  for(size_t i = 0; i < vec1.size(); i++)
  {
    dist += (vec1[i] - vec2[i]) * (vec1[i] - vec2[i]);
  }

  //  return std::sqrt(dist); // why is this here
  return dist;
}

float32 PointSegmentDistance(const std::vector<float32>& x0, const std::vector<float32>& x1, const std::vector<float32>& x2)
{
  std::vector<float32> dx = {0.0f, 0.0f, 0.0f};
  std::transform(x2.begin(), x2.end(), x1.begin(), dx.begin(), std::minus<float32>());

  float64 m2 = 0.0;
  for(usize i = 0; i < dx.size(); i++)
  {
    m2 += static_cast<float64>(dx[i] * dx[i]);
  }

  std::vector<float32> x2minx0 = {0.0f, 0.0f, 0.0f};
  std::transform(x2.begin(), x2.end(), x0.begin(), x2minx0.begin(), std::minus<float32>());

  float64 dotProduct = 0.0;
  for(usize i = 0; i < x2minx0.size(); i++)
  {
    dotProduct += static_cast<float64>(x2minx0[i] * dx[i]);
  }

  float32 s12 = static_cast<float32>(dotProduct / m2);
  if(s12 < 0.0f)
  {
    s12 = 0.0f;
  }
  else if(s12 > 1.0f)
  {
    s12 = 1.0f;
  }

  std::vector<float32> multVal1 = {x1[0] * s12, x1[1] * s12, x1[2] * s12};
  std::vector<float32> mutlVal2 = {x2[0] * (1 - s12), x2[1] * (1 - s12), x2[2] * (1 - s12)};

  std::vector<float32> vecSum = {0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < x1.size(); i++)
  {
    vecSum[i] = multVal1[i] + mutlVal2[i];
  }

  return distanceFromVec(x0, vecSum);
}

float32 CosThetaBetweenVectors(const std::array<float32, 3>& a, const float32 b[3])
{
  float32 norm1 = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  float32 norm2 = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
  if(norm1 == 0 || norm2 == 0)
  {
    return 1.0;
  }
  return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (norm1 * norm2);
}

float32 PointTriangleDistance(const std::vector<float32>& x0, const std::vector<float32>& x1, const std::vector<float32>& x2, const std::vector<float32>& x3, const int64 triangle,
                              const Float64Array& normals)
{
  float32 dist = 0.0f;

  std::array<float32, 3> x13 = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> x23 = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> x03 = {0.0f, 0.0f, 0.0f};
  std::transform(x1.begin(), x1.end(), x3.begin(), x13.begin(), std::minus<float32>());
  std::transform(x2.begin(), x2.end(), x3.begin(), x23.begin(), std::minus<float32>());
  std::transform(x0.begin(), x0.end(), x3.begin(), x03.begin(), std::minus<float32>());

  float32 m13 = 0.0f;
  float32 m23 = 0.0f;
  for(usize i = 0; i < x13.size(); i++)
  {
    m13 += (x13[i] * x13[i]);
  }
  for(usize i = 0; i < x23.size(); i++)
  {
    m23 += (x23[i] * x23[i]);
  }
  float32 d = 0.0;
  for(usize i = 0; i < x13.size(); i++)
  {
    d += (x13[i] * x23[i]);
  }
  float32 invdet = 1.0f / std::max(m13 * m23 - d * d, 1e-30f);
  float32 a = 0.0f;
  float32 b = 0.0f;
  for(usize i = 0; i < x13.size(); i++)
  {
    a += (x13[i] * x03[i]);
    b += (x23[i] * x03[i]);
  }

  float32 w23 = invdet * (m23 * a - d * b);
  float32 w31 = invdet * (m13 * b - d * a);
  float32 w12 = 1 - w23 - w31;

  if(w23 >= 0.0f && w31 >= 0.0f && w12 >= 0.0f)
  {
    std::vector<float32> tmpVec = {0.0f, 0.0f, 0.0f};
    for(usize i = 0; i < tmpVec.size(); i++)
    {
      tmpVec[i] = (w23 * x1[i]) + (w31 * x2[i]) + (w12 * x3[i]);
    }

    dist = distanceFromVec(x0, tmpVec);
  }
  else
  {
    if(w23 > 0)
    {
      dist = std::min(PointSegmentDistance(x0, x1, x2), PointSegmentDistance(x0, x1, x3));
    }
    else if(w31 > 0)
    {
      dist = std::min(PointSegmentDistance(x0, x1, x2), PointSegmentDistance(x0, x2, x3));
    }
    else
    {
      dist = std::min(PointSegmentDistance(x0, x1, x3), PointSegmentDistance(x0, x2, x3));
    }
  }

  std::array<float32, 3> normal = {static_cast<float32>(normals[3 * triangle + 0]), static_cast<float32>(normals[3 * triangle + 1]), static_cast<float32>(normals[3 * triangle + 2])};

  float32 cosTheta = CosThetaBetweenVectors(normal, x03.data());

  if(cosTheta < 0.0f)
  {
    dist *= -1.0f;
  }

  return dist;
}

class FindVertexToTriangleDistancesImpl
{
public:
  FindVertexToTriangleDistancesImpl(FindVertexToTriangleDistances* filter, std::vector<std::vector<size_t>>& tris, std::vector<std::vector<float>>& verts,
                                    const IGeometry::SharedVertexList& sourceVerts, Float32Array& distances, Int64Array& closestTri, size_t numTris, Float32Array& triBounds,
                                    const Float64Array& normals)
  : m_Filter(filter)
  , m_Tris(tris)
  , m_Verts(verts)
  , m_SourceVerts(sourceVerts)
  , m_Distances(distances)
  , m_ClosestTri(closestTri)
  , m_NumTris(numTris)
  , m_TriBounds(triBounds)
  , m_Normals(normals)
  {
  }
  virtual ~FindVertexToTriangleDistancesImpl() = default;

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

  void compute(usize start, usize end) const
  {
    int64 counter = 0;
    int64 totalElements = end - start;
    int64 progIncrement = static_cast<int64>(totalElements / 100);

    for(usize v = start; v < end; v++)
    {
      for(usize t = 0; t < m_NumTris; t++)
      {
        if(m_Filter->getCancel())
        {
          return;
        }
        if((m_SourceVerts[3 * v] >= m_TriBounds[6 * t] && m_SourceVerts[3 * v] <= m_TriBounds[6 * t + 1]) ||
           (m_SourceVerts[3 * v + 1] >= m_TriBounds[6 * t + 2] && m_SourceVerts[3 * v + 1] <= m_TriBounds[6 * t + 3]) ||
           (m_SourceVerts[3 * v + 2] >= m_TriBounds[6 * t + 4] && m_SourceVerts[3 * v + 2] <= m_TriBounds[6 * t + 5]))
        {
          int64 p = m_Tris[t][0];
          int64 q = m_Tris[t][1];
          int64 r = m_Tris[t][2];
          std::vector<float32> gx = {m_SourceVerts[3 * v + 0], m_SourceVerts[3 * v + 1], m_SourceVerts[3 * v + 2]};
          float32 d = PointTriangleDistance(gx, m_Verts[p], m_Verts[q], m_Verts[r], t, m_Normals);
          if(std::abs(d) < std::abs(m_Distances[v]))
          {
            m_Distances[v] = d;
            m_ClosestTri[v] = t;
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
  }

private:
  FindVertexToTriangleDistances* m_Filter;
  std::vector<std::vector<size_t>>& m_Tris;
  std::vector<std::vector<float>>& m_Verts;
  const IGeometry::SharedVertexList& m_SourceVerts;
  Float32Array& m_Distances;
  Int64Array& m_ClosestTri;
  int64_t m_NumTris;
  Float32Array& m_TriBounds;
  const Float64Array& m_Normals;
};
} // namespace

// -----------------------------------------------------------------------------
FindVertexToTriangleDistances::FindVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             FindVertexToTriangleDistancesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindVertexToTriangleDistances::~FindVertexToTriangleDistances() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindVertexToTriangleDistances::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void FindVertexToTriangleDistances::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  size_t progressInt = static_cast<size_t>((static_cast<float32>(m_ProgressCounter) / m_TotalElements) * 100.0f);

  size_t progIncrement = m_TotalElements / 100;

  if(m_ProgressCounter > 1 && m_LastProgressInt != progressInt)
  {
    std::string progressMessage = "Calculating...";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32>(progressInt)});
  }

  m_LastProgressInt = progressInt;
}

// -----------------------------------------------------------------------------
Result<> FindVertexToTriangleDistances::operator()()
{
  auto& vertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->VertexDataContainer);
  IGeometry::SharedVertexList& sourceVerts = vertexGeom.getVerticesRef();
  usize numVerts = vertexGeom.getNumberOfVertices();

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->VertexDataContainer);
  usize numTris = triangleGeom.getNumberOfFaces();
  IGeometry::SharedTriList& triangles = triangleGeom.getFacesRef();
  IGeometry::SharedVertexList& vertices = triangleGeom.getVerticesRef();

  std::vector<std::vector<usize>> tmpTris;
  auto& triBoundsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->TriBoundsDataPath);
  for(size_t i = 0; i < numTris; i++)
  {
    triBoundsArray[6 * i] = std::numeric_limits<float32>::max();
    triBoundsArray[6 * i + 1] = -std::numeric_limits<float32>::max();
    triBoundsArray[6 * i + 2] = std::numeric_limits<float32>::max();
    triBoundsArray[6 * i + 3] = -std::numeric_limits<float32>::max();
    triBoundsArray[6 * i + 4] = std::numeric_limits<float32>::max();
    triBoundsArray[6 * i + 5] = -std::numeric_limits<float32>::max();
    std::vector<usize> tmpTri = {triangles[3 * i + 0], triangles[3 * i + 1], triangles[3 * i + 2]};
    tmpTris.push_back(tmpTri);
    triBoundsArray[6 * i] = std::min({vertices[3 * triangles[3 * i]], vertices[3 * triangles[3 * i + 1]], vertices[3 * triangles[3 * i + 2]]});
    triBoundsArray[6 * i + 1] = std::max({vertices[3 * triangles[3 * i]], vertices[3 * triangles[3 * i + 1]], vertices[3 * triangles[3 * i + 2]]});
    triBoundsArray[6 * i + 2] = std::min({vertices[3 * triangles[3 * i] + 1], vertices[3 * triangles[3 * i + 1] + 1], vertices[3 * triangles[3 * i + 2] + 1]});
    triBoundsArray[6 * i + 3] = std::max({vertices[3 * triangles[3 * i] + 1], vertices[3 * triangles[3 * i + 1] + 1], vertices[3 * triangles[3 * i + 2] + 1]});
    triBoundsArray[6 * i + 4] = std::min({vertices[3 * triangles[3 * i] + 2], vertices[3 * triangles[3 * i + 1] + 2], vertices[3 * triangles[3 * i + 2] + 2]});
    triBoundsArray[6 * i + 5] = std::max({vertices[3 * triangles[3 * i] + 2], vertices[3 * triangles[3 * i + 1] + 2], vertices[3 * triangles[3 * i + 2] + 2]});
  }

  std::vector<std::vector<float32>> tmpVerts;
  for(size_t i = 0; i < triangleGeom.getNumberOfVertices(); i++)
  {
    std::vector<float32> tmpVert = {vertices[3 * i + 0], vertices[3 * i + 1], vertices[3 * i + 2]};
    tmpVerts.push_back(tmpVert);
  }

  auto& normalsArray = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->TriangleNormalsArrayPath);
  auto& distancesArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->DistancesArrayPath);
  auto& closestTriangleIdsArray = m_DataStructure.getDataRefAs<Int64Array>(m_InputValues->ClosestTriangleIdArrayPath);

  // Allow data-based parallelization
  m_TotalElements = numVerts;

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numVerts);
  dataAlg.execute(FindVertexToTriangleDistancesImpl(this, tmpTris, tmpVerts, sourceVerts, distancesArray, closestTriangleIdsArray, numTris, triBoundsArray, normalsArray));

  return {};
}
