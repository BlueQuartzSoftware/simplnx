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
// -----------------------------------------------------------------------------
char determineIntersectCoord(const std::array<float32, 2>& p1, const std::array<float32, 2>& q1, const std::array<float32, 2>& p2, const std::array<float32, 2>& q2, float32& coordX)
{
  // assumes p1q1 is the hatch vector and p2q2 is the CAD edge
  // also assumes the p1q1 is in x direction only so can just check y coords for potential intersection
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

// A structure to store line segments resulting from the fill.
// Each filled line is represented by start and end points in 3D.
struct LineSegment
{
  Eigen::Vector3f start;
  Eigen::Vector3f end;
};

// Intersect a line defined as y' = const_line with a segment defined by two points in rotated space.
// The segment endpoints are (x1', y1') and (x2', y2'). We want to find intersection with y' = lineY'.
bool lineSegmentHorizontalIntersect(const Eigen::Vector3f& p1, const Eigen::Vector3f& p2, float lineYprime, Eigen::Vector3f& intersection)
{
  float y1 = p1.y();
  float y2 = p2.y();

  // Check if the horizontal line at lineYprime intersects the segment.
  if((y1 <= lineYprime && y2 >= lineYprime) || (y2 <= lineYprime && y1 >= lineYprime))
  {
    // The segment crosses y' = lineYprime
    float dy = y2 - y1;
    if(std::abs(dy) < 1e-9f)
    {
      // Horizontal line segment: intersection can be direct
      // If the lineYprime equals y1=y2, then the whole segment is on the line.
      // This is a rare case; we can handle by returning one endpoint as intersection.
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
      // Linear interpolation for x and y
      float x = p1.x() + t * (p2.x() - p1.x());
      // z unchanged, assuming flat polygon
      intersection = Eigen::Vector3f(x, lineYprime, p1.z());
      return true;
    }
  }
  return false;
}

// Main function that generates fill lines.
std::vector<LineSegment> fillPolygonWithParallelLines(const std::vector<float>& vertices, const std::vector<usize>& edges, float lineSpacing, float angleRadians)
{
  float rotAngle = -angleRadians;
  Eigen::Matrix3f k_RotationMatrix;
  k_RotationMatrix << std::cos(rotAngle), -std::sin(rotAngle), 0.0f, std::sin(rotAngle), std::cos(rotAngle), 0.0f, 0.0f, 0.0f, 1.0f;

  Eigen::Matrix3f k_InvRotationMatrix;
  k_InvRotationMatrix << std::cos(angleRadians), -std::sin(angleRadians), 0.0f, std::sin(angleRadians), std::cos(angleRadians), 0.0f, 0.0f, 0.0f, 1.0f;

  usize numVerts = vertices.size() / 3;
  usize numEdges = edges.size() / 2;

  // Rotate all vertices by -angleRadians to align fill lines horizontally in the rotated frame
  std::vector<Eigen::Vector3f> rotatedVertices(numVerts);
  for(size_t i = 0; i < numVerts; ++i)
  {
    Eigen::Vector3f pt(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
    rotatedVertices[i] = k_RotationMatrix * pt; // rotatePoint(vertices[i], rotAngle);
  }

  // Build rotated edges
  // Actually, edges remain the same indices, but we consider rotatedVertices now.

  // Compute bounding box in rotated frame
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

  // Determine the set of parallel lines: they will be horizontal lines in the rotated frame,
  // starting from minY to maxY, spaced by lineSpacing.
  // We can start from a line at floor(minY/lineSpacing)*lineSpacing to be neat:
  float startLineY = std::floor(minY / lineSpacing) * lineSpacing;
  if(startLineY < minY)
  {
    startLineY += lineSpacing;
  }

  std::vector<LineSegment> filledSegments;

  // For each line, we find intersection points with polygon edges.
  // The polygon edges are (rotatedVertices[ei.first], rotatedVertices[ei.second]).
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

    // Sort intersections by x to find pairs that form inside segments
    std::sort(intersections.begin(), intersections.end(), [](const Eigen::Vector3f& a, const Eigen::Vector3f& b) { return a.x() < b.x(); });

    // Polygon fill lines: between intersections, we pick pairs (every two intersection points form a segment inside the polygon)
    // This simple approach assumes a well-formed polygon where intersection points on a scanline come in pairs and the starting
    // point of the scan line is ALWAYS OUTSIDE of the polygon.
    //
    // ******* Complex polygons simply break in very subtle an unique ways. Don't try to fix the code. Fix the mesh instead.
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

      // Rotate back the line segment to the original frame
      // Rotation back is by angleRadians
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

// ----------------------------------------------------------------------------
void extractRegion(INodeGeometry0D::SharedVertexList& vertices, INodeGeometry1D::SharedEdgeList& edges, AbstractDataStore<int32>& regionIds, AbstractDataStore<int32>& sliceIds,
                   int32_t regionIdToExtract, int32_t sliceIdToExtract, std::vector<float>& outVertices, std::vector<size_t>& outEdges)
{
  outVertices.clear();
  outVertices.reserve(750);
  outEdges.clear();
  outEdges.reserve(500);

  // Mapping from old vertex index to new vertex index
  std::unordered_map<size_t, size_t> vertexMap;
  vertexMap.reserve(750);

  const size_t numEdges = edges.getNumberOfTuples();

  // Iterate over all edges
  for(size_t i = 0; i < numEdges; ++i)
  {
    if(regionIds[i] == regionIdToExtract && sliceIds[i] == sliceIdToExtract)
    {
      // This edge belongs to the target region
      size_t oldV0 = edges[2 * i];
      size_t oldV1 = edges[2 * i + 1];

      // Check if we have already encountered oldV0
      size_t newV0;
      auto itV0 = vertexMap.find(oldV0);
      if(itV0 == vertexMap.end())
      {
        // Add new vertex
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

      // Check oldV1 similarly
      size_t newV1;
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

      // Now add the edge to outEdges
      outEdges.push_back(newV0);
      outEdges.push_back(newV1);
    }
  }
}

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

// -----------------------------------------------------------------------------
CreateAMScanPaths::CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateAMScanPaths::~CreateAMScanPaths() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateAMScanPaths::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateAMScanPaths::operator()()
{
  // Get References to all the INPUT Data Objects
  auto& CADLayers = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->CADSliceDataContainerName);
  INodeGeometry1D::SharedEdgeList& outlineEdges = CADLayers.getEdgesRef();
  INodeGeometry0D::SharedVertexList& outlineVertices = CADLayers.getVerticesRef();
  auto& cadSliceIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CADSliceIdsArrayPath)->getDataStoreRef();
  auto& cadRegionIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CADRegionIdsArrayPath)->getDataStoreRef();
  usize numCADLayerEdges = CADLayers.getNumberOfEdges();

  // Get References to all the OUTPUT Data Objects
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

  using LineSegmentsType = std::vector<LineSegment>;

  // Loop on every Region
  // Parallelize over the regions?
  for(int32 regionId = 0; regionId < numCADRegions; regionId++)
  {
    float angle = 0; // Start at zero degree rotation

    std::vector<LineSegmentsType> regionHatches(numCADLayers);

    // Loop on every slice within that region
    for(int32 sliceId = 0; sliceId < numCADLayers; sliceId++)
    {
      // Extract the Edges for just this region and slice
      // This should be output to its own function
      std::vector<float> outVertices;
      std::vector<size_t> outEdges;

      extractRegion(outlineVertices, outlineEdges, cadRegionIds, cadSliceIds, regionId, sliceId, outVertices, outEdges);

      regionHatches[sliceId] = ::fillPolygonWithParallelLines(outVertices, outEdges, m_InputValues->HatchSpacing, angle);

      // printRegionSliceFiles(regionId, sliceId, regionHatches[sliceId]);
      angle = angle + m_InputValues->SliceHatchRotationAngle; // Rotate each layer by 67 degrees.
    }

    // This can come out into a function that could be thread safe in order
    // to parallelize over each Region
    // Now that we have our Hatches for a given Region, copy those into an ever expanding Edge Geometry.
    usize currentNumVerts = hatchVertsDataStore.getNumberOfTuples();
    usize currentNumEdges = hatchesDataStore.getNumberOfTuples();
    usize vertStartOffset = currentNumVerts;
    usize edgeStartOffset = currentNumEdges;

    for(const auto& lineSegmentVector : regionHatches)
    {
      currentNumVerts = currentNumVerts + lineSegmentVector.size() * 2;
      currentNumEdges = currentNumEdges + lineSegmentVector.size();
    }
    // Resize the Edge Geometry
    hatchesEdgeGeom.resizeVertexList(currentNumVerts);
    hatchesEdgeGeom.resizeEdgeList(currentNumEdges);
    // Resize the Vertex and Edge Attribute Matrix to the new values
    hatchesEdgeGeom.getVertexAttributeMatrix()->resizeTuples({currentNumVerts});
    hatchesEdgeGeom.getEdgeAttributeMatrix()->resizeTuples({currentNumEdges});

    int32 currentSliceId = 0;
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
