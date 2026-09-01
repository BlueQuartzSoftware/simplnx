#include "CreateAMScanPaths.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

#include <fmt/format.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Finds the X intersection of a hatch line and CAD edge.
 * @param p1 Specifies the first hatch endpoint.
 * @param q1 Specifies the second hatch endpoint.
 * @param p2 Specifies the first CAD-edge endpoint.
 * @param q2 Specifies the second CAD-edge endpoint.
 * @param coordX Receives the intersection X coordinate.
 * @return 'c' or 'd' for a CAD endpoint, 'i' for an interior crossing, or 'n' when absent.
 * @pre The hatch segment is horizontal in the XY plane.
 */
char determineIntersectCoord(const std::array<float32, 2>& p1, const std::array<float32, 2>& q1, const std::array<float32, 2>& p2, const std::array<float32, 2>& q2, float32& coordX)
{
  float32 x1 = p1[0];
  float32 x2 = q1[0];
  float32 x3 = p2[0];
  float32 x4 = q2[0];
  float32 y1 = p1[1];
  //  float32 y2 = q1[1];
  float32 y3 = p2[1];
  float32 y4 = q2[1];

  if(y3 > y1 && y4 > y1)
  {
    return 'n';
  }
  if(y3 < y1 && y4 < y1)
  {
    return 'n';
  }
  if(y3 == y1 && y4 == y1)
  {
    return 'n';
  }
  if(y3 == y1)
  {
    coordX = x3;
    if(x3 >= x1 && x3 <= x2)
    {
      return 'c';
    }
    return 'n';
  }
  if(y4 == y1)
  {
    coordX = x4;
    if(x4 >= x1 && x4 <= x2)
    {
      return 'd';
    }
    return 'n';
  }
  float32 frac = (y1 - y3) / (y4 - y3);
  coordX = x3 + (frac * (x4 - x3));
  if(coordX >= x1 && coordX <= x2)
  {
    return 'i';
  }
  return 'n';
}

/**
 * @struct LineSegment
 * @brief Stores one generated hatch segment.
 */
struct LineSegment
{
  Eigen::Vector3f start;
  Eigen::Vector3f end;
};

/**
 * @brief Intersects a rotated CAD edge with one horizontal hatch line.
 * @param p1 Specifies the first rotated edge endpoint.
 * @param p2 Specifies the second rotated edge endpoint.
 * @param lineYprime Specifies the rotated hatch-line Y coordinate.
 * @param intersection Receives one intersection point.
 * @return True when the edge intersects the hatch line.
 */
bool lineSegmentHorizontalIntersect(const Eigen::Vector3f& p1, const Eigen::Vector3f& p2, float lineYprime, Eigen::Vector3f& intersection)
{
  float y1 = p1.y();
  float y2 = p2.y();

  if((y1 <= lineYprime && y2 >= lineYprime) || (y2 <= lineYprime && y1 >= lineYprime))
  {
    // The segment crosses y' = lineYprime
    float dy = y2 - y1;
    if(std::abs(dy) < 1e-9f)
    {
      // A coincident edge returns its first endpoint.
      intersection = p1;
      return true;
    }
    else if(std::abs(lineYprime - p1.y()) < 1e-9f)
    {
      intersection = p1;
      return true;
    }
    else if(std::abs(lineYprime - p2.y()) < 1e-9f)
    {
      intersection = p2;
      return true;
    }
    else
    {
      float t = (lineYprime - y1) / dy;
      float x = p1.x() + t * (p2.x() - p1.x());
      intersection = Eigen::Vector3f(x, lineYprime, p1.z());
      return true;
    }
  }
  return false;
}

/**
 * @brief Generates rotated parallel hatch segments for one CAD polygon.
 * @param vertices Provides XYZ vertex values.
 * @param edges Provides paired vertex indexes.
 * @param lineSpacing Specifies spacing between rotated hatch lines.
 * @param angleRadians Specifies hatch rotation in radians.
 * @return Hatch segments in the original coordinate system.
 * @pre Each hatch line has one filled interval and lineSpacing is positive.
 *
 * The loop connects adjacent sorted crossings. Concave polygons can fill
 * exterior gaps. Malformed or complex CAD meshes can produce incorrect hatches.
 */
