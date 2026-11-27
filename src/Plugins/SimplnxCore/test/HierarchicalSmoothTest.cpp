#include "SimplnxCore/Filters/HierarchicalSmoothFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp" // nx::core::NodeType constants
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
// float64 variant of a 3-vector; nx::core::Vec3 is float32, so a distinct alias avoids ambiguity.
using F64Vec3Type = std::array<float64, 3>;

F64Vec3Type operator-(const F64Vec3Type& a, const F64Vec3Type& b)
{
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

float64 Dot(const F64Vec3Type& a, const F64Vec3Type& b)
{
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

F64Vec3Type Cross(const F64Vec3Type& a, const F64Vec3Type& b)
{
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

float64 Norm(const F64Vec3Type& a)
{
  return std::sqrt(Dot(a, a));
}

F64Vec3Type Normalized(const F64Vec3Type& a)
{
  const float64 n = Norm(a);
  return {a[0] / n, a[1] / n, a[2] / n};
}

struct Plane
{
  F64Vec3Type point;  // a point on the plane (the seed-pair midpoint)
  F64Vec3Type normal; // unit normal
};
struct Line
{
  F64Vec3Type point;     // a point on the line (the seed-triple circumcenter)
  F64Vec3Type direction; // unit direction
};

Plane BisectorPlane(const F64Vec3Type& p, const F64Vec3Type& q)
{
  return Plane{.point = {(p[0] + q[0]) / 2.0, (p[1] + q[1]) / 2.0, (p[2] + q[2]) / 2.0}, .normal = Normalized(q - p)};
}

float64 DistanceToPlane(const F64Vec3Type& x, const Plane& plane)
{
  return std::fabs(Dot(x - plane.point, plane.normal));
}

// Circumcenter of triangle (a,b,c) is equidistant to all three and lies in their plane; the locus
// of points equidistant to a,b,c is the line through it perpendicular to the triangle plane.
Line TripleLine(const F64Vec3Type& a, const F64Vec3Type& b, const F64Vec3Type& c)
{
  const F64Vec3Type ab = b - a;
  const F64Vec3Type ac = c - a;
  const F64Vec3Type abXac = Cross(ab, ac);
  const float64 denom = 2.0 * Dot(abXac, abXac);
  const float64 ab2 = Dot(ab, ab);
  const float64 ac2 = Dot(ac, ac);
  // Barycentric circumcenter offset from vertex a (weights are the opposite-edge squared lengths).
  const F64Vec3Type w1 = Cross(abXac, ab);
  const F64Vec3Type w2 = Cross(ac, abXac);
  const F64Vec3Type toCenter = {(w1[0] * ac2 + w2[0] * ab2) / denom, (w1[1] * ac2 + w2[1] * ab2) / denom, (w1[2] * ac2 + w2[2] * ab2) / denom};
  // Direction sign depends on seed ordering, but distanceToLine projects out the parallel component.
  return Line{.point = {a[0] + toCenter[0], a[1] + toCenter[1], a[2] + toCenter[2]}, .direction = Normalized(abXac)};
}

float64 DistanceToLine(const F64Vec3Type& x, const Line& line)
{
  const F64Vec3Type d = x - line.point;
  const float64 t = Dot(d, line.direction);
  const F64Vec3Type proj = {d[0] - t * line.direction[0], d[1] - t * line.direction[1], d[2] - t * line.direction[2]};
  return Norm(proj);
}

// Point equidistant to 4 seeds = intersection of 3 bisector planes. Solve A x = b by Cramer's rule.
F64Vec3Type QuadPoint(const F64Vec3Type& p, const F64Vec3Type& q, const F64Vec3Type& r, const F64Vec3Type& s)
{
  const std::array<F64Vec3Type, 3> n = {q - p, r - p, s - p};
  const std::array<F64Vec3Type, 3> mid = {BisectorPlane(p, q).point, BisectorPlane(p, r).point, BisectorPlane(p, s).point};
  const F64Vec3Type rhs = {Dot(n[0], mid[0]), Dot(n[1], mid[1]), Dot(n[2], mid[2])};
  const auto det3 = [](const F64Vec3Type& c0, const F64Vec3Type& c1, const F64Vec3Type& c2) { return Dot(c0, Cross(c1, c2)); };
  const F64Vec3Type col0 = {n[0][0], n[1][0], n[2][0]};
  const F64Vec3Type col1 = {n[0][1], n[1][1], n[2][1]};
  const F64Vec3Type col2 = {n[0][2], n[1][2], n[2][2]};
  const float64 det = det3(col0, col1, col2);
  return {det3(rhs, col1, col2) / det, det3(col0, rhs, col2) / det, det3(col0, col1, rhs) / det};
}

bool IsQuadNode(int8 type)
{
  // Quad junctions: 4 (interior) or 14 (on the sample surface). The filter holds these fixed.
  return type == node_type::k_QuadPoint || type == node_type::k_SurfaceQuadPoint;
}

// Distinct grain ids touching a vertex (exterior label 0 excluded), capped at 4: a Voronoi vertex
// meets at most four grains. A vertex exceeding four is never a 2- or 3-grain node, so dropping the
// overflow cannot affect the plane/line classification. This flat structure avoids the per-vertex
// heap allocation a std::set would incur for every mesh vertex (matters at out-of-core scale).
struct IncidentGrains
{
  std::array<int32, 4> ids{0, 0, 0, 0};
  uint8 count = 0;
  void add(int32 grain)
  {
    for(uint8 i = 0; i < count; i++)
    {
      if(ids[i] == grain)
      {
        return;
      }
    }
    if(count < static_cast<uint8>(ids.size()))
    {
      ids[count] = grain;
      count++;
    }
  }
};

// Test geometry: off-axis seeds make the Voronoi bisectors oblique to the voxel grid, so the meshed
// interface is genuinely stair-stepped (an axis-aligned cut would mesh perfectly flat and give the
// smoother nothing to do).
const SizeVec3Type k_GridDims = {40, 40, 40};
const std::vector<F64Vec3Type> k_BicrystalSeeds = {{12.3, 16.1, 20.7}, {28.4, 24.9, 19.2}};
// Tetrahedral corners about the center (20,20,20): the point equidistant to all four is interior,
// giving one quad point with four triple lines radiating to the box surface.
const std::vector<F64Vec3Type> k_QuadSeeds = {{12.0, 12.0, 12.0}, {28.0, 28.0, 12.0}, {28.0, 12.0, 28.0}, {12.0, 28.0, 28.0}};

constexpr int32 k_MaxIterations = 53;     // HierarchicalSmooth default; ample for these meshes
constexpr float64 k_ErrorThreshold = 2.0; // reject nodes displaced > ~2x the characteristic edge length

// Paths produced by buildVoronoiMesh, consumed by the filter call and the oracle.
struct MeshPaths
{
  DataPath triangle_geom;
  DataPath node_type;   // Vertex Data / Node Type   (Int8)
  DataPath face_labels; // Face Data / Face Labels    (Int32 x2)
  DataPath vertices;    // SharedVertexList           (Float32 x3)
};

// Build an ImageGeom whose FeatureIds are the nearest-seed Voronoi assignment, then run
// QuickSurfaceMesh to obtain a stair-stepped TriangleGeom carrying Node Type + Face Labels.
// Seeds are grain ids 1..N (FeatureIds never 0, so QuickSurfaceMesh uses 0 for the exterior).
MeshPaths BuildVoronoiMesh(DataStructure& dataStructure, const SizeVec3Type& dims, const std::vector<F64Vec3Type>& seeds)
{
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Grid");
  imageGeomPtr->setDimensions(dims);
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());

  const std::vector<usize> tupleShape = {dims[2], dims[1], dims[0]}; // slowest..fastest = z,y,x
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", tupleShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  // FeatureIds lives in a DataArray (disk-backed in an out-of-core build), written element-by-element
  // below — never materialized into an in-core voxel-sized std::vector.
  auto* featureIdsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "FeatureIds", tupleShape, {1}, cellDataPtr->getId());

  const int32 seedCount = static_cast<int32>(seeds.size());
  for(usize z = 0; z < dims[2]; z++)
  {
    for(usize y = 0; y < dims[1]; y++)
    {
      for(usize x = 0; x < dims[0]; x++)
      {
        const F64Vec3Type center = {static_cast<float64>(x) + 0.5, static_cast<float64>(y) + 0.5, static_cast<float64>(z) + 0.5};
        int32 best = 0;
        float64 bestDistSq = std::numeric_limits<float64>::max();
        for(int32 s = 0; s < seedCount; s++)
        {
          const F64Vec3Type d = center - seeds[s];
          const float64 distSq = Dot(d, d);
          if(distSq < bestDistSq)
          {
            bestDistSq = distSq;
            best = s + 1; // grain id = seed index + 1
          }
        }
        const usize idx = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
        (*featureIdsPtr)[idx] = best;
      }
    }
  }

  const DataPath gridPath = imageGeomPtr->getDataPaths().at(0);
  const DataPath featureIdsPath = cellDataPtr->getDataPaths().at(0).createChildPath("FeatureIds");
  const DataPath triangleGeomPath({"SurfaceMesh"});

  QuickSurfaceMeshFilter qsm;
  Arguments args;
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("Node Type"));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("Face Labels"));

  auto preflight = qsm.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  auto execute = qsm.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  return MeshPaths{.triangle_geom = triangleGeomPath,
                   .node_type = triangleGeomPath.createChildPath("Vertex Data").createChildPath("Node Type"),
                   .face_labels = triangleGeomPath.createChildPath("Face Data").createChildPath("Face Labels"),
                   .vertices = triangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName)};
}

