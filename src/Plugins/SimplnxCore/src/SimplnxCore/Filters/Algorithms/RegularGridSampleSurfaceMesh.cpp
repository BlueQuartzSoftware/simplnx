#include "RegularGridSampleSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

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

// Precomputed per-triangle Z-range for fast rejection during Z-slice processing.
struct TriangleZRange
{
  float32 zMin = 0.0f;
  float32 zMax = 0.0f;
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

// -----------------------------------------------------------------------------
// Worker class that rasterizes a single Z-slice into a thread-local buffer,
// then copies results back to the output DataArray via the parent algorithm's
// mutex-protected sendThreadSafeSliceUpdate() method.
// -----------------------------------------------------------------------------
template <typename FaceLabelsT, typename OutputT>
class ZSliceWorker
{
public:
  ZSliceWorker(RegularGridSampleSurfaceMesh* algorithm, usize zSlice, usize xDim, usize yDim, usize numTriangles, usize numFaceLabelComps, const FloatVec3& origin, const FloatVec3& spacing,
               const AbstractDataStore<IGeometry::MeshIndexType>& facesRef, const AbstractDataStore<float32>& verticesRef, const AbstractDataStore<FaceLabelsT>& faceLabelsRef,
               const std::vector<TriangleZRange>& triZRanges, std::atomic_bool& overflowHit)
  : m_Algorithm(algorithm)
  , m_ZSlice(zSlice)
  , m_XDim(xDim)
  , m_YDim(yDim)
  , m_NumTriangles(numTriangles)
  , m_NumFaceLabelComps(numFaceLabelComps)
  , m_Origin(origin)
  , m_Spacing(spacing)
  , m_FacesRef(facesRef)
  , m_VerticesRef(verticesRef)
  , m_FaceLabelsRef(faceLabelsRef)
  , m_TriZRanges(triZRanges)
  , m_OverflowHit(overflowHit)
  {
  }