std::vector<LineSegment> fillPolygonWithParallelLines(const std::vector<float>& vertices, const std::vector<usize>& edges, float lineSpacing, float angleRadians)
{
  float rotAngle = -angleRadians;
  Eigen::Matrix3f k_RotationMatrix;
  k_RotationMatrix << std::cos(rotAngle), -std::sin(rotAngle), 0.0f, std::sin(rotAngle), std::cos(rotAngle), 0.0f, 0.0f, 0.0f, 1.0f;

  Eigen::Matrix3f k_InvRotationMatrix;
  k_InvRotationMatrix << std::cos(angleRadians), -std::sin(angleRadians), 0.0f, std::sin(angleRadians), std::cos(angleRadians), 0.0f, 0.0f, 0.0f, 1.0f;

  usize numVerts = vertices.size() / 3;
  usize numEdges = edges.size() / 2;

  // Rotation makes every hatch line horizontal in the temporary frame.
  std::vector<Eigen::Vector3f> rotatedVertices(numVerts);
  for(size_t i = 0; i < numVerts; ++i)
  {
    Eigen::Vector3f pt(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
    rotatedVertices[i] = k_RotationMatrix * pt; // rotatePoint(vertices[i], rotAngle);
  }

  float minX = std::numeric_limits<float>::infinity();
  float maxX = -std::numeric_limits<float>::infinity();
  float minY = std::numeric_limits<float>::infinity();
  float maxY = -std::numeric_limits<float>::infinity();
  for(auto& v : rotatedVertices)
  {
    minX = std::min(v.x(), minX);
    maxX = std::max(v.x(), maxX);

    minY = std::min(v.y(), minY);
    maxY = std::max(v.y(), maxY);
  }

  // Start on the first rotated grid line inside the polygon bounds.
  float startLineY = std::floor(minY / lineSpacing) * lineSpacing;
  if(startLineY < minY)
  {
    startLineY += lineSpacing;
  }

  std::vector<LineSegment> filledSegments;

  for(float lineY = startLineY; lineY <= maxY; lineY += lineSpacing)
  {
    std::vector<Eigen::Vector3f> intersections;

    for(size_t edgeIdx = 0; edgeIdx < numEdges; edgeIdx++)
    {
      Eigen::Vector3f p1 = rotatedVertices[edges[edgeIdx * 2]];
      Eigen::Vector3f p2 = rotatedVertices[edges[edgeIdx * 2 + 1]];
      Eigen::Vector3f inter;
      if(lineSegmentHorizontalIntersect(p1, p2, lineY, inter))
      {
        intersections.push_back({inter});
      }
    }

    std::sort(intersections.begin(), intersections.end(), [](const Eigen::Vector3f& a, const Eigen::Vector3f& b) { return a.x() < b.x(); });

    for(size_t i = 0; i + 1 < intersections.size(); i++)
    {
      Eigen::Vector3f startPt = intersections[i];
      Eigen::Vector3f endPt = intersections[i + 1];

      if(startPt == endPt)
      {
        if(intersections.size() % 2 == 0)
        {
          i++;
        }
        continue;
      }

      Eigen::Vector3f origStart = k_InvRotationMatrix * startPt;
      Eigen::Vector3f origEnd = k_InvRotationMatrix * endPt;

      LineSegment seg;
      seg.start = origStart;
      seg.end = origEnd;
      filledSegments.push_back(seg);
      i++;
    }
  }

  return filledSegments;
}

/**
 * @brief Extracts one region-slice submesh with contiguous vertex indexes.
 * @param vertices Provides source XYZ vertex values.
 * @param edges Provides source paired vertex indexes.
 * @param regionSliceEdgeIndices Provides ascending edge indexes for one region and slice.
 * @param outVertices Receives remapped XYZ vertex values.
 * @param outEdges Receives paired remapped vertex indexes.
 *
 * Pre-bucketed indexes avoid scanning the complete CAD edge list for each
 * region-slice pair.
 */
void extractRegion(const INodeGeometry0D::SharedVertexList& vertices, const INodeGeometry1D::SharedEdgeList& edges, nonstd::span<const usize> regionSliceEdgeIndices, std::vector<float>& outVertices,
                   std::vector<usize>& outEdges)
{
  outVertices.clear();
  outVertices.reserve(750);
  outEdges.clear();
  outEdges.reserve(500);

  std::unordered_map<usize, usize> vertexMap;
  vertexMap.reserve(750);

  for(usize i : regionSliceEdgeIndices)
  {
    usize oldV0 = edges[2 * i];
    usize oldV1 = edges[2 * i + 1];

    usize newV0;
    auto itV0 = vertexMap.find(oldV0);
    if(itV0 == vertexMap.end())
    {
      newV0 = outVertices.size() / 3;
      outVertices.push_back(vertices[oldV0 * 3]);
      outVertices.push_back(vertices[oldV0 * 3 + 1]);
      outVertices.push_back(vertices[oldV0 * 3 + 2]);
      vertexMap[oldV0] = newV0;
    }
    else
    {
      newV0 = itV0->second;
    }

    usize newV1;
    auto itV1 = vertexMap.find(oldV1);
    if(itV1 == vertexMap.end())
    {
      newV1 = outVertices.size() / 3;
      outVertices.push_back(vertices[oldV1 * 3]);
      outVertices.push_back(vertices[oldV1 * 3 + 1]);
      outVertices.push_back(vertices[oldV1 * 3 + 2]);

      vertexMap[oldV1] = newV1;
    }
    else
    {
      newV1 = itV1->second;
    }

    outEdges.push_back(newV0);
    outEdges.push_back(newV1);
  }
}

/**
 * @brief Writes one region-slice hatch debug file pair under /tmp.
 * @param regionId Identifies the CAD region.
 * @param sliceId Identifies the CAD slice.
 * @param lineSegments Provides hatch segments to write.
 */
void printRegionSliceFiles(int32 regionId, int32 sliceId, const std::vector<LineSegment>& lineSegments)
{
  if(lineSegments.empty())
  {
    fmt::print("NO LINES: Region {}  Slice {}\n", regionId, sliceId);
    return;
  }
  std::string outputVertsFilePath = fmt::format("/tmp/{}_{}_verts.csv", regionId, sliceId);
  std::ofstream vertsFile(outputVertsFilePath, std::ios_base::binary);
  vertsFile << "X,Y,Z\n";

  std::string outputEdgeFilePath = fmt::format("/tmp/{}_{}_edges.csv", regionId, sliceId);
  std::ofstream edgesFile(outputEdgeFilePath, std::ios_base::binary);
  edgesFile << "V0,V1\n";
  usize vertIndex = 0;

  for(const auto& segment : lineSegments)
  {
    vertsFile << fmt::format("{}\n", fmt::join(segment.start, ","));
    vertsFile << fmt::format("{}\n", fmt::join(segment.end, ","));

    edgesFile << vertIndex++ << "," << vertIndex++ << "\n";
  }
}

} // namespace