void RunHierarchicalSmooth(DataStructure& dataStructure, const MeshPaths& paths)
{
  HierarchicalSmoothFilter filter;
  Arguments args;
  args.insertOrAssign(HierarchicalSmoothFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(paths.triangle_geom));
  args.insertOrAssign(HierarchicalSmoothFilter::k_SurfaceMeshNodeTypeArrayPath_Key, std::make_any<DataPath>(paths.node_type));
  args.insertOrAssign(HierarchicalSmoothFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(paths.face_labels));
  args.insertOrAssign(HierarchicalSmoothFilter::k_MaxIterations_Key, std::make_any<int32>(k_MaxIterations));
  args.insertOrAssign(HierarchicalSmoothFilter::k_ErrorThreshold_Key, std::make_any<float64>(k_ErrorThreshold));

  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
}

// Distinct grain ids per vertex from the face labels (exterior label 0 excluded). One contiguous
// allocation sized to the vertex count; arrays are read element-wise (out-of-core safe).
std::vector<IncidentGrains> BuildIncidentGrains(const TriangleGeom& triangleGeom, const Int32ArrayType& faceLabels)
{
  const usize numVertices = triangleGeom.getNumberOfVertices();
  const usize numFaces = triangleGeom.getNumberOfFaces();
  const auto& faces = triangleGeom.getFacesRef();
  std::vector<IncidentGrains> incident(numVertices);
  for(usize f = 0; f < numFaces; f++)
  {
    for(int32 side = 0; side < 2; side++)
    {
      const int32 grain = faceLabels[(f * 2) + side];
      if(grain == 0)
      {
        continue;
      }
      for(usize v = 0; v < 3; v++)
      {
        incident[faces[(f * 3) + v]].add(grain);
      }
    }
  }
  return incident;
}