  void operator()() const
  {
    usize cellsPerSlice = m_XDim * m_YDim;
    std::vector<OutputT> sliceBuffer(cellsPerSlice, OutputT{0});

    const float32 zCoord = m_Origin[2] + (static_cast<float32>(m_ZSlice) + 0.5f) * m_Spacing[2];

    // ----- Find all triangles spanning this Z and compute 2D edges -----
    std::vector<SliceEdge> edges;
    for(usize t = 0; t < m_NumTriangles; t++)
    {
      const auto& zRange = m_TriZRanges[t];
      if(zRange.zMax < zCoord || zRange.zMin > zCoord)
      {
        continue;
      }

      const usize v0Idx = m_FacesRef[t * 3];
      const usize v1Idx = m_FacesRef[t * 3 + 1];
      const usize v2Idx = m_FacesRef[t * 3 + 2];
      std::array<Point3Df, 3> verts = {Point3Df{m_VerticesRef[v0Idx * 3], m_VerticesRef[v0Idx * 3 + 1], m_VerticesRef[v0Idx * 3 + 2]},
                                       Point3Df{m_VerticesRef[v1Idx * 3], m_VerticesRef[v1Idx * 3 + 1], m_VerticesRef[v1Idx * 3 + 2]},
                                       Point3Df{m_VerticesRef[v2Idx * 3], m_VerticesRef[v2Idx * 3 + 1], m_VerticesRef[v2Idx * 3 + 2]}};

      float32 ex1, ey1, ex2, ey2;
      if(sliceTriangleAtZ(verts, zCoord, ex1, ey1, ex2, ey2))
      {
        edges.push_back({ex1, ey1, ex2, ey2, t});
      }
    }

    // ----- Scanline fill for each Y row -----
    std::vector<ScanlineIntersection> intersections;
    for(usize y = 0; y < m_YDim; y++)
    {
      const float32 yCoord = m_Origin[1] + (static_cast<float32>(y) + 0.5f) * m_Spacing[1];

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

        const float32 dy = edge.y2 - edge.y1;
        if(std::abs(dy) < 1e-10f)
        {
          continue; // Skip horizontal edges
        }

        const float32 t = (yCoord - edge.y1) / dy;
        const float32 xIntersect = edge.x1 + t * (edge.x2 - edge.x1);
        intersections.push_back({xIntersect, edge.faceIndex});
      }

      // Sort intersections by X coordinate
      std::sort(intersections.begin(), intersections.end(), [](const ScanlineIntersection& a, const ScanlineIntersection& b) { return a.x < b.x; });

      // Walk left to right, toggling feature IDs at each crossing
      OutputT currentFeature = 0;
      usize nextIsect = 0;
      const usize rowOffset = y * m_XDim;

      for(usize x = 0; x < m_XDim; x++)
      {
        const float32 xCoord = m_Origin[0] + (static_cast<float32>(x) + 0.5f) * m_Spacing[0];

        if(m_OverflowHit)
        {
          return;
        }

        // Process all crossings up to this voxel center
        while(nextIsect < intersections.size() && intersections[nextIsect].x <= xCoord)
        {
          usize faceIdx = intersections[nextIsect].faceIndex;
          OutputT label0, label1;
          if constexpr(std::is_same_v<FaceLabelsT, int8>)
          {
            if(m_NumFaceLabelComps == 2)
            {
              label0 = static_cast<uint8>(m_FaceLabelsRef[faceIdx * 2]);
              label1 = static_cast<uint8>(m_FaceLabelsRef[faceIdx * 2 + 1]);

              if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
              {
                if(std::numeric_limits<OutputT>::max() < label0 || std::numeric_limits<OutputT>::max() < label1)
                {
                  m_OverflowHit = true;
                }
              }
            }
            else
            {
              label0 = static_cast<uint8>(m_FaceLabelsRef[faceIdx]);
              label1 = OutputT{0};

              if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
              {
                if(std::numeric_limits<OutputT>::max() < label0)
                {
                  m_OverflowHit = true;
                }
              }
            }
          }
          else
          {
            if(m_NumFaceLabelComps == 2)
            {
              label0 = m_FaceLabelsRef[faceIdx * 2];
              label1 = m_FaceLabelsRef[faceIdx * 2 + 1];

              if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
              {
                if(std::numeric_limits<OutputT>::max() < label0 || std::numeric_limits<OutputT>::max() < label1)
                {
                  m_OverflowHit = true;
                }
              }
            }
            else
            {
              label0 = m_FaceLabelsRef[faceIdx];
              label1 = OutputT{0};

              if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
              {
                if(std::numeric_limits<OutputT>::max() < label0)
                {
                  m_OverflowHit = true;
                }
              }
            }
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
              // Both positive but neither matches -- take the larger label
              // as a deterministic fallback.
              currentFeature = std::max(label0, label1);
            }
          }

          nextIsect++;
        }

        sliceBuffer[rowOffset + x] = currentFeature;
      }
    }

    // ----- Copy results back to the output DataArray under mutex -----
    m_Algorithm->sendThreadSafeSliceUpdate(m_ZSlice, sliceBuffer);
  }

private:
  RegularGridSampleSurfaceMesh* m_Algorithm;
  usize m_ZSlice;
  usize m_XDim;
  usize m_YDim;
  usize m_NumTriangles;
  usize m_NumFaceLabelComps;
  const FloatVec3& m_Origin;
  const FloatVec3& m_Spacing;
  const AbstractDataStore<IGeometry::MeshIndexType>& m_FacesRef;
  const AbstractDataStore<float32>& m_VerticesRef;
  const AbstractDataStore<FaceLabelsT>& m_FaceLabelsRef;
  const std::vector<TriangleZRange>& m_TriZRanges;
  std::atomic_bool& m_OverflowHit;
};

