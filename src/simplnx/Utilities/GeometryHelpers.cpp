#include "GeometryHelpers.hpp"

using namespace nx::core;
namespace nx::core::GeometryHelpers::Description
{
std::string GenerateGeometryInfo(const nx::core::SizeVec3& dims, const nx::core::FloatVec3& spacing, const nx::core::FloatVec3& origin, IGeometry::LengthUnit units)
{
  std::stringstream description;

  std::string unitStr;
  if(units != IGeometry::LengthUnit::Unknown && units != IGeometry::LengthUnit::Unspecified)
  {
    unitStr = IGeometry::LengthUnitToString(units);
  }

  std::array<std::string, 3> label = {"X", "Y", "Z"};

  for(size_t i = 0; i < 3; i++)
  {
    description << label[i] << " Bounds: " << origin[i] << " to " << (origin[i] + (static_cast<float>(dims[i]) * spacing[i])) << " (Delta: " << (dims[i] * spacing[i]) << ") " << unitStr
                << "    |  Extent: " << 0 << "-" << dims[i] - 1 << "    |  Dims: " << dims[i] << " Voxels    |  Spacing: " << spacing[i] << "\n";
  }

  return description.str();
}
} // namespace nx::core::GeometryHelpers::Description

namespace nx::core::GeometryHelpers::Connectivity
{
std::vector<int32> FindEulerCharacteristicValues(const TriangleGeom& triangleGeom, const Int32Array& faceLabelsRef)
{
  const auto& triangleList = triangleGeom.getFacesRef().getDataStoreRef();
  const auto& faceLabels = faceLabelsRef.getDataStoreRef();
  const usize numRegions = 1 + *std::max_element(faceLabels.begin(), faceLabels.end());

  using EdgePairType = std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>;
  using UniqueEdgesType = std::set<EdgePairType>;
  using UniqueVertType = std::set<IGeometry::MeshIndexType>;

  constexpr IGeometry::MeshIndexType numVertsPerElem = 3;
  std::vector<int64> regionTriangleCount(numRegions, 0);
  std::vector<UniqueEdgesType> uniqueEdges(numRegions);
  std::vector<UniqueVertType> uniqueVerts(numRegions);

  for(IGeometry::MeshIndexType tIdx = 0; tIdx < triangleList.getNumberOfTuples(); ++tIdx)
  {
    const usize offset = tIdx * numVertsPerElem;

    for(IGeometry::MeshIndexType labelIdx = 0; labelIdx < 2; labelIdx++)
    {
      IGeometry::MeshIndexType v0 = 0;
      IGeometry::MeshIndexType v1 = 0;

      const auto regionIdx = faceLabels[tIdx * 2 + labelIdx];
      if(regionIdx < 0)
      {
        continue;
      }

      ++regionTriangleCount[regionIdx];
      // Compute the first 2 pairs of vertices: V0,V1 and V1,V2
      for(usize j = 0; j < numVertsPerElem - 1; j++)
      {
        auto t0 = triangleList[offset + j];
        auto tj = triangleList[offset + j + 1];
        v0 = std::min(t0, tj);
        v1 = std::max(t0, tj);

        EdgePairType edge = std::make_pair(v0, v1);
        uniqueVerts[regionIdx].insert(v0);
        uniqueVerts[regionIdx].insert(v1);
        uniqueEdges[regionIdx].insert(edge);
      }

      // compute the last pair which wraps from V2,V0
      {
        usize j = numVertsPerElem - 1;

        auto t0 = triangleList[offset];
        auto tj = triangleList[offset + j];

        v0 = std::min(t0, tj);
        v1 = std::max(t0, tj);

        EdgePairType edge = std::make_pair(v0, v1);
        uniqueVerts[regionIdx].insert(v0);
        uniqueVerts[regionIdx].insert(v1);
        uniqueEdges[regionIdx].insert(edge);
      }
    }
  } // End triangle Loop

  std::vector<int32> eulerCharacteristicValues(regionTriangleCount.size(), 0);
  for(usize i = 0; i < regionTriangleCount.size(); i++)
  {
    eulerCharacteristicValues[i] = static_cast<int32>(static_cast<int64>(uniqueVerts[i].size()) + regionTriangleCount[i] - static_cast<int64>(uniqueEdges[i].size()));
  }
  return eulerCharacteristicValues;
}

/** This section of code calculates the Euler Characteristic values for all regions
 * in a triangle geometry. It trades computation speed for memory efficiency by looping
 * over each region, and then all triangles. This is potentially very slow due to
 * the looping of every triangle for every region.
 */
#if 0
 // this is the very slow, but less memory intensive way to do this. This will not scale well!!
  // Loop over each Unique Feature's set of triangles
  for(MeshIndexType regionIdx = 0; regionIdx < numRegions; regionIdx++)
  {
    using EdgePairType = std::pair<MeshIndexType, MeshIndexType>;
    std::set<EdgePairType> uniqueEdges;
    std::set<MeshIndexType> uniqueVerts;
    MeshIndexType v0 = 0;
    MeshIndexType v1 = 0;
    int64 numTriangles = 0;
    // Loop over all triangles; Each Triangle has 3 Vertices
    for(MeshIndexType tIdx = 0; tIdx < triangleList.getNumberOfTuples(); ++tIdx)
    {
      constexpr MeshIndexType numVertsPerElem = 3;
      const usize offset = tIdx * numVertsPerElem;
      if(faceLabels[tIdx * 2] == regionIdx || faceLabels[tIdx * 2 + 1] == regionIdx)
      {
        numTriangles++;
        for(usize j = 0; j < numVertsPerElem; j++)
        {
          if(j == (numVertsPerElem - 1))
          {
            if(triangleList[offset + j] > triangleList[offset + 0])
            {
              v0 = triangleList[offset + 0];
              v1 = triangleList[offset + j];
            }
            else
            {
              v0 = triangleList[offset + j];
              v1 = triangleList[offset + 0];
            }
          }
          else
          {
            if(triangleList[offset + j] > triangleList[offset + j + 1])
            {
              v0 = triangleList[offset + j + 1];
              v1 = triangleList[offset + j];
            }
            else
            {
              v0 = triangleList[offset + j];
              v1 = triangleList[offset + j + 1];
            }
          }
          EdgePairType edge = std::make_pair(v0, v1);
          uniqueVerts.insert(v0);
          uniqueVerts.insert(v1);
          uniqueEdges.insert(edge);
        }
      }
    } // End triangle Loop
    int64 eulerCharacteristic = static_cast<int64>(uniqueVerts.size()) + numTriangles - static_cast<int64>(uniqueEdges.size());
    std::string message = fmt::format("Region: {} Euler Characteristic: {} = V:{} + F:{} - E:{}  ==> {}", regionIdx, eulerCharacteristic, uniqueVerts.size(), numTriangles, uniqueEdges.size(),
                                      eulerCharacteristics[regionIdx]);
    m_MessageHandler.sendInfoMessage(message);
  } // End Region Loop
#endif
} // namespace nx::core::GeometryHelpers::Connectivity

