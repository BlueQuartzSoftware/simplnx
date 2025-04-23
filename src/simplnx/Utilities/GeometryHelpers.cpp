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
                << "     Extent " << 0 << "-" << dims[i] - 1 << " (dimension: " << dims[i] << ") Voxels\n";
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

      auto regionIdx = faceLabels[tIdx * 2 + labelIdx];
      if(regionIdx < 0)
      {
        continue;
      }

      ++regionTriangleCount[regionIdx];

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
  //    std::string message = fmt::format("Region: {} Euler Characteristic: {} = V:{} + F:{} - E:{}", regionIdx, eulerCharacteristic, uniqueVerts.size(), numTriangles, uniqueEdges.size());
  //    m_MessageHandler(IFilter::Message::Type::Info, message);
  return eulerCharacteristicValues;
}

#if 0

/** This section of code calculates the Euler Characteristic values for all regions
 * in a triangle geometry. It trades computation speed for memory efficiency.
 */


 // this is the very slow, but lest memory intensive way to do this. This will not scale well!!
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
