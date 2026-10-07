#include "TriangleDihedralAngle.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <vector>

using namespace nx::core;

namespace
{
constexpr float64 k_radToDeg = Constants::k_180OverPiD; // used for translating radians to degrees

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateDihedralAnglesImpl
{
public:
  CalculateDihedralAnglesImpl(const TriangleGeom* triangleGeom, Float64AbstractDataStore& dihedralAngles, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_DihedralAngles(dihedralAngles)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateDihedralAnglesImpl() = default;

  void generate(size_t start, size_t end) const
  {
    // std::array<float64, 3> vectorEx = {x, y, z};  // coordinate example
    nx::core::Vec3<float64> vecAB = {0.0f, 0.0f, 0.0f};
    nx::core::Vec3<float64> vecAC = {0.0f, 0.0f, 0.0f};
    nx::core::Vec3<float64> vecBC = {0.0f, 0.0f, 0.0f};

    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      std::array<Point3Df, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);

      for(usize i = 0; i < 3; i++)
      {
        vecAB[i] = vertCoords[0][i] - vertCoords[1][i];
        vecAC[i] = vertCoords[0][i] - vertCoords[2][i];
        vecBC[i] = vertCoords[1][i] - vertCoords[2][i];
      }

      float64 magAB = vecAB.magnitude();
      float64 magAC = vecAC.magnitude();
      float64 magBC = vecBC.magnitude();

      if(magAB == 0.0f || magAC == 0.0f || magBC == 0.0f)
      {
        m_DihedralAngles[triangleIndex] = std::nan("0");
      }
      else
      {
        std::vector<float64> dihedralAnglesVec;
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(vecAB.dot(vecAC)) / (magAB * magAC))));

        // 180 - angle because AB points out of vertex and BC points into vertex, so angle is actually angle outside of triangle
        dihedralAnglesVec.push_back(180.0 - (k_radToDeg * acos((std::fabs(vecAB.dot(vecBC)) / (magAB * magBC)))));
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(vecBC.dot(vecAC)) / (magBC * magAC))));

        m_DihedralAngles[triangleIndex] = *std::min_element(dihedralAnglesVec.begin(), dihedralAnglesVec.end());
      }

      if(m_ShouldCancel)
      {
        return;
      }
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float64AbstractDataStore& m_DihedralAngles;
  const std::atomic_bool& m_ShouldCancel;
};

