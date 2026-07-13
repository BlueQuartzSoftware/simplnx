#include "GeometryHelpers.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>

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
    m_MessageHandler(IFilter::Message::Type::Info, message);
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

namespace
{
// Computes the periodic (minimum-image) mean of a set of 1-D coordinates on a domain of length
// domainLength whose lower bound is domainMin, using the largest-empty-gap method. Every coordinate is
// reduced into [0, domainLength); the largest circular gap between consecutive sorted values locates the
// empty region of the wrapped feature; the coordinates on the near side of that gap are unwrapped
// (+domainLength) so the feature becomes contiguous, the mean is taken, and the result is mapped back into
// [domainMin, domainMin + domainLength). When no single gap dominates (the feature fills the domain, so the
// largest gap is not unique) the plain arithmetic mean of the original coordinates is returned as a
// fallback, because the circular mean of a domain-filling distribution is not meaningful. Unlike a constant
// half-domain offset, this depends on where the feature's mass actually sits, so it is correct for
// asymmetric wrapped features and never lands outside the domain.
float32 PeriodicMean1D(const std::vector<float32>& coords, float32 domainMin, float32 domainLength)
{
  const usize numCoords = coords.size();
  if(numCoords == 0)
  {
    return domainMin;
  }
  const float32 arithmeticMean = std::accumulate(coords.cbegin(), coords.cend(), 0.0f) / static_cast<float32>(numCoords);
  if(domainLength <= 0.0f)
  {
    return arithmeticMean;
  }

  // Reduce every coordinate into [0, domainLength). A coordinate on the far face maps to 0, which is
  // correct: under periodicity the two opposing faces are the same location.
  std::vector<float32> reduced(numCoords);
  for(usize i = 0; i < numCoords; i++)
  {
    float32 value = std::fmod(coords[i] - domainMin, domainLength);
    if(value < 0.0f)
    {
      value += domainLength;
    }
    reduced[i] = value;
  }
  std::sort(reduced.begin(), reduced.end());

  // Find the largest circular gap between consecutive sorted coordinates (the last gap wraps the ring).
  float32 maxGap = -1.0f;
  usize cutIndex = 0;
  for(usize i = 0; i < numCoords; i++)
  {
    const float32 next = (i + 1 < numCoords) ? reduced[i + 1] : reduced[0] + domainLength;
    const float32 gap = next - reduced[i];
    if(gap > maxGap)
    {
      maxGap = gap;
      cutIndex = i;
    }
  }

  // If more than one gap is (within tolerance) as large as the maximum, there is no dominant empty region:
  // the feature fills the domain and the wrapped mean is undefined, so fall back to the arithmetic mean.
  const float32 gapTolerance = domainLength * 1.0e-6f;
  usize maxGapCount = 0;
  for(usize i = 0; i < numCoords; i++)
  {
    const float32 next = (i + 1 < numCoords) ? reduced[i + 1] : reduced[0] + domainLength;
    if(std::abs((next - reduced[i]) - maxGap) <= gapTolerance)
    {
      maxGapCount++;
    }
  }
  if(maxGapCount > 1)
  {
    return arithmeticMean;
  }

  // Unwrap the coordinates on the near side of the largest gap by one domain length so the feature is
  // contiguous, average in double precision, then map the result back into the domain.
  float64 sum = 0.0;
  for(usize i = 0; i < numCoords; i++)
  {
    float64 value = static_cast<float64>(reduced[i]);
    if(i <= cutIndex)
    {
      value += static_cast<float64>(domainLength);
    }
    sum += value;
  }
  float64 mean = std::fmod(sum / static_cast<float64>(numCoords), static_cast<float64>(domainLength));
  if(mean < 0.0)
  {
    mean += static_cast<float64>(domainLength);
  }
  return domainMin + static_cast<float32>(mean);
}
} // namespace

bool AdjustCentroidsForPeriodicFaces(const BoundingBox3Df& boundingBox, const BoundingBoxFaces& faces, const Float32AbstractDataStore& vertices, const std::set<IGeometry::MeshIndexType>& vertexSet,
                                     Float32AbstractDataStore& centroids, IGeometry::MeshIndexType featureId)
{
  bool isPeriodic = false;
  const auto minPoint = boundingBox.getMinPoint();
  const auto maxPoint = boundingBox.getMaxPoint();

  // left/right => X (axis 0), top/bottom => Y (axis 1), front/back => Z (axis 2)
  const std::array<std::pair<BoundingBox3Df::faces_enum, BoundingBox3Df::faces_enum>, 3> axisFaces = {std::make_pair(BoundingBox3Df::faces_enum::left, BoundingBox3Df::faces_enum::right),
                                                                                                      std::make_pair(BoundingBox3Df::faces_enum::top, BoundingBox3Df::faces_enum::bottom),
                                                                                                      std::make_pair(BoundingBox3Df::faces_enum::front, BoundingBox3Df::faces_enum::back)};

  // For each axis on which the feature touches both opposing periodic faces, the naive arithmetic centroid
  // lands in the empty middle of the wrapped feature. Replace that component with the minimum-image mean
  // computed directly from the feature's vertex coordinates on that axis.
  for(usize axis = 0; axis < 3; axis++)
  {
    if(!faces.contains(axisFaces[axis].first) || !faces.contains(axisFaces[axis].second))
    {
      continue;
    }
    const float32 domainLength = std::abs(maxPoint[axis] - minPoint[axis]);
    std::vector<float32> axisCoords;
    axisCoords.reserve(vertexSet.size());
    for(const auto& vert : vertexSet)
    {
      axisCoords.push_back(vertices[3 * vert + axis]);
    }
    centroids[3 * featureId + axis] = PeriodicMean1D(axisCoords, minPoint[axis], domainLength);
    isPeriodic = true;
  }

  return isPeriodic;
}
} // namespace nx::core::GeometryHelpers::Topology