CreateAMScanPaths::CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

CreateAMScanPaths::~CreateAMScanPaths() noexcept = default;

const std::atomic_bool& CreateAMScanPaths::getCancel()
{
  return m_ShouldCancel;
}

Result<> CreateAMScanPaths::operator()()
{
  auto& CADLayers = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->CADSliceDataContainerName);
  INodeGeometry1D::SharedEdgeList& outlineEdges = CADLayers.getEdgesRef();
  INodeGeometry0D::SharedVertexList& outlineVertices = CADLayers.getVerticesRef();
  auto& cadSliceIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CADSliceIdsArrayPath)->getDataStoreRef();
  auto& cadRegionIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CADRegionIdsArrayPath)->getDataStoreRef();
  usize numCADLayerEdges = CADLayers.getNumberOfEdges();

  auto& hatchesEdgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->HatchDataContainerName);
  hatchesEdgeGeom.resizeEdgeList(0ULL);
  hatchesEdgeGeom.resizeVertexList(0ULL);

  AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>& hatchVertsDataStore = hatchesEdgeGeom.getVertices()->getDataStoreRef();
  AbstractDataStore<INodeGeometry1D::SharedEdgeList::value_type>& hatchesDataStore = hatchesEdgeGeom.getEdges()->getDataStoreRef();

  const DataPath hatchAttributeMatrixPath = m_InputValues->HatchDataContainerName.createChildPath(m_InputValues->HatchAttributeMatrixName);
  auto& hatchSliceIdsDataStore = m_DataStructure.getDataAs<Int32Array>(hatchAttributeMatrixPath.createChildPath(m_InputValues->CADSliceIdsArrayPath.getTargetName()))->getDataStoreRef();
  auto& hatchRegionIdsDataStore = m_DataStructure.getDataAs<Int32Array>(hatchAttributeMatrixPath.createChildPath(m_InputValues->RegionIdsArrayName))->getDataStoreRef();

  int32 numCADLayers = 0;
  int32 numCADRegions = 0;
  for(usize i = 0; i < numCADLayerEdges; i++)
  {
    int32 layer = cadSliceIds[i];
    int32 region = cadRegionIds[i];
    if(numCADLayers < layer)
    {
      numCADLayers = layer;
    }
    if(numCADRegions < region)
    {
      numCADRegions = region;
    }
  }
  numCADLayers += 1;
  numCADRegions += 1;

  // Bucket each CAD edge once by region and slice. Ascending insertion preserves
  // its edge order and prevents each region-slice pair from rescanning all edges.
  std::vector<std::vector<std::vector<usize>>> edgeBucketsByRegionThenSlice(numCADRegions, std::vector<std::vector<usize>>(numCADLayers));
  for(usize i = 0; i < numCADLayerEdges; i++)
  {
    edgeBucketsByRegionThenSlice[cadRegionIds[i]][cadSliceIds[i]].push_back(i);
  }

  using LineSegmentsType = std::vector<LineSegment>;

  // Hatch generation currently does not check cancellation or use parallel regions.
  // StripeWidth is currently not read by hatch generation.
  for(int32 regionId = 0; regionId < numCADRegions; regionId++)
  {
    float angle = 0;

    std::vector<LineSegmentsType> regionHatches(numCADLayers);
    const std::vector<std::vector<usize>>& sliceBucketsForRegion = edgeBucketsByRegionThenSlice[regionId];

    for(int32 sliceId = 0; sliceId < numCADLayers; sliceId++)
    {
      std::vector<float> outVertices;
      std::vector<usize> outEdges;

      extractRegion(outlineVertices, outlineEdges, sliceBucketsForRegion[sliceId], outVertices, outEdges);

      regionHatches[sliceId] = ::fillPolygonWithParallelLines(outVertices, outEdges, m_InputValues->HatchSpacing, angle);

      // printRegionSliceFiles(regionId, sliceId, regionHatches[sliceId]);
      angle = angle + m_InputValues->SliceHatchRotationAngle;
    }

    usize currentNumVerts = hatchVertsDataStore.getNumberOfTuples();
    usize currentNumEdges = hatchesDataStore.getNumberOfTuples();
    usize vertStartOffset = currentNumVerts;
    usize edgeStartOffset = currentNumEdges;

    for(const auto& lineSegmentVector : regionHatches)
    {
      currentNumVerts = currentNumVerts + lineSegmentVector.size() * 2;
      currentNumEdges = currentNumEdges + lineSegmentVector.size();
    }
    hatchesEdgeGeom.resizeVertexList(currentNumVerts);
    hatchesEdgeGeom.resizeEdgeList(currentNumEdges);
    hatchesEdgeGeom.getVertexAttributeMatrix()->resizeTuples({currentNumVerts});
    hatchesEdgeGeom.getEdgeAttributeMatrix()->resizeTuples({currentNumEdges});

    int32 currentSliceId = 0;
    // Current output writes use per-value store access and can be slow for
    // disk-backed hatch arrays.
    for(const auto& lineSegmentVector : regionHatches)
    {
      for(const auto& lineSegment : lineSegmentVector)
      {
        hatchRegionIdsDataStore[edgeStartOffset] = regionId;
        hatchSliceIdsDataStore[edgeStartOffset] = currentSliceId;

        hatchVertsDataStore[vertStartOffset * 3] = lineSegment.start[0];
        hatchVertsDataStore[vertStartOffset * 3 + 1] = lineSegment.start[1];
        hatchVertsDataStore[vertStartOffset * 3 + 2] = lineSegment.start[2];
        hatchesDataStore[edgeStartOffset * 2] = vertStartOffset;
        vertStartOffset++;

        hatchVertsDataStore[vertStartOffset * 3] = lineSegment.end[0];
        hatchVertsDataStore[vertStartOffset * 3 + 1] = lineSegment.end[1];
        hatchVertsDataStore[vertStartOffset * 3 + 2] = lineSegment.end[2];
        hatchesDataStore[edgeStartOffset * 2 + 1] = vertStartOffset;
        vertStartOffset++;
        edgeStartOffset++;
      }
      currentSliceId++;
    }
  }
  return {};
}
