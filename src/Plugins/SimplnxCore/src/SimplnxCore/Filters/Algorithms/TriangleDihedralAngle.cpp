#include "TriangleDihedralAngle.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
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

constexpr usize k_DihedralBlockTriangles = 262144;
constexpr usize k_DihedralFastBytesPerFace = (3 * sizeof(IGeometry::MeshIndexType)) + sizeof(float64);
constexpr usize k_DihedralGatherBytesPerFace = k_DihedralFastBytesPerFace + (3 * sizeof(usize)) + (18 * sizeof(float32));
static_assert(k_DihedralBlockTriangles <= std::numeric_limits<usize>::max() / k_DihedralGatherBytesPerFace);

struct TriangleDihedralWorkingPlan
{
  bool CacheVertices = false;
  usize FaceCapacity = 0;
  usize PageVertices = 0;
  usize PayloadBytes = 0;
};

/** @brief Plans named buffers after the caller checks addressable vertex and face shapes. */
TriangleDihedralWorkingPlan planDihedralWorkingMemory(const AbstractDataStore<IGeometry::MeshIndexType>& triangles, const Float32AbstractDataStore& vertices, usize availableBytes)
{
  const usize maximumTriangles = std::min(k_DihedralBlockTriangles, triangles.getNumberOfTuples());
  const usize vertexCount = vertices.getNumberOfTuples();
  const usize vertexBytes = vertexCount * 3 * sizeof(float32);
  const usize gatherTriangles = std::min(maximumTriangles, availableBytes / k_DihedralGatherBytesPerFace);
  const usize admissionTriangles = std::max(usize{1}, gatherTriangles);
  // Retaining coordinates must preserve the gathered route's available face capacity.
  const bool cacheVertices = vertexBytes <= availableBytes && admissionTriangles * k_DihedralFastBytesPerFace <= availableBytes - vertexBytes;
  const usize faceCapacity = cacheVertices ? std::min(maximumTriangles, (availableBytes - vertexBytes) / k_DihedralFastBytesPerFace) : gatherTriangles;
  if(faceCapacity == 0)
  {
    return {};
  }
  const usize pageVertices = cacheVertices ? 0 : std::min(vertexCount, faceCapacity * 3);
  const usize coordinateBytes = cacheVertices ? vertexBytes : pageVertices * 3 * sizeof(float32);
  const usize bytesPerFace = k_DihedralFastBytesPerFace + (cacheVertices ? 0 : (3 * sizeof(usize)) + (9 * sizeof(float32)));
  if(coordinateBytes > availableBytes || faceCapacity > (availableBytes - coordinateBytes) / bytesPerFace)
  {
    return {};
  }
  return {.CacheVertices = cacheVertices, .FaceCapacity = faceCapacity, .PageVertices = pageVertices, .PayloadBytes = coordinateBytes + (faceCapacity * bytesPerFace)};
}