Result<> CalculateDihedralAnglesInBlocks(const TriangleGeom& triangleGeom, Float64AbstractDataStore& dihedralAngles, const std::atomic_bool& shouldCancel)
{
  const auto& triangles = triangleGeom.getFaces()->getDataStoreRef();
  const auto& verts = triangleGeom.getVertices()->getDataStoreRef();
  const usize numTriangles = triangles.getNumberOfTuples();
  if(shouldCancel || numTriangles == 0)
  {
    return {};
  }

  // Match CalculateNormalsInBlocks: cache 3 * N floats for random vertex access.
  std::vector<float32> vertices(verts.getSize());
  Result<> result;
  result = MergeResults(std::move(result), verts.copyIntoBuffer(0, nonstd::span<float32>(vertices.data(), vertices.size())));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-65820, "Error reading vertex coordinates for triangle dihedral angle calculation."));
  }

  // Connectivity and scalar angles use at most 8 MiB with 64-bit mesh indexes.
  constexpr usize k_BlockTriangles = 262144;
  const usize bufferTriangles = std::min(k_BlockTriangles, numTriangles);
  std::vector<IGeometry::MeshIndexType> triangleBuffer(bufferTriangles * 3);
  std::vector<float64> angleBuffer(bufferTriangles);
  for(usize start = 0; start < numTriangles;)
  {
    if(shouldCancel)
    {
      return result;
    }
    const usize count = std::min(k_BlockTriangles, numTriangles - start);
    result = MergeResults(std::move(result), triangles.copyIntoBuffer(start * 3, nonstd::span<IGeometry::MeshIndexType>(triangleBuffer.data(), count * 3)));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-65821, fmt::format("Error reading connectivity for triangle dihedral angle calculation at triangle {} ({} triangles).", start, count)));
    }

    nx::core::Vec3<float64> vecAB = {0.0f, 0.0f, 0.0f};
    nx::core::Vec3<float64> vecAC = {0.0f, 0.0f, 0.0f};
    nx::core::Vec3<float64> vecBC = {0.0f, 0.0f, 0.0f};
    for(usize triangleIndex = 0; triangleIndex < count; triangleIndex++)
    {
      std::array<Point3Df, 3> vertCoords;
      for(usize vertex = 0; vertex < 3; ++vertex)
      {
        const usize offset = triangleBuffer[triangleIndex * 3 + vertex] * 3;
        vertCoords[vertex] = Point3Df{vertices[offset], vertices[offset + 1], vertices[offset + 2]};
      }
      // Keep float coordinate subtraction and the exact in-core operation order.
      for(usize i = 0; i < 3; i++)
      {
        vecAB[i] = vertCoords[0][i] - vertCoords[1][i];
        vecAC[i] = vertCoords[0][i] - vertCoords[2][i];
        vecBC[i] = vertCoords[1][i] - vertCoords[2][i];
      }

      float64 magAB = vecAB.magnitude();
      float64 magAC = vecAC.magnitude();
      float64 magBC = vecBC.magnitude();

      if(magAB == 0.0f || magAC == 0.0f || magBC == 0.0f)
      {
        angleBuffer[triangleIndex] = std::nan("0");
      }
      else
      {
        std::vector<float64> dihedralAnglesVec;
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(vecAB.dot(vecAC)) / (magAB * magAC))));

        // 180 - angle because AB points out of vertex and BC points into vertex, so angle is actually angle outside of triangle
        dihedralAnglesVec.push_back(180.0 - (k_radToDeg * acos((std::fabs(vecAB.dot(vecBC)) / (magAB * magBC)))));
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(vecBC.dot(vecAC)) / (magBC * magAC))));

        angleBuffer[triangleIndex] = *std::min_element(dihedralAnglesVec.begin(), dihedralAnglesVec.end());
      }
    }
    result = MergeResults(std::move(result), dihedralAngles.copyFromBuffer(start, nonstd::span<const float64>(angleBuffer.data(), count)));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-65822, fmt::format("Error writing dihedral angles at triangle {} ({} triangles).", start, count)));
    }
    start += count;
  }
  return result;
}
} // namespace

// -----------------------------------------------------------------------------
TriangleDihedralAngle::TriangleDihedralAngle(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             TriangleDihedralAngleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TriangleDihedralAngle::~TriangleDihedralAngle() noexcept = default;

// -----------------------------------------------------------------------------
Result<> TriangleDihedralAngle::operator()()
{
  auto pTriangleGeometryDataPath = m_InputValues->InputTriangleGeometryPath;
  auto pMinDihedralAnglesName = m_InputValues->SurfaceMeshTriangleDihedralAnglesArrayName;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom.getFaceAttributeMatrix();
  const DataPath dihedralAnglesArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pMinDihedralAnglesName);
  auto& dihedralAnglesRef = m_DataStructure.getDataAs<Float64Array>(dihedralAnglesArrayPath)->getDataStoreRef();

  const bool usesOutOfCoreStore = AnyOutOfCore({triangleGeom.getFaces(), triangleGeom.getVertices(), m_DataStructure.getDataAs<Float64Array>(dihedralAnglesArrayPath)});
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    return CalculateDihedralAnglesInBlocks(triangleGeom, dihedralAnglesRef, m_ShouldCancel);
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(CalculateDihedralAnglesImpl(&triangleGeom, dihedralAnglesRef, m_ShouldCancel));

  return {};
}
