#include "RegularGridSampleSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <vector>

using namespace nx::core;

namespace
{
// Represents a 2D edge segment in the XY plane produced by slicing a triangle at a Z-plane.
struct SliceEdge
{
  float32 x1, y1;  // First endpoint
  float32 x2, y2;  // Second endpoint
  usize faceIndex; // Index of the triangle face that produced this edge
};

// Represents a scanline-edge intersection used for sorting and filling.
struct ScanlineIntersection
{
  float32 x;       // X coordinate of the intersection
  usize faceIndex; // Face that was crossed
};

// -----------------------------------------------------------------------------
// Compute the intersection of a triangle with a horizontal Z-plane.
// Uses a half-open interval (z >= zPlane is "above") to avoid double-counting
// at shared vertices and edges.
// Returns true if a valid (non-degenerate) edge was produced.
// -----------------------------------------------------------------------------
bool sliceTriangleAtZ(const std::array<Point3Df, 3>& verts, float32 zPlane, float32& outX1, float32& outY1, float32& outX2, float32& outY2)
{
  // Classify each vertex: true = above or on the plane, false = below
  std::array<bool, 3> above = {verts[0][2] >= zPlane, verts[1][2] >= zPlane, verts[2][2] >= zPlane};

  // If all vertices are on the same side, no intersection
  if(above[0] == above[1] && above[1] == above[2])
  {
    return false;
  }

  // Find the two points where triangle edges cross the Z-plane
  float32 pts[2][2];
  int32 numPts = 0;

  for(int32 i = 0; i < 3 && numPts < 2; i++)
  {
    int32 j = (i + 1) % 3;
    if(above[i] != above[j])
    {
      float32 dz = verts[j][2] - verts[i][2];
      float32 t = (zPlane - verts[i][2]) / dz;
      pts[numPts][0] = verts[i][0] + t * (verts[j][0] - verts[i][0]);
      pts[numPts][1] = verts[i][1] + t * (verts[j][1] - verts[i][1]);
      numPts++;
    }
  }

  if(numPts != 2)
  {
    return false;
  }

  // Skip degenerate zero-length edges (vertex exactly on the plane)
  float32 dx = pts[1][0] - pts[0][0];
  float32 dy = pts[1][1] - pts[0][1];
  if(dx * dx + dy * dy < 1e-12f)
  {
    return false;
  }

  outX1 = pts[0][0];
  outY1 = pts[0][1];
  outX2 = pts[1][0];
  outY2 = pts[1][1];
  return true;
}

struct ZSliceFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const std::atomic_bool& m_ShouldCancel, const IFilter::MessageHandler& m_MessageHandler, const ImageGeom& imageGeom, const TriangleGeom& triangleGeom,
                  const DataPath& faceLabelsArrayPath, const DataPath& featureIdsDataPath)
  {
    // -------------------------------------------------------------------------
    // 1. Get references to input data
    // -------------------------------------------------------------------------
    SizeVec3 dims = imageGeom.getDimensions();
    FloatVec3 origin = imageGeom.getOrigin();
    FloatVec3 spacing = imageGeom.getSpacing();

    usize xDim = dims[0];
    usize yDim = dims[1];
    usize zDim = dims[2];
    usize cellsPerSlice = xDim * yDim;
    usize numTriangles = triangleGeom.getNumberOfFaces();
    const auto& verticesRef = triangleGeom.getVertices()->getDataStoreRef();
    const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();

    using DataArrayType = DataArray<T>;
    const auto& faceLabelsArray = dataStructure.getDataRefAs<DataArrayType>(faceLabelsArrayPath);
    usize numFaceLabelComps = faceLabelsArray.getNumberOfComponents();
    const auto& faceLabelsRef = faceLabelsArray.getDataStoreRef();

    auto& featureIds = dataStructure.getDataRefAs<DataArrayType>(featureIdsDataPath);
    auto& featureIdsRef = featureIds.getDataStoreRef();
    // -------------------------------------------------------------------------
    // 2. Precompute per-triangle Z-range for fast rejection
    // -------------------------------------------------------------------------
    m_MessageHandler({IFilter::Message::Type::Info, "Preprocessing triangle data..."});

    struct TriangleZRange
    {
      float32 zMin = 0.0f;
      float32 zMax = 0.0f;
    };

    std::vector<TriangleZRange> triZRanges(numTriangles);
    for(usize t = 0; t < numTriangles; t++)
    {
      usize v0Idx = facesRef[t * 3];
      usize v1Idx = facesRef[t * 3 + 1];
      usize v2Idx = facesRef[t * 3 + 2];

      float32 z0 = verticesRef[v0Idx * 3 + 2];
      float32 z1 = verticesRef[v1Idx * 3 + 2];
      float32 z2 = verticesRef[v2Idx * 3 + 2];

      triZRanges[t].zMin = std::min({z0, z1, z2});
      triZRanges[t].zMax = std::max({z0, z1, z2});
    }

    if(m_ShouldCancel)
    {
      return;
    }

    // -------------------------------------------------------------------------
    // 3. Process each Z-slice using scanline rasterization
    // -------------------------------------------------------------------------
    m_MessageHandler({IFilter::Message::Type::Info, "Sampling surface mesh using scanline rasterization..."});

    // Reusable buffers to avoid per-slice allocations
    std::vector<SliceEdge> edges;
    std::vector<ScanlineIntersection> intersections;

    for(usize z = 0; z < zDim; z++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      if(z % 10 == 0)
      {
        m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Processing Z-slice {}/{}", z, zDim)});
      }

      float32 zCoord = origin[2] + (static_cast<float32>(z) + 0.5f) * spacing[2];

      // ----- Find all triangles spanning this Z and compute 2D edges -----
      edges.clear();
      for(usize t = 0; t < numTriangles; t++)
      {
        const auto& zRange = triZRanges[t];
        if(zRange.zMax < zCoord || zRange.zMin > zCoord)
        {
          continue;
        }

        // Reconstruct vertices from the original geometry arrays
        usize v0Idx = facesRef[t * 3];
        usize v1Idx = facesRef[t * 3 + 1];
        usize v2Idx = facesRef[t * 3 + 2];
        std::array<Point3Df, 3> verts = {Point3Df{verticesRef[v0Idx * 3], verticesRef[v0Idx * 3 + 1], verticesRef[v0Idx * 3 + 2]},
                                         Point3Df{verticesRef[v1Idx * 3], verticesRef[v1Idx * 3 + 1], verticesRef[v1Idx * 3 + 2]},
                                         Point3Df{verticesRef[v2Idx * 3], verticesRef[v2Idx * 3 + 1], verticesRef[v2Idx * 3 + 2]}};

        float32 ex1, ey1, ex2, ey2;
        if(sliceTriangleAtZ(verts, zCoord, ex1, ey1, ex2, ey2))
        {
          edges.push_back({ex1, ey1, ex2, ey2, t});
        }
      }

      // ----- Scanline fill for each Y row -----
      for(usize y = 0; y < yDim; y++)
      {
        float32 yCoord = origin[1] + (static_cast<float32>(y) + 0.5f) * spacing[1];

        // Find X-intersections of this scanline with all 2D edges
        intersections.clear();
        for(const auto& edge : edges)
        {
          float32 eYMin, eYMax;
          if(edge.y1 < edge.y2)
          {
            eYMin = edge.y1;
            eYMax = edge.y2;
          }
          else
          {
            eYMin = edge.y2;
            eYMax = edge.y1;
          }

          // Half-open interval [yMin, yMax) to avoid double-counting at endpoints
          if(yCoord < eYMin || yCoord >= eYMax)
          {
            continue;
          }

          float32 dy = edge.y2 - edge.y1;
          if(std::abs(dy) < 1e-10f)
          {
            continue; // Skip horizontal edges
          }

          float32 t = (yCoord - edge.y1) / dy;
          float32 xIntersect = edge.x1 + t * (edge.x2 - edge.x1);
          intersections.push_back({xIntersect, edge.faceIndex});
        }

        // Sort intersections by X coordinate
        std::sort(intersections.begin(), intersections.end(), [](const ScanlineIntersection& a, const ScanlineIntersection& b) { return a.x < b.x; });

        // Walk left to right, toggling feature IDs at each crossing
        T currentFeature = 0;
        usize nextIsect = 0;
        usize rowOffset = z * cellsPerSlice + y * xDim;

        for(usize x = 0; x < xDim; x++)
        {
          float32 xCoord = origin[0] + (static_cast<float32>(x) + 0.5f) * spacing[0];

          // Process all crossings up to this voxel center
          while(nextIsect < intersections.size() && intersections[nextIsect].x <= xCoord)
          {
            usize faceIdx = intersections[nextIsect].faceIndex;
            T label0 = faceLabelsRef[faceIdx];
            T label1 = T{0};
            if(numFaceLabelComps == 2)
            {
              label0 = faceLabelsRef[faceIdx * 2];
              label1 = faceLabelsRef[faceIdx * 2 + 1];
            }

            // Toggle: if we are currently in one of the two bordering features,
            // switch to the other. This correctly handles entering, exiting,
            // and transitioning between adjacent features.
            if(currentFeature == label0)
            {
              currentFeature = label1;
            }
            else if(currentFeature == label1)
            {
              currentFeature = label0;
            }
            else
            {
              // Neither label matches current feature (first crossing from
              // outside or mesh inconsistency). Pick the positive label.
              currentFeature = 0;
              if(label0 > 0 && label1 <= 0)
              {
                currentFeature = label0;
              }
              else if(label1 > 0 && label0 <= 0)
              {
                currentFeature = label1;
              }
              else
              {
                // Both positive but neither matches — take the larger label
                // as a deterministic fallback.
                currentFeature = std::max(label0, label1);
              }
            }

            nextIsect++;
          }

          featureIdsRef[rowOffset + x] = currentFeature;
        }
      }
    }
  }
};

} // namespace

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           RegularGridSampleSurfaceMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::~RegularGridSampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMesh::operator()()
{
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryOutputPath);

  ExecuteDataFunctionNoBool(ZSliceFunctor{}, m_DataStructure.getDataAsUnsafe<IDataArray>(m_InputValues->SurfaceMeshFaceLabelsArrayPath)->getDataType(), m_DataStructure, m_ShouldCancel,
                            m_MessageHandler, imageGeom, triangleGeom, m_InputValues->SurfaceMeshFaceLabelsArrayPath, m_InputValues->FeatureIdsArrayPath);

  return {};
}