F64Vec3Type VertexCoord(const Float32ArrayType& vertices, usize v)
{
  return {static_cast<float64>(vertices[(v * 3) + 0]), static_cast<float64>(vertices[(v * 3) + 1]), static_cast<float64>(vertices[(v * 3) + 2])};
}
} // namespace

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: oracle math", "[SimplnxCore][HierarchicalSmoothFilter]")
{
  // Bisector plane of (0,0,0) and (2,0,0) is x=1; a point at x=1 has distance 0, at x=4 has distance 3.
  const Plane plane = BisectorPlane({0, 0, 0}, {2, 0, 0});
  REQUIRE(DistanceToPlane({1.0, 5.0, 9.0}, plane) == Approx(0.0).margin(1e-9));
  REQUIRE(DistanceToPlane({4.0, 0.0, 0.0}, plane) == Approx(3.0).margin(1e-9));

  // Triple line of three seeds in z=0 is the vertical line through their circumcenter.
  const Line line = TripleLine({0, 0, 0}, {2, 0, 0}, {0, 2, 0});
  REQUIRE(DistanceToLine({1.0, 1.0, 17.0}, line) == Approx(0.0).margin(1e-9));

  // Quad point equidistant to the 4 corners is (1,1,1).
  const F64Vec3Type quad = QuadPoint({0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2});
  REQUIRE(Norm(quad - F64Vec3Type{1.0, 1.0, 1.0}) == Approx(0.0).margin(1e-9));
}

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: Quad junction", "[SimplnxCore][HierarchicalSmoothFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  MeshPaths paths = BuildVoronoiMesh(dataStructure, k_GridDims, k_QuadSeeds);

  RunHierarchicalSmooth(dataStructure, paths);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(paths.triangle_geom));
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(paths.triangle_geom);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(paths.face_labels));
  const auto& faceLabels = dataStructure.getDataRefAs<Int32ArrayType>(paths.face_labels);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8ArrayType>(paths.node_type));
  const auto& nodeType = dataStructure.getDataRefAs<Int8ArrayType>(paths.node_type);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(paths.vertices));
  const auto& smoothed = dataStructure.getDataRefAs<Float32ArrayType>(paths.vertices);
  const std::vector<IncidentGrains> incident = BuildIncidentGrains(triangleGeom, faceLabels);

  // Tolerances are fixed literals calibrated from one-time measured maxima on this mesh
  // (maxPlane=0.53, maxLine=0.98, maxQuad=1.17 voxels), each with a ~1.5x cross-platform margin.
  constexpr float64 k_TolPlane = 0.8;
  constexpr float64 k_TolLine = 1.5;
  constexpr float64 k_TolQuad = 1.8;
  usize interiorNodes = 0;
  usize tripleNodes = 0;
  usize quadJunctionNodes = 0;
  const usize numVertices = triangleGeom.getNumberOfVertices();
  for(usize v = 0; v < numVertices; v++)
  {
    const F64Vec3Type x = VertexCoord(smoothed, v);
    if(nodeType[v] == node_type::k_Default && incident[v].count == 2)
    {
      REQUIRE(DistanceToPlane(x, BisectorPlane(k_QuadSeeds[incident[v].ids[0] - 1], k_QuadSeeds[incident[v].ids[1] - 1])) < k_TolPlane);
      interiorNodes++;
    }
    else if(nodeType[v] == node_type::k_TriplePoint && incident[v].count == 3)
    {
      REQUIRE(DistanceToLine(x, TripleLine(k_QuadSeeds[incident[v].ids[0] - 1], k_QuadSeeds[incident[v].ids[1] - 1], k_QuadSeeds[incident[v].ids[2] - 1])) < k_TolLine);
      tripleNodes++;
    }
    else if(nodeType[v] == node_type::k_QuadPoint && incident[v].count == 4)
    {
      // Interior quad points are held fixed by the filter and must sit at the analytic point where all four cells meet.
      const F64Vec3Type q = QuadPoint(k_QuadSeeds[incident[v].ids[0] - 1], k_QuadSeeds[incident[v].ids[1] - 1], k_QuadSeeds[incident[v].ids[2] - 1], k_QuadSeeds[incident[v].ids[3] - 1]);
      REQUIRE(Norm(x - q) < k_TolQuad);
      quadJunctionNodes++;
    }
  }

  // The arrangement must actually exercise the interior-boundary, triple-line, and quad-point paths.
  REQUIRE(interiorNodes > 0);
  REQUIRE(tripleNodes > 0);
  REQUIRE(quadJunctionNodes > 0);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: Bicrystal planar boundary", "[SimplnxCore][HierarchicalSmoothFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  MeshPaths paths = BuildVoronoiMesh(dataStructure, k_GridDims, k_BicrystalSeeds);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(paths.triangle_geom));
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(paths.triangle_geom);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(paths.face_labels));
  const auto& faceLabels = dataStructure.getDataRefAs<Int32ArrayType>(paths.face_labels);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8ArrayType>(paths.node_type));
  const auto& nodeType = dataStructure.getDataRefAs<Int8ArrayType>(paths.node_type);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(paths.vertices));
  auto& vertices = dataStructure.getDataRefAs<Float32ArrayType>(paths.vertices); // smoothed in place below
  const std::vector<IncidentGrains> incident = BuildIncidentGrains(triangleGeom, faceLabels);
  const usize numVertices = triangleGeom.getNumberOfVertices();

  // Accumulate the pre-smoothing (stair-stepped) error directly from the live array — no full-array
  // snapshot, so this stays out-of-core friendly. Topology (node types, incident grains) is fixed by
  // the mesh and unchanged by smoothing, so the same classification is reused after the smooth.
  bool sawQuad = false;
  float64 sumBefore = 0.0;
  for(usize v = 0; v < numVertices; v++)
  {
    if(IsQuadNode(nodeType[v]))
    {
      sawQuad = true;
    }
    if(nodeType[v] == node_type::k_Default && incident[v].count == 2)
    {
      const Plane plane = BisectorPlane(k_BicrystalSeeds[incident[v].ids[0] - 1], k_BicrystalSeeds[incident[v].ids[1] - 1]);
      sumBefore += DistanceToPlane(VertexCoord(vertices, v), plane);
    }
  }

  RunHierarchicalSmooth(dataStructure, paths);

  // Measured (macOS): interior plane-distance maxAfter ~= 0.16 voxel, mean reduction ratio ~= 0.15.
  constexpr float64 k_TolPlane = 0.25;     // 1.5x margin over the measured max
  constexpr float64 k_MaxErrorRatio = 0.4; // require >= 60% of the stair-step error removed
  float64 sumAfter = 0.0;
  usize n = 0;
  for(usize v = 0; v < numVertices; v++)
  {
    if(nodeType[v] != node_type::k_Default || incident[v].count != 2)
    {
      continue;
    }
    const Plane plane = BisectorPlane(k_BicrystalSeeds[incident[v].ids[0] - 1], k_BicrystalSeeds[incident[v].ids[1] - 1]);
    const float64 dAfter = DistanceToPlane(VertexCoord(vertices, v), plane);
    REQUIRE(dAfter < k_TolPlane); // every interior boundary node lands near the analytic bisector plane
    sumAfter += dAfter;
    n++;
  }
  REQUIRE(n > 0);
  // A bicrystal has no quad junctions, so every free-boundary segment uses the cyclic / no-quad path.
  // (Type-13 surface triple points appear where the interface meets the box surface; those are not
  // quad junctions and do not affect this check.)
  REQUIRE_FALSE(sawQuad);
  REQUIRE(sumAfter <= k_MaxErrorRatio * sumBefore);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: Displacement rejection", "[SimplnxCore][HierarchicalSmoothFilter]")
{
  unit_test::LoadPlugins();

  // buildVoronoiMesh is deterministic, so an identical control mesh has the same vertex indices.
  // Choose the interior node farthest from the analytic plane (it will move the most when smoothed).
  DataStructure controlDs;
  MeshPaths controlPaths = BuildVoronoiMesh(controlDs, k_GridDims, k_BicrystalSeeds);
  REQUIRE_NOTHROW(controlDs.getDataRefAs<TriangleGeom>(controlPaths.triangle_geom));
  const auto& controlGeom = controlDs.getDataRefAs<TriangleGeom>(controlPaths.triangle_geom);
  REQUIRE_NOTHROW(controlDs.getDataRefAs<Int32ArrayType>(controlPaths.face_labels));
  const auto& controlFaceLabels = controlDs.getDataRefAs<Int32ArrayType>(controlPaths.face_labels);
  REQUIRE_NOTHROW(controlDs.getDataRefAs<Int8ArrayType>(controlPaths.node_type));
  const auto& controlNodeType = controlDs.getDataRefAs<Int8ArrayType>(controlPaths.node_type);
  REQUIRE_NOTHROW(controlDs.getDataRefAs<Float32ArrayType>(controlPaths.vertices));
  auto& controlVerts = controlDs.getDataRefAs<Float32ArrayType>(controlPaths.vertices);
  const std::vector<IncidentGrains> incident = BuildIncidentGrains(controlGeom, controlFaceLabels);

  const usize numVertices = controlGeom.getNumberOfVertices();
  usize target = numVertices; // sentinel: not found
  float64 worstDist = -1.0;
  for(usize v = 0; v < numVertices; v++)
  {
    if(controlNodeType[v] != node_type::k_Default || incident[v].count != 2)
    {
      continue;
    }
    const Plane plane = BisectorPlane(k_BicrystalSeeds[incident[v].ids[0] - 1], k_BicrystalSeeds[incident[v].ids[1] - 1]);
    const float64 d = DistanceToPlane(VertexCoord(controlVerts, v), plane);
    if(d > worstDist)
    {
      worstDist = d;
      target = v;
    }
  }
  REQUIRE(target < numVertices);

  // Control: with no injection the filter genuinely moves this node, so "stays put after injection"
  // below demonstrates rejection rather than the node simply being immobile.
  const F64Vec3Type controlBefore = VertexCoord(controlVerts, target);
  RunHierarchicalSmooth(controlDs, controlPaths);
  REQUIRE(Norm(VertexCoord(controlVerts, target) - controlBefore) > 0.05);

  // Rejection: shove the same node far off the interface (>> 2x edge length). Its post-smoothing
  // displacement exceeds the error threshold, so the filter resets it to this injected position.
  DataStructure dataStructure;
  MeshPaths paths = BuildVoronoiMesh(dataStructure, k_GridDims, k_BicrystalSeeds);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(paths.vertices));
  auto& verts = dataStructure.getDataRefAs<Float32ArrayType>(paths.vertices);
  const std::array<float32, 3> injected = {verts[target * 3] + 25.0f, verts[target * 3 + 1] + 25.0f, verts[target * 3 + 2] + 25.0f};
  verts[target * 3] = injected[0];
  verts[target * 3 + 1] = injected[1];
  verts[target * 3 + 2] = injected[2];

  RunHierarchicalSmooth(dataStructure, paths);

  REQUIRE(verts[target * 3] == Approx(injected[0]));
  REQUIRE(verts[target * 3 + 1] == Approx(injected[1]));
  REQUIRE(verts[target * 3 + 2] == Approx(injected[2]));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: Idempotency", "[SimplnxCore][HierarchicalSmoothFilter]")
{
  unit_test::LoadPlugins();

  // Two independent meshes (buildVoronoiMesh is deterministic, so they are identical): one smoothed
  // once, one smoothed twice. Comparing them streams element-wise from both live arrays, avoiding a
  // full-array snapshot that would defeat an out-of-core build.
  DataStructure onceDs;
  MeshPaths oncePaths = BuildVoronoiMesh(onceDs, k_GridDims, k_BicrystalSeeds);
  RunHierarchicalSmooth(onceDs, oncePaths);

  DataStructure twiceDs;
  MeshPaths twicePaths = BuildVoronoiMesh(twiceDs, k_GridDims, k_BicrystalSeeds);
  RunHierarchicalSmooth(twiceDs, twicePaths);
  RunHierarchicalSmooth(twiceDs, twicePaths);

  REQUIRE_NOTHROW(onceDs.getDataRefAs<TriangleGeom>(oncePaths.triangle_geom));
  const auto& onceGeom = onceDs.getDataRefAs<TriangleGeom>(oncePaths.triangle_geom);
  REQUIRE_NOTHROW(onceDs.getDataRefAs<Float32ArrayType>(oncePaths.vertices));
  const auto& onceVerts = onceDs.getDataRefAs<Float32ArrayType>(oncePaths.vertices);
  REQUIRE_NOTHROW(twiceDs.getDataRefAs<Float32ArrayType>(twicePaths.vertices));
  const auto& twiceVerts = twiceDs.getDataRefAs<Float32ArrayType>(twicePaths.vertices);
  REQUIRE(onceVerts.getNumberOfTuples() == twiceVerts.getNumberOfTuples());

  float64 maxDisp = 0.0;
  const usize numVertices = onceGeom.getNumberOfVertices();
  for(usize v = 0; v < numVertices; v++)
  {
    maxDisp = std::max(maxDisp, Norm(VertexCoord(twiceVerts, v) - VertexCoord(onceVerts, v)));
  }

  // Approximate-stability check: HierarchicalSmooth is NOT strictly idempotent in one pass (the
  // reference MATLAB runs Smooth three times), so a second pass still nudges nodes slightly.
  // Measured (macOS): maxDisp ~= 0.31 voxel; tol = 0.5 includes a 1.5x margin.
  constexpr float64 k_TolIdempotent = 0.5;
  REQUIRE(maxDisp < k_TolIdempotent);

  unit_test::CheckArraysInheritTupleDims(twiceDs);
}