namespace nx::core::GeometryHelpers::Topology
{
BoundingBoxFaces FindElementPeriodicFaces(const BoundingBox3Df& boundingBox, const Float32AbstractDataStore& vertices, const std::set<IGeometry::MeshIndexType>& vertexSet)
{
  if(vertexSet.empty())
  {
    return {};
  }

  BoundingBoxFaces edgeFaces;

  const auto maxPoint = boundingBox.getMaxPoint();
  const auto minPoint = boundingBox.getMinPoint();

  constexpr float32 k_Epsilon = std::numeric_limits<float32>::epsilon();

  for(const auto& vert : vertexSet)
  {
    const float32 x = vertices[3 * vert + 0];
    const float32 y = vertices[3 * vert + 1];
    const float32 z = vertices[3 * vert + 2];

    if(std::abs(x - minPoint[0]) <= k_Epsilon)
    {
      edgeFaces.insert(BoundingBox3Df::faces_enum::left);
    }
    else if(std::abs(x - maxPoint[0]) <= k_Epsilon)
    {
      edgeFaces.insert(BoundingBox3Df::faces_enum::right);
    }

    if(std::abs(y - minPoint[1]) <= k_Epsilon)
    {
      edgeFaces.insert(BoundingBox3Df::faces_enum::top);
    }
    else if(std::abs(y - maxPoint[1]) <= k_Epsilon)
    {
      edgeFaces.insert(BoundingBox3Df::faces_enum::bottom);
    }

    if(std::abs(z - minPoint[2]) <= k_Epsilon)
    {
      edgeFaces.insert(BoundingBox3Df::faces_enum::front);
    }
    else if(std::abs(z - maxPoint[2]) <= k_Epsilon)
    {
      edgeFaces.insert(BoundingBox3Df::faces_enum::back);
    }
  }
  return edgeFaces;
}

bool AdjustCentroidsForPeriodicFaces(const BoundingBox3Df& boundingBox, const BoundingBoxFaces& faces, Float32AbstractDataStore& centroids, IGeometry::MeshIndexType featureId)
{
  bool isPeriodic = false;
  const auto minPoint = boundingBox.getMinPoint();
  const auto maxPoint = boundingBox.getMaxPoint();

  if(faces.contains(BoundingBox3Df::faces_enum::left) && faces.contains(BoundingBox3Df::faces_enum::right))
  {
    centroids[3 * featureId + 0] += std::abs(maxPoint[0] - minPoint[0]) / 2.0f;
    isPeriodic = true;
  }
  if(faces.contains(BoundingBox3Df::faces_enum::top) && faces.contains(BoundingBox3Df::faces_enum::bottom))
  {
    centroids[3 * featureId + 1] += std::abs(maxPoint[1] - minPoint[1]) / 2.0f;
    isPeriodic = true;
  }
  if(faces.contains(BoundingBox3Df::faces_enum::front) && faces.contains(BoundingBox3Df::faces_enum::back))
  {
    centroids[3 * featureId + 2] += std::abs(maxPoint[2] - minPoint[2]) / 2.0f;
    isPeriodic = true;
  }

  return isPeriodic;
}

bool AdjustCentroidsForPeriodicFaces(const ImageGeom& imageGeom, const UInt64AbstractDataStore& xRanges, const UInt64AbstractDataStore& yRanges, const UInt64AbstractDataStore& zRanges,
                                     Float32AbstractDataStore& centroids)
{
  const size_t xPoints = imageGeom.getNumXCells() - 1;
  const size_t yPoints = imageGeom.getNumYCells() - 1;
  const size_t zPoints = imageGeom.getNumZCells() - 1;

  // Centroids are stored in physical coordinates (origin + (index + 0.5) * spacing), so the periodic
  // wrap offset must also be a physical distance. Scale the (cell-count) span by the spacing on each
  // axis; the offset is half the physical distance between the first and last cell centers. Using the
  // raw cell count here would only be correct for unit spacing.
  const auto spacing = imageGeom.getSpacing();

  bool isAdjusted = false;

  const usize numFeatures = xRanges.size() / 2;
  for(usize featureId = 0; featureId < numFeatures; featureId++)
  {
    if(xRanges[featureId * 2 + 0] == 0 && xRanges[featureId * 2 + 1] == xPoints)
    {
      isAdjusted = true;
      centroids[featureId * 3 + 0] += (static_cast<float32>(xPoints) * spacing[0]) / 2.0f;
    }

    if(yRanges[featureId * 2 + 0] == 0 && yRanges[featureId * 2 + 1] == yPoints)
    {
      isAdjusted = true;
      centroids[featureId * 3 + 1] += (static_cast<float32>(yPoints) * spacing[1]) / 2.0f;
    }

    if(zRanges[featureId * 2 + 0] == 0 && zRanges[featureId * 2 + 1] == zPoints)
    {
      isAdjusted = true;
      centroids[featureId * 3 + 2] += (static_cast<float32>(zPoints) * spacing[2]) / 2.0f;
    }
  }

  return isAdjusted;
}
} // namespace nx::core::GeometryHelpers::Topology