// -----------------------------------------------------------------------------
// Functor dispatched by ExecuteDataFunctionIntType to handle the templated T.
// Sets up shared read-only data and dispatches Z-slice workers in parallel.
// -----------------------------------------------------------------------------
template <typename FaceLabelsT>
struct ZSliceFunctor
{
  template <typename OutputT>
  Result<> operator()(const AbstractDataStore<FaceLabelsT>& faceLabelsRef, RegularGridSampleSurfaceMesh* algorithm, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                      const ImageGeom& imageGeom, const TriangleGeom& triangleGeom)
  {
    // -------------------------------------------------------------------------
    // 1. Get references to input data
    // -------------------------------------------------------------------------
    const SizeVec3 dims = imageGeom.getDimensions();
    const FloatVec3 origin = imageGeom.getOrigin();
    const FloatVec3 spacing = imageGeom.getSpacing();

    usize xDim = dims[0];
    usize yDim = dims[1];
    usize zDim = dims[2];
    usize numTriangles = triangleGeom.getNumberOfFaces();
    const auto& verticesRef = triangleGeom.getVertices()->getDataStoreRef();
    const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();

    usize numFaceLabelComps = faceLabelsRef.getNumberOfComponents();

    // -------------------------------------------------------------------------
    // 2. Precompute per-triangle Z-range for fast rejection
    // -------------------------------------------------------------------------
    messageHandler.sendInfoMessage("Preprocessing triangle data...");

    std::vector<TriangleZRange> triZRanges(numTriangles);
    for(usize t = 0; t < numTriangles; t++)
    {
      const usize v0Idx = facesRef[t * 3];
      const usize v1Idx = facesRef[t * 3 + 1];
      const usize v2Idx = facesRef[t * 3 + 2];

      float32 z0 = verticesRef[v0Idx * 3 + 2];
      float32 z1 = verticesRef[v1Idx * 3 + 2];
      float32 z2 = verticesRef[v2Idx * 3 + 2];

      triZRanges[t].zMin = std::min({z0, z1, z2});
      triZRanges[t].zMax = std::max({z0, z1, z2});
    }

    if(shouldCancel)
    {
      return {};
    }

    // -------------------------------------------------------------------------
    // 3. Dispatch Z-slices in parallel using ParallelTaskAlgorithm
    // -------------------------------------------------------------------------
    messageHandler.sendInfoMessage(fmt::format("Sampling surface mesh using scanline rasterization ({} Z-slices)...", zDim));

    std::atomic_bool overflowHit(false);

    ParallelTaskAlgorithm taskAlgorithm;
    for(usize z = 0; z < zDim; z++)
    {
      if(shouldCancel)
      {
        break;
      }

      taskAlgorithm.execute(
          ZSliceWorker<FaceLabelsT, OutputT>(algorithm, z, xDim, yDim, numTriangles, numFaceLabelComps, origin, spacing, facesRef, verticesRef, faceLabelsRef, triZRanges, overflowHit));
    }
    taskAlgorithm.wait();

    if(overflowHit)
    {
      return MakeErrorResult(-158640, fmt::format("Overflow occurred when downcasting a Face Label value of type {} to a feature Id value of type {}",
                                                  DataTypeToHumanString(GetDataType<FaceLabelsT>()), DataTypeToHumanString(GetDataType<OutputT>())));
    }

    return {};
  }
};

struct OutputTypeDispatcher
{
  template <typename FaceLabelsT, typename... ArgsT>
  auto operator()(const IDataArray& iFaceLabels, DataType outputType, ArgsT&&... args)
  {
    const auto& faceLabels = dynamic_cast<const DataArray<FaceLabelsT>&>(iFaceLabels).getDataStoreRef();
    return ExecuteDataFunctionIntType(ZSliceFunctor<FaceLabelsT>{}, outputType, faceLabels, std::forward<ArgsT>(args)...);
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
  const auto& iFaceLabels = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  const DataType outputType = m_DataStructure.getDataAsUnsafe<IDataArray>(m_InputValues->FeatureIdsArrayPath)->getDataType();

  SizeVec3 dims = imageGeom.getDimensions();
  m_CellsPerSlice = dims[0] * dims[1];

  return ExecuteDataFunctionIntType(OutputTypeDispatcher{}, iFaceLabels.getDataType(), iFaceLabels, outputType, this, m_ShouldCancel, m_MessageHandler, imageGeom, triangleGeom);
}