Result<> CalculateDihedralAnglesInBlocks(const TriangleGeom& triangleGeom, Float64AbstractDataStore& dihedralAngles, const std::atomic_bool& shouldCancel)
{
  const auto& triangles = triangleGeom.getFaces()->getDataStoreRef();
  const auto& verts = triangleGeom.getVertices()->getDataStoreRef();
  const usize numTriangles = triangles.getNumberOfTuples();
  const usize numVertices = verts.getNumberOfTuples();
  if(shouldCancel || numTriangles == 0)
  {
    return {};
  }

  constexpr usize maximum = std::numeric_limits<usize>::max();
  const usize maximumTriangles = std::min(k_DihedralBlockTriangles, numTriangles);
  if(verts.getNumberOfComponents() != 3 || triangles.getNumberOfComponents() != 3 || numVertices > maximum / (3 * sizeof(float32)) || numTriangles > maximum / 3 ||
     numVertices * (3 * sizeof(float32)) > maximum - maximumTriangles * k_DihedralFastBytesPerFace)
  {
    return MakeErrorResult(-65824, "Triangle dihedral-angle buffers require addressable three-component vertex and face arrays. Reduce the mesh size or repair the array shapes.");
  }
  const usize vertexBytes = numVertices * 3 * sizeof(float32);
  const usize desiredBytes = vertexBytes + (maximumTriangles * k_DihedralFastBytesPerFace);
  auto& budget = CacheMemoryBudgetManager::instance();
  const uint64 maximumWorkingBytes = budget.maximumWorkingMemoryBytes();
  const uint64 reservedBytes = budget.reservedWorkingMemoryBytes();
  const usize observedHeadroom = static_cast<usize>(std::min<uint64>(maximumWorkingBytes > reservedBytes ? maximumWorkingBytes - reservedBytes : 0, maximum));
  const auto observedPlan = planDihedralWorkingMemory(triangles, verts, observedHeadroom);
  const usize preferredBytes = std::min(desiredBytes, maximumTriangles * k_DihedralGatherBytesPerFace);
  const uint64 requestedBytes = std::min<uint64>(observedPlan.FaceCapacity == 0 ? preferredBytes : observedPlan.PayloadBytes, budget.maximumWorkingMemoryBytes());
  // A snapshot avoids known excess admission; a racing reservation can still reduce the actual grant.
  auto reservation = budget.reserveWorkingMemory(requestedBytes);
  const usize grantedBytes = static_cast<usize>(std::min<uint64>(reservation.sizeBytes(), maximum));
  const auto plan = planDihedralWorkingMemory(triangles, verts, grantedBytes);
  const bool cacheVertices = plan.CacheVertices;
  const usize bufferTriangles = plan.FaceCapacity;
  if(bufferTriangles == 0)
  {
    return MakeErrorResult(
        -65825,
        fmt::format("Triangle dihedral-angle calculation requested {} working bytes and received {}. At least {} bytes are needed for one gathered face, or {} bytes for cached vertices and one face.",
                    requestedBytes, grantedBytes, k_DihedralGatherBytesPerFace, vertexBytes + k_DihedralFastBytesPerFace));
  }
  const usize pageVertices = plan.PageVertices;
  // The checked plan fits the actual grant; release any remainder before allocations or I/O.
  reservation.shrinkTo(plan.PayloadBytes);
  Result<> result;
  try
  {
    std::vector<float32> vertices(cacheVertices ? numVertices * 3 : 0);
    if(cacheVertices)
    {
      result = MergeResults(std::move(result), verts.copyIntoBuffer(0, {vertices.data(), vertices.size()}));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-65820, "Error reading vertex coordinates for triangle dihedral angle calculation."));
      }
    }
    std::vector<usize> order(cacheVertices ? 0 : bufferTriangles * 3);
    std::vector<float32> gathered(cacheVertices ? 0 : bufferTriangles * 9);
    std::vector<float32> page(pageVertices * 3);
    std::vector<IGeometry::MeshIndexType> triangleBuffer(bufferTriangles * 3);
    std::vector<float64> angleBuffer(bufferTriangles);
    for(usize start = 0; start < numTriangles;)
    {
      if(shouldCancel)
      {
        return result;
      }
      const usize count = std::min(bufferTriangles, numTriangles - start);
      result = MergeResults(std::move(result), triangles.copyIntoBuffer(start * 3, nonstd::span<IGeometry::MeshIndexType>(triangleBuffer.data(), count * 3)));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-65821, fmt::format("Error reading connectivity for triangle dihedral angle calculation at triangle {} ({} triangles).", start, count)));
      }

      for(usize triangleIndex = 0; triangleIndex < count; triangleIndex++)
      {
        for(usize vertex = 0; vertex < 3; ++vertex)
        {
          const auto vertexIndex = triangleBuffer[(triangleIndex * 3) + vertex];
          if(vertexIndex >= numVertices)
          {
            return MergeResults(std::move(result),
                                MakeErrorResult(-65823, fmt::format("Triangle {} references vertex index {}, but the vertex tuple count is {}. Valid vertex indices are less than {}.",
                                                                    start + triangleIndex, vertexIndex, numVertices, numVertices)));
          }
        }
      }
      if(!cacheVertices)
      {
        result = MergeResults(std::move(result), MeshingUtilities::detail::GatherVertexCoordinates(verts, {triangleBuffer.data(), count * 3}, {order.data(), count * 3}, {gathered.data(), count * 9},
                                                                                                   {page.data(), page.size()}));
        if(result.invalid())
        {
          return MergeResults(std::move(result),
                              MakeErrorResult(-65820, fmt::format("Error reading vertex coordinates for triangle dihedral-angle calculation at triangle {} ({} triangles).", start, count)));
        }
      }
      nx::core::Vec3<float64> vecAB = {0.0f, 0.0f, 0.0f};
      nx::core::Vec3<float64> vecAC = {0.0f, 0.0f, 0.0f};
      nx::core::Vec3<float64> vecBC = {0.0f, 0.0f, 0.0f};
      for(usize triangleIndex = 0; triangleIndex < count; triangleIndex++)
      {
        std::array<Point3Df, 3> vertCoords;
        for(usize vertex = 0; vertex < 3; ++vertex)
        {
          const usize offset = cacheVertices ? triangleBuffer[(triangleIndex * 3) + vertex] * 3 : (triangleIndex * 9) + (vertex * 3);
          const auto& coordinates = cacheVertices ? vertices : gathered;
          vertCoords.at(vertex) = Point3Df{coordinates[offset], coordinates[offset + 1], coordinates[offset + 2]};
        }
        // Keep float coordinate subtraction and the exact in-core operation order.
        for(usize i = 0; i < 3; i++)
        {
          vecAB[i] = vertCoords[0][i] - vertCoords[1][i];
          vecAC[i] = vertCoords[0][i] - vertCoords[2][i];
          vecBC[i] = vertCoords[1][i] - vertCoords[2][i];
        }

        const float64 magAB = vecAB.magnitude();
        const float64 magAC = vecAC.magnitude();
        const float64 magBC = vecBC.magnitude();

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

          angleBuffer[triangleIndex] = *std::ranges::min_element(dihedralAnglesVec);
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
  } catch(const std::bad_alloc&)
  {
    return MergeResults(std::move(result), MakeErrorResult(-65826, "Triangle dihedral-angle calculation could not allocate its admitted working buffers. Reduce the mesh size or free memory."));
  } catch(const std::length_error&)
  {
    return MergeResults(std::move(result), MakeErrorResult(-65826, "Triangle dihedral-angle calculation exceeded the supported buffer allocation size. Reduce the mesh size."));
  }
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
