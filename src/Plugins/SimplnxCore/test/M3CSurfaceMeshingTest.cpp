#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
// Independent structural validation of an M3C output mesh (does NOT rely on the exemplar oracle):
//  - no degenerate triangles and all vertex indices in range,
//  - FaceLabels ordered (smaller feature id in component 0, per the output convention),
//  - every emitted vertex is a real node (compaction keeps only NodeType > 0).
void CheckMeshIntegrity(DataStructure& dataStructure, const DataPath& triGeomPath, const DataPath& faceLabelsPath, const DataPath& nodeTypesPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(triGeomPath));
  const TriangleGeom& tri = dataStructure.getDataRefAs<TriangleGeom>(triGeomPath);
  const auto& triStore = tri.getFaces()->getDataStoreRef();
  const usize numTriangles = tri.getNumberOfFaces();
  const usize numVertices = tri.getNumberOfVertices();
  REQUIRE(numTriangles > 0);
  REQUIRE(numVertices > 0);
  for(usize i = 0; i < numTriangles; i++)
  {
    const auto a = triStore[i * 3 + 0];
    const auto b = triStore[i * 3 + 1];
    const auto c = triStore[i * 3 + 2];
    REQUIRE(a < numVertices);
    REQUIRE(b < numVertices);
    REQUIRE(c < numVertices);
    REQUIRE(a != b);
    REQUIRE(b != c);
    REQUIRE(a != c);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(faceLabelsPath));
  const auto& faceLabelStore = dataStructure.getDataRefAs<Int32Array>(faceLabelsPath).getDataStoreRef();
  for(usize i = 0; i < numTriangles; i++)
  {
    REQUIRE(faceLabelStore[i * 2 + 0] <= faceLabelStore[i * 2 + 1]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(nodeTypesPath));
  const auto& nodeTypeStore = dataStructure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();
  for(usize i = 0; i < numVertices; i++)
  {
    REQUIRE(nodeTypeStore[i] > 0);
  }

  // A face separates exactly two DISTINCT regions. (-1,-1) would mean a face between two cells that
  // are both outside the volume, which is the signature of the ghost shell being triangulated
  // against itself; (a,a) would mean a face inside a single feature.
  for(usize i = 0; i < numTriangles; i++)
  {
    const int32 labelA = faceLabelStore[i * 2 + 0];
    const int32 labelB = faceLabelStore[i * 2 + 1];
    INFO("triangle " << i << " has FaceLabels (" << labelA << ", " << labelB << ")");
    REQUIRE(labelA != labelB);
    REQUIRE_FALSE((labelA == -1 && labelB == -1));
  }
}

/**
 * @brief Asserts that the mesh lies within the source volume, and that its exterior surface lies ON
 * the volume boundary.
 *
 * These are first-principles properties of a surface mesh of a gridded volume, derived from the
 * input geometry alone - no golden file is involved, so they validate the mesher rather than merely
 * pinning whatever it last produced.
 *
 * The bound is inclusive: a correct mesh touches the boundary planes exactly.
 */
void CheckMeshWithinVolume(const DataStructure& dataStructure, const DataPath& triGeomPath, const DataPath& faceLabelsPath, const DataPath& imageGeomPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(imageGeomPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 origin = imageGeom.getOrigin();
  const FloatVec3 spacing = imageGeom.getSpacing();

  const std::array<float32, 3> lo = {origin[0], origin[1], origin[2]};
  const std::array<float32, 3> hi = {origin[0] + static_cast<float32>(dims[0]) * spacing[0], origin[1] + static_cast<float32>(dims[1]) * spacing[1],
                                     origin[2] + static_cast<float32>(dims[2]) * spacing[2]};
  // Tolerance has to clear float32 representation error at the coordinate magnitudes in play while
  // staying far below the defect being guarded against, which was half a cell. The spacing term sets
  // the physical scale; the epsilon term keeps the check from becoming flaky for volumes with a large
  // origin, where the gap between representable floats can exceed a small absolute tolerance.
  const float32 maxSpacing = std::max({spacing[0], spacing[1], spacing[2]});
  const float32 maxMagnitude = std::max({std::abs(lo[0]), std::abs(lo[1]), std::abs(lo[2]), std::abs(hi[0]), std::abs(hi[1]), std::abs(hi[2])});
  const float32 tol = std::max(1.0e-4f * maxSpacing, 8.0f * std::numeric_limits<float32>::epsilon() * maxMagnitude);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(triGeomPath));
  const auto& tri = dataStructure.getDataRefAs<TriangleGeom>(triGeomPath);
  const auto& vertStore = tri.getVertices()->getDataStoreRef();
  const usize numVertices = tri.getNumberOfVertices();

  for(usize i = 0; i < numVertices; i++)
  {
    for(usize ax = 0; ax < 3; ax++)
    {
      const float32 c = vertStore[i * 3 + ax];
      INFO("vertex " << i << " axis " << ax << " = " << c << ", volume spans [" << lo[ax] << ", " << hi[ax] << "]");
      REQUIRE(c >= lo[ax] - tol);
      REQUIRE(c <= hi[ax] + tol);
    }
  }

  // Every VERTEX of an exterior triangle (one side outside the volume) must lie on the boundary
  // surface of the volume, i.e. on at least one of the six bounding planes.
  //
  // Deliberately per-vertex rather than per-triangle: along a volume edge or corner the ghost-to-
  // feature interface wraps around, so a single exterior triangle legitimately spans two different
  // bounding planes and is not coplanar with either. Requiring the whole triangle to sit on one
  // plane fails on real data for that reason. Spurious geometry generated inside the ghost shell
  // still violates the per-vertex form, which is what this guards.
  const auto& triStore = tri.getFaces()->getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(faceLabelsPath));
  const auto& faceLabelStore = dataStructure.getDataRefAs<Int32Array>(faceLabelsPath).getDataStoreRef();

  for(usize i = 0; i < tri.getNumberOfFaces(); i++)
  {
    if(faceLabelStore[i * 2 + 0] != -1 && faceLabelStore[i * 2 + 1] != -1)
    {
      continue;
    }
    for(usize corner = 0; corner < 3; corner++)
    {
      const usize vertIndex = triStore[i * 3 + corner];
      bool onBoundary = false;
      for(usize ax = 0; ax < 3 && !onBoundary; ax++)
      {
        const float32 c = vertStore[vertIndex * 3 + ax];
        onBoundary = (std::abs(c - lo[ax]) <= tol) || (std::abs(c - hi[ax]) <= tol);
      }
      INFO("exterior triangle " << i << " corner " << corner << " (vertex " << vertIndex << ") is not on the volume boundary: (" << vertStore[vertIndex * 3 + 0] << ", " << vertStore[vertIndex * 3 + 1]
                                << ", " << vertStore[vertIndex * 3 + 2] << ")");
      REQUIRE(onBoundary);
    }
  }
}

// Runs M3C on the shared QuickSurfaceMesh Small IN100 test dataset and returns nothing;
// asserts a valid, non-empty, well-formed mesh. `repairWinding` toggles the winding pass.
void RunM3C(bool repairWinding, [[maybe_unused]] const std::string& outputName)
{
  UnitTest::LoadPlugins();

  // Reuse the QuickSurfaceMesh test data (Small IN100 segmented volume).
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");
  auto baseDataFilePath = fs::path(fmt::format("{}/QuickSurfaceMeshTest_v2/QuickSurfaceMeshTest_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});

  DataPath cellDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  DataPath computedTriangleGeomPath({"Computed M3C Mesh"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);

  // Transfer every Cell and Feature attribute array (mirrors the QuickSurfaceMesh test).
  MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
  for(const auto& [id, child] : dataStructure.getDataRefAs<AttributeMatrix>(cellDataPath))
  {
    selectedCellArrayPaths.push_back(cellDataPath.createChildPath(child->getName()));
  }
  MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
  for(const auto& [id, child] : dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath))
  {
    selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child->getName()));
  }

  {
    Arguments args;
    M3CSurfaceMeshingFilter filter;

    args.insertOrAssign(M3CSurfaceMeshingFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(repairWinding));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, outputName)));
#endif
  }

  // There is no M3C exemplar yet, so validate that the mesh is present and well-formed:
  // non-empty, and every triangle references a vertex index in range.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  auto& triStore = triangleGeom.getFaces()->getDataStoreRef();
  auto& vertStore = triangleGeom.getVertices()->getDataStoreRef();

  const usize numTriangles = triangleGeom.getNumberOfFaces();
  const usize numVertices = triangleGeom.getNumberOfVertices();
  REQUIRE(numTriangles > 0);
  REQUIRE(numVertices > 0);

  // FaceLabels (int32 x2) and NodeTypes (int8) should be tuple-consistent with the geometry.
  const auto& faceLabels = dataStructure.getDataRefAs<Int32Array>(faceGroupDataPath.createChildPath(k_Face_Labels));
  const auto& nodeTypes = dataStructure.getDataRefAs<Int8Array>(vertexGroupDataPath.createChildPath(k_NodeTypeArrayName));
  REQUIRE(faceLabels.getNumberOfTuples() == numTriangles);
  REQUIRE(faceLabels.getNumberOfComponents() == 2);
  REQUIRE(nodeTypes.getNumberOfTuples() == numVertices);

  // FaceLabels must have the smaller feature id in component 0 (QuickSurfaceMesh/SurfaceNets convention).
  const auto& faceLabelsStore = faceLabels.getDataStoreRef();
  for(usize i = 0; i < numTriangles; i++)
  {
    REQUIRE(faceLabelsStore[i * 2] <= faceLabelsStore[i * 2 + 1]);
  }

  // Each transferred Cell/Feature array must exist on the face group with the component shape
  // doubled (one value per side of the face) and one tuple per triangle.
  auto checkTransferred = [&](const std::vector<DataPath>& selectedPaths) {
    for(const auto& sourcePath : selectedPaths)
    {
      const auto& source = dataStructure.getDataRefAs<IDataArray>(sourcePath);
      DataPath transferredPath = faceGroupDataPath.createChildPath(sourcePath.getTargetName());
      const auto* transferred = dataStructure.getDataAs<IDataArray>(transferredPath);
      REQUIRE(transferred != nullptr);
      REQUIRE(transferred->getNumberOfTuples() == numTriangles);
      REQUIRE(transferred->getNumberOfComponents() == 2 * source.getNumberOfComponents());
    }
  };
  checkTransferred(selectedCellArrayPaths);
  checkTransferred(selectedFeatureArrayPaths);

  for(usize i = 0; i < numTriangles; i++)
  {
    for(usize v = 0; v < 3; v++)
    {
      REQUIRE(triStore[i * 3 + v] < numVertices);
    }
  }
  (void)vertStore;

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Default", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3C(false, "M3CSurfaceMeshingFilterTest.dream3d");
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Winding", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3C(true, "M3CSurfaceMeshingFilterTest_Winding.dream3d");
}

namespace
{
// Builds a small in-memory ImageGeom + FeatureIds volume from a labeling functor, runs M3C, and
// asserts a valid, non-empty, well-formed mesh. Used for tiny toy datasets that deterministically
// exercise specific get_square_index cases (saddles, quad points, triple lines) with no data file.
template <typename LabelFuncT>
void RunM3COnToy(usize xDim, usize yDim, usize zDim, LabelFuncT&& labeler, [[maybe_unused]] const std::string& outputName)
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({xDim, yDim, zDim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  const std::vector<usize> tupleDims = {zDim, yDim, xDim};
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", tupleDims, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", tupleDims, std::vector<usize>{1}, cellAM->getId());
  auto& fidStore = featureIds->getDataStoreRef();
  usize idx = 0;
  for(usize z = 0; z < zDim; z++)
  {
    for(usize y = 0; y < yDim; y++)
    {
      for(usize x = 0; x < xDim; x++)
      {
        fidStore[idx++] = labeler(x, y, z);
      }
    }
  }

  DataPath gridGeomDataPath({"ImageGeometry"});
  DataPath featureIdsDataPath({"ImageGeometry", "Cell Data", "FeatureIds"});
  DataPath computedTriangleGeomPath({"Computed M3C Mesh"});

  Arguments args;
  M3CSurfaceMeshingFilter filter;
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  auto& triStore = triangleGeom.getFaces()->getDataStoreRef();
  const usize numTriangles = triangleGeom.getNumberOfFaces();
  const usize numVertices = triangleGeom.getNumberOfVertices();
  REQUIRE(numTriangles > 0);
  REQUIRE(numVertices > 0);
  for(usize i = 0; i < numTriangles; i++)
  {
    for(usize v = 0; v < 3; v++)
    {
      REQUIRE(triStore[i * 3 + v] < numVertices);
    }
  }

  // Coordinate alignment: with unit spacing and origin 0, mesh vertices must lie within the padded
  // volume envelope [-1, dim] per axis. A one-cell coordinate offset would push the maximum past dim.
  auto& vertStore = triangleGeom.getVertices()->getDataStoreRef();
  const float dimF[3] = {static_cast<float>(xDim), static_cast<float>(yDim), static_cast<float>(zDim)};
  for(usize i = 0; i < numVertices; i++)
  {
    for(usize c = 0; c < 3; c++)
    {
      REQUIRE(vertStore[i * 3 + c] >= -1.0f);
      REQUIRE(vertStore[i * 3 + c] <= dimF[c]);
    }
  }

  CheckMeshIntegrity(dataStructure, computedTriangleGeomPath, computedTriangleGeomPath.createChildPath(k_FaceDataGroupName).createChildPath(k_Face_Labels),
                     computedTriangleGeomPath.createChildPath(k_VertexDataGroupName).createChildPath(k_NodeTypeArrayName));

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, outputName)));
#endif
}
} // namespace

// 3D 2-label checkerboard: every marching square has corners [A,B,A,B] (aBit[0..3]=1, both
// diagonals equal) => get_square_index == 15 => treat_anomaly runs on EVERY effective square.
// This is the minimal deterministic regression for the null-neighbor crash in treat_anomaly.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Checkerboard Saddle", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(8, 8, 8, [](usize x, usize y, usize z) -> int32 { return static_cast<int32>(((x + y + z) & 1U) + 1); }, "M3CSurfaceMeshingFilterTest_Checkerboard.dream3d");
}

// 8-label octant pattern (1 + x%2 + 2*(y%2) + 4*(z%2)): a repeating 2x2x2 of 8 distinct labels, so
// squares present 4 distinct corners (quad points, get_square_index == 19) plus triple configs.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Octant Quad Points", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(8, 8, 8, [](usize x, usize y, usize z) -> int32 { return static_cast<int32>(1 + (x & 1U) + 2 * (y & 1U) + 4 * (z & 1U)); }, "M3CSurfaceMeshingFilterTest_Octant.dream3d");
}

// Three regions meeting along a vertical line (L-shaped split) => triple lines
// (get_square_index in {7,11,13,14}) plus ordinary binary interfaces.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Triple Line", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(8, 8, 8, [](usize x, usize y, usize /*z*/) -> int32 { return (x < 4) ? 1 : ((y < 4) ? 2 : 3); }, "M3CSurfaceMeshingFilterTest_TripleLine.dream3d");
}

// Single isolated interior voxel in a uniform matrix: the four neighboring squares are [B,A,A,A]
// rotations, exercising get_square_index in {3,6,9,12} (the single-corner binary configurations).
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Single Voxel Inclusion", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(8, 8, 8, [](usize x, usize y, usize z) -> int32 { return (x == 4 && y == 4 && z == 4) ? 2 : 1; }, "M3CSurfaceMeshingFilterTest_Inclusion.dream3d");
}

// Interleaved 3-label tiling (rows "1 2" / "3 1"): squares [1,2,1,3] and [2,1,3,1] have all edges
// differing with exactly one diagonal equal, exercising get_square_index 17 and 18.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Interleaved Diagonal", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(8, 8, 8, [](usize x, usize y, usize /*z*/) -> int32 { return (y % 2 == 0) ? ((x % 2 == 0) ? 1 : 2) : ((x % 2 == 0) ? 3 : 1); }, "M3CSurfaceMeshingFilterTest_Interleaved.dream3d");
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][M3CSurfaceMeshingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / "6_5" / "M3CSurfaceMeshingFilter.json";

  auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
  REQUIRE(pipelineResult.valid());

  auto& pipeline = pipelineResult.value();
  REQUIRE(pipeline.size() == 1);

  auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
  REQUIRE(pipelineFilter != nullptr);

  const IFilter* filter = pipelineFilter->getFilter();
  REQUIRE(filter != nullptr);
  REQUIRE(filter->uuid() == FilterTraits<M3CSurfaceMeshingFilter>::uuid);

  const Arguments args = pipelineFilter->getArguments();
  CHECK(args.value<DataPath>(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key) == DataPath({"DataContainer"}));
  CHECK(args.value<DataPath>(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  CHECK(args.value<DataPath>(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
  CHECK(args.value<std::string>(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key) == "TestName");
  CHECK(args.value<std::string>(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key) == "TestName");
  CHECK(args.value<std::string>(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key) == "TestName");
  CHECK(args.value<std::string>(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key) == "TestName");
}

namespace
{
// Self-generated ("circular") regression oracle: the exemplar is produced BY this filter, so it only
// guards against future *changes* to the output, not against correctness of the current output. It
// should be replaced by an independent oracle (e.g. legacy DREAM3D M3C output) when available.
const fs::path k_ExemplarFile = fs::path(nx::core::unit_test::k_TestFilesDir.view()) / "M3CSurfaceMeshingExemplar_v2" / "M3CSurfaceMeshingExemplar_v2.dream3d";
const DataPath k_ExemplarMeshPath({"Computed M3C Mesh"});

// Runs M3C (winding repair on) on an already-loaded Small IN100 input, creating k_ExemplarMeshPath.
void RunM3COnSmallIn100(DataStructure& dataStructure)
{
  Arguments args;
  M3CSurfaceMeshingFilter filter;
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({k_DataContainer})));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_DataContainer, k_CellData, k_FeatureIds})));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ExemplarMeshPath));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

DataStructure LoadSmallIn100Input()
{
  auto inputPath = fs::path(fmt::format("{}/QuickSurfaceMeshTest_v2/QuickSurfaceMeshTest_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  return UnitTest::LoadDataStructure(inputPath);
}
} // namespace

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Preflight Error Paths", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // Tuple-mismatch error (-90200): a selected Cell transfer array whose tuple count (5) differs from
  // the FeatureIds tuple count (8) must fail preflight.
  SECTION("Cell transfer array tuple mismatch -> error -90200")
  {
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeom->setDimensions({2, 2, 2});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
    auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", std::vector<usize>{2, 2, 2}, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", std::vector<usize>{2, 2, 2}, std::vector<usize>{1}, cellAM->getId());
    // A Cell array with a deliberately wrong tuple count (5 != 8).
    auto* badAM = AttributeMatrix::Create(dataStructure, "Bad", std::vector<usize>{5}, imageGeom->getId());
    Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "BadArray", std::vector<usize>{5}, std::vector<usize>{1}, badAM->getId());

    Arguments args;
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "Cell Data", "FeatureIds"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Mesh"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"ImageGeometry", "Bad", "BadArray"})}));

    M3CSurfaceMeshingFilter filter;
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
    bool has90200 = false;
    for(const auto& err : preflightResult.outputActions.errors())
    {
      if(err.code == -90200)
      {
        has90200 = true;
      }
    }
    REQUIRE(has90200);
  }

  // RectGrid geometries are rejected by the geometry parameter (M3C node coordinates assume uniform
  // cell spacing, so only ImageGeom is an allowed input type).
  SECTION("RectGrid geometry -> preflight fails")
  {
    DataStructure dataStructure;
    auto* rectGrid = RectGridGeom::Create(dataStructure, "RectGrid");
    rectGrid->setDimensions({2, 2, 2});
    const char* axes[3] = {"xBounds", "yBounds", "zBounds"};
    Float32Array* bounds[3];
    for(int a = 0; a < 3; a++)
    {
      bounds[a] = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, axes[a], std::vector<usize>{3}, std::vector<usize>{1}, rectGrid->getId());
      bounds[a]->setValue(0, 0.0f);
      bounds[a]->setValue(1, 1.0f);
      bounds[a]->setValue(2, 2.0f);
    }
    rectGrid->setXBoundsId(bounds[0]->getId());
    rectGrid->setYBoundsId(bounds[1]->getId());
    rectGrid->setZBoundsId(bounds[2]->getId());
    auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", std::vector<usize>{2, 2, 2}, rectGrid->getId());
    rectGrid->setCellData(*cellAM);
    Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", std::vector<usize>{2, 2, 2}, std::vector<usize>{1}, cellAM->getId());

    Arguments args;
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"RectGrid"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"RectGrid", "Cell Data", "FeatureIds"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Mesh"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));

    M3CSurfaceMeshingFilter filter;
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
  }
}

// Hidden test: regenerate the exemplar .dream3d from the current output. Run explicitly with the tag
// [.][M3CGenerateExemplar], then upload the file to the DREAM3D Data_Archive.
// PROVENANCE TRIPWIRE - read before using this.
//
// This hidden helper writes an M3C mesh to disk. It must NOT be used to recreate a golden mesh that
// M3C is then tested against. Doing so builds a circular oracle: the "expected" output is whatever
// this filter last produced, so any future defect that changes the mesh is simply re-baselined as
// correct. That is precisely how the half-cell offset and the ghost-shell surface survived - the
// retired exemplar had frozen both, and every byte-for-byte comparison against it passed.
//
// M3C is validated instead by properties derived from the input geometry alone (see
// CheckMeshIntegrity and CheckMeshWithinVolume), which fail on a wrong mesh rather than adopting it.
// If a stored mesh is ever genuinely needed - for cross-version diffing, say - it must be validated
// by those invariants first, and its provenance recorded.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Generate Exemplar", "[.][M3CGenerateExemplar]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  DataStructure dataStructure = LoadSmallIn100Input();
  RunM3COnSmallIn100(dataStructure);

  // Drop everything except the generated mesh (the input file also carries QuickMesh exemplar
  // geometries) so the exemplar archive holds only the M3C output.
  for(DataObject* topLevelPtr : dataStructure.getTopLevelData())
  {
    if(topLevelPtr->getName() != k_ExemplarMeshPath.getTargetName())
    {
      dataStructure.removeData(topLevelPtr->getId());
    }
  }
  fs::create_directories(k_ExemplarFile.parent_path());
  WriteTestDataStructure(dataStructure, k_ExemplarFile);
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Small IN100 structural validation", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();
  // Only the INPUT dataset is needed. The M3CSurfaceMeshingExemplar_v2 archive is no longer read -
  // see the note below on why the golden comparison was replaced.
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  DataStructure dataStructure = LoadSmallIn100Input();
  RunM3COnSmallIn100(dataStructure);

  const DataPath faceLabelsPath = k_ExemplarMeshPath.createChildPath(k_FaceDataGroupName).createChildPath(k_Face_Labels);
  const DataPath nodeTypesPath = k_ExemplarMeshPath.createChildPath(k_VertexDataGroupName).createChildPath(k_NodeTypeArrayName);

  // Validated against first-principles invariants rather than a stored golden mesh.
  //
  // The previous version of this test compared vertices, faces, FaceLabels and NodeTypes byte for
  // byte against M3CSurfaceMeshingExemplar_v2. That exemplar was generated by this filter while it
  // still placed roughly half of every mesh outside the source volume, so it cannot be used to
  // validate the corrected output. Regenerating it from the fixed filter and then testing the fixed
  // filter against it would be a circular oracle - it would pin whatever the code happens to emit
  // rather than establish that the emitted mesh is right.
  //
  // The checks below are derived from the input geometry alone and would have FAILED on the old
  // behaviour, so they validate the mesher rather than freeze it.
  CheckMeshIntegrity(dataStructure, k_ExemplarMeshPath, faceLabelsPath, nodeTypesPath);
  CheckMeshWithinVolume(dataStructure, k_ExemplarMeshPath, faceLabelsPath, DataPath({k_DataContainer}));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Mesh lies within the source volume", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // Every cell is its own Feature, so interfaces exist along all three axes and no axis can pass by
  // accident. A non-unit spacing and a non-zero origin are used deliberately: the two defects this
  // guards against were a half-CELL offset and a one-CELL overhang, either of which would be masked
  // by the common origin=0, spacing=1 case where those magnitudes coincide with round numbers.
  auto [dimX, dimY, dimZ] = GENERATE(std::make_tuple(2, 2, 2), std::make_tuple(3, 2, 4), std::make_tuple(2, 2, 1));
  const FloatVec3 origin(10.0f, -5.0f, 2.5f);
  const FloatVec3 spacing(0.25f, 2.0f, 0.5f);

  DYNAMIC_SECTION("dims " << dimX << "x" << dimY << "x" << dimZ)
  {
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeom->setDimensions({static_cast<usize>(dimX), static_cast<usize>(dimY), static_cast<usize>(dimZ)});
    imageGeom->setSpacing(spacing);
    imageGeom->setOrigin(origin);
    const std::vector<usize> tupleShape = {static_cast<usize>(dimZ), static_cast<usize>(dimY), static_cast<usize>(dimX)};
    auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", tupleShape, std::vector<usize>{1}, cellAM->getId());
    auto& featureIdsRef = featureIds->getDataStoreRef();
    for(usize i = 0; i < featureIdsRef.getNumberOfTuples(); i++)
    {
      featureIdsRef[i] = static_cast<int32>(i) + 1;
    }

    M3CSurfaceMeshingFilter filter;
    Arguments args;
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "Cell Data", "FeatureIds"})));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ExemplarMeshPath));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath faceLabelsPath = k_ExemplarMeshPath.createChildPath(k_FaceDataGroupName).createChildPath(k_Face_Labels);
    const DataPath nodeTypesPath = k_ExemplarMeshPath.createChildPath(k_VertexDataGroupName).createChildPath(k_NodeTypeArrayName);

    CheckMeshIntegrity(dataStructure, k_ExemplarMeshPath, faceLabelsPath, nodeTypesPath);
    CheckMeshWithinVolume(dataStructure, k_ExemplarMeshPath, faceLabelsPath, DataPath({"ImageGeometry"}));

    // The mesh must also REACH the volume boundary on every axis, not merely stay inside it. Without
    // this a mesher that collapsed everything to a point would satisfy the bounds check above.
    const auto& tri = dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarMeshPath);
    const auto& vertStore = tri.getVertices()->getDataStoreRef();
    const std::array<usize, 3> dims = {static_cast<usize>(dimX), static_cast<usize>(dimY), static_cast<usize>(dimZ)};
    for(usize ax = 0; ax < 3; ax++)
    {
      float32 minC = std::numeric_limits<float32>::max();
      float32 maxC = std::numeric_limits<float32>::lowest();
      for(usize i = 0; i < tri.getNumberOfVertices(); i++)
      {
        minC = std::min(minC, vertStore[i * 3 + ax]);
        maxC = std::max(maxC, vertStore[i * 3 + ax]);
      }
      const float32 expectedLo = origin[ax];
      const float32 expectedHi = origin[ax] + static_cast<float32>(dims[ax]) * spacing[ax];
      INFO("axis " << ax << " spans [" << minC << ", " << maxC << "], volume spans [" << expectedLo << ", " << expectedHi << "]");
      REQUIRE(minC == Approx(expectedLo));
      REQUIRE(maxC == Approx(expectedHi));
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Single feature meshes exactly the bounding box", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // A volume containing one Feature has no interior interfaces at all, so the entire mesh is the
  // exterior surface and must be exactly the bounding box of the volume - nothing inside it, nothing
  // outside it. That makes the expected answer fully hand-derivable, independent of any stored mesh.
  //
  // This is the case the ghost-shell defect distorted most visibly: with six distinct sentinels the
  // shell triangulated against itself and wrapped the box in extra surface.
  const FloatVec3 origin(-1.0f, 3.0f, 0.5f);
  const FloatVec3 spacing(2.0f, 0.5f, 1.5f);
  const usize dimX = 3, dimY = 2, dimZ = 2;

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);
  const std::vector<usize> tupleShape = {dimZ, dimY, dimX};
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", tupleShape, std::vector<usize>{1}, cellAM->getId());
  auto& featureIdsRef = featureIds->getDataStoreRef();
  for(usize i = 0; i < featureIdsRef.getNumberOfTuples(); i++)
  {
    featureIdsRef[i] = 1;
  }

  M3CSurfaceMeshingFilter filter;
  Arguments args;
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "Cell Data", "FeatureIds"})));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ExemplarMeshPath));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath faceLabelsPath = k_ExemplarMeshPath.createChildPath(k_FaceDataGroupName).createChildPath(k_Face_Labels);
  const auto& tri = dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarMeshPath);
  const auto& vertStore = tri.getVertices()->getDataStoreRef();
  const auto& faceLabelStore = dataStructure.getDataRefAs<Int32Array>(faceLabelsPath).getDataStoreRef();

  const std::array<float32, 3> lo = {origin[0], origin[1], origin[2]};
  const std::array<float32, 3> hi = {origin[0] + static_cast<float32>(dimX) * spacing[0], origin[1] + static_cast<float32>(dimY) * spacing[1], origin[2] + static_cast<float32>(dimZ) * spacing[2]};

  // With one feature every face must separate that feature from the outside.
  for(usize i = 0; i < tri.getNumberOfFaces(); i++)
  {
    INFO("triangle " << i << " FaceLabels (" << faceLabelStore[i * 2 + 0] << ", " << faceLabelStore[i * 2 + 1] << ")");
    REQUIRE(faceLabelStore[i * 2 + 0] == -1);
    REQUIRE(faceLabelStore[i * 2 + 1] == 1);
  }

  // Every vertex must lie on the surface of the bounding box - on at least one of the six planes -
  // and none may lie strictly inside it.
  constexpr float32 k_Tol = 1.0e-4f;
  for(usize i = 0; i < tri.getNumberOfVertices(); i++)
  {
    bool onBoundary = false;
    for(usize ax = 0; ax < 3 && !onBoundary; ax++)
    {
      const float32 c = vertStore[i * 3 + ax];
      onBoundary = (std::abs(c - lo[ax]) <= k_Tol) || (std::abs(c - hi[ax]) <= k_Tol);
    }
    INFO("vertex " << i << " (" << vertStore[i * 3 + 0] << ", " << vertStore[i * 3 + 1] << ", " << vertStore[i * 3 + 2] << ") is not on the bounding box");
    REQUIRE(onBoundary);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
// Portable environment-variable mutators. POSIX declares setenv()/unsetenv(), but MSVC does not,
// so plain std::getenv() is used for reads everywhere else in this repo while writes need this
// shim. MSVC's _putenv_s() removes a variable when given an empty string, which is the closest
// portable equivalent to unsetenv().
#ifdef _WIN32
void SetEnvVar(const std::string& name, const char* value)
{
  _putenv_s(name.c_str(), value != nullptr ? value : "");
}

void UnsetEnvVar(const std::string& name)
{
  _putenv_s(name.c_str(), "");
}
#else
void SetEnvVar(const std::string& name, const char* value)
{
  ::setenv(name.c_str(), value, 1);
}

void UnsetEnvVar(const std::string& name)
{
  ::unsetenv(name.c_str());
}
#endif

// Temporarily sets (or clears) an environment variable for the lifetime of the object, restoring
// whatever was there before on destruction. Used below to select M3C's sweep path, which is read
// once from the environment at the top of M3CSurfaceMeshing::operator()().
class ScopedEnvVar
{
public:
  ScopedEnvVar(std::string name, const char* value)
  : m_Name(std::move(name))
  {
    if(const char* previous = std::getenv(m_Name.c_str()); previous != nullptr)
    {
      m_HadPrevious = true;
      m_PreviousValue = previous;
    }
    if(value != nullptr)
    {
      SetEnvVar(m_Name, value);
    }
    else
    {
      UnsetEnvVar(m_Name);
    }
  }

  ~ScopedEnvVar()
  {
    if(m_HadPrevious)
    {
      SetEnvVar(m_Name, m_PreviousValue.c_str());
    }
    else
    {
      UnsetEnvVar(m_Name);
    }
  }

  ScopedEnvVar(const ScopedEnvVar&) = delete;
  ScopedEnvVar(ScopedEnvVar&&) noexcept = delete;
  ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;
  ScopedEnvVar& operator=(ScopedEnvVar&&) noexcept = delete;

private:
  std::string m_Name;
  bool m_HadPrevious = false;
  std::string m_PreviousValue;
};

const DataPath k_SweepPathTriGeomPath({"M3CSweepPathMesh"});

// Runs the flush-with-bottom cylinder (Task 1's shared test dataset) through M3C with the Bounding
// Box Skin option's 'Background-Backed Walls Only' mode enabled, forcing the sweep path selected by the two M3C_* environment variables (see
// M3CSurfaceMeshing::operator()()). Passing false/false for both selects the default parallel
// sliding-window path.
SurfaceMeshingTest::MeshResult RunM3COmitSkinOnPath(bool wholeVolume, bool serial)
{
  const ScopedEnvVar wholeVolumeVar("M3C_WHOLE_VOLUME", wholeVolume ? "1" : nullptr);
  const ScopedEnvVar serialVar("M3C_SERIAL", serial ? "1" : nullptr);
  return SurfaceMeshingTest::RunMesher<M3CSurfaceMeshingFilter>(SurfaceMeshingTest::CreateCylinderInBox(true), k_SweepPathTriGeomPath, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly,
                                                                [](Arguments&) {});
}
} // namespace

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Bounding Box Skin agrees across sweep paths", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // M3C's two serial paths (runEntireVolume and runWindowed(false)) are documented as byte-identical to
  // each other, including the exact triangulation. The default parallel path (runWindowed(true)) is
  // documented to produce byte-identical vertices, FaceLabels, and NodeTypes, but MAY legitimately
  // triangulate the same interfaces differently (the legacy per-cube loop triangulation depends on
  // cross-cube edge-flip propagation, which is inherently serial) -- so its Shared Faces List is not
  // compared for exact equality here. Confirm those same guarantees still hold with the Bounding Box
  // Skin option's 'Background-Backed Walls Only' mode enabled: the prune runs once, inside the shared
  // finalizeMesh(), after all three paths have produced their triangles/mCubeID vectors, so it should
  // not disturb this invariant either way.
  SurfaceMeshingTest::MeshResult wholeVolumeResult = RunM3COmitSkinOnPath(/*wholeVolume*/ true, /*serial*/ false);
  SurfaceMeshingTest::MeshResult serialWindowedResult = RunM3COmitSkinOnPath(/*wholeVolume*/ false, /*serial*/ true);
  SurfaceMeshingTest::MeshResult parallelWindowedResult = RunM3COmitSkinOnPath(/*wholeVolume*/ false, /*serial*/ false);

  REQUIRE_NOTHROW(wholeVolumeResult.Structure.getDataRefAs<TriangleGeom>(k_SweepPathTriGeomPath));
  REQUIRE_NOTHROW(serialWindowedResult.Structure.getDataRefAs<TriangleGeom>(k_SweepPathTriGeomPath));
  REQUIRE_NOTHROW(parallelWindowedResult.Structure.getDataRefAs<TriangleGeom>(k_SweepPathTriGeomPath));
  const auto& wholeVolumeGeom = wholeVolumeResult.Structure.getDataRefAs<TriangleGeom>(k_SweepPathTriGeomPath);
  const auto& serialWindowedGeom = serialWindowedResult.Structure.getDataRefAs<TriangleGeom>(k_SweepPathTriGeomPath);
  const auto& parallelWindowedGeom = parallelWindowedResult.Structure.getDataRefAs<TriangleGeom>(k_SweepPathTriGeomPath);

  REQUIRE(wholeVolumeGeom.getNumberOfVertices() == serialWindowedGeom.getNumberOfVertices());
  REQUIRE(wholeVolumeGeom.getNumberOfVertices() == parallelWindowedGeom.getNumberOfVertices());
  REQUIRE(wholeVolumeGeom.getNumberOfFaces() == serialWindowedGeom.getNumberOfFaces());
  REQUIRE(wholeVolumeGeom.getNumberOfFaces() == parallelWindowedGeom.getNumberOfFaces());

  // Vertices (Shared Vertex List) must match exactly across all three paths.
  UnitTest::CompareArrays<float32>(wholeVolumeGeom.getVertices(), serialWindowedGeom.getVertices());
  UnitTest::CompareArrays<float32>(wholeVolumeGeom.getVertices(), parallelWindowedGeom.getVertices());

  // Shared Faces List: only the two serial paths are guaranteed to match exactly (see comment above).
  UnitTest::CompareArrays<IGeometry::MeshIndexType>(wholeVolumeGeom.getFaces(), serialWindowedGeom.getFaces());

  // RunM3COmitSkinOnPath (via SurfaceMeshingTest::RunMesher) creates "FaceLabels" and "NodeTypes" --
  // NOT the nx::core::UnitTest::k_Face_Labels ("Face Labels") / k_NodeTypeArrayName ("Node Type")
  // constants used elsewhere in this file for the SIMPL-compatibility/exemplar tests.
  const DataPath faceLabelsPath = wholeVolumeResult.FaceLabelsPath;
  const DataPath nodeTypesPath = k_SweepPathTriGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");

  // Face Labels must match exactly across all three paths.
  UnitTest::CompareArrays<int32>(wholeVolumeResult.Structure.getDataAs<IArray>(faceLabelsPath), serialWindowedResult.Structure.getDataAs<IArray>(faceLabelsPath));
  UnitTest::CompareArrays<int32>(wholeVolumeResult.Structure.getDataAs<IArray>(faceLabelsPath), parallelWindowedResult.Structure.getDataAs<IArray>(faceLabelsPath));

  // Node Types must match exactly across all three paths.
  UnitTest::CompareArrays<int8>(wholeVolumeResult.Structure.getDataAs<IArray>(nodeTypesPath), serialWindowedResult.Structure.getDataAs<IArray>(nodeTypesPath));
  UnitTest::CompareArrays<int8>(wholeVolumeResult.Structure.getDataAs<IArray>(nodeTypesPath), parallelWindowedResult.Structure.getDataAs<IArray>(nodeTypesPath));

  UnitTest::CheckArraysInheritTupleDims(wholeVolumeResult.Structure);
  UnitTest::CheckArraysInheritTupleDims(serialWindowedResult.Structure);
  UnitTest::CheckArraysInheritTupleDims(parallelWindowedResult.Structure);
}
// Characterization test for an OPEN BUG in M3CSurfaceMeshing, pre-existing and unrelated to triple-line
// generation itself (triple-line extraction only made it visible).
//
// On a 2x2x1 four-grain block (domain spanning z in [0, 1]), M3C emits a spurious extra triple-line
// vertex/edge at z = -0.5 -- one full cell BELOW the domain -- carrying NumFeatures == 4 as if it were a
// genuine quadruple-point junction. The real physical quadruple line is a single vertical segment
// through the shared corner of the four grains (1 edge); M3C instead reports 2 collinear edges split at
// z = 0, with the extra segment/vertex sitting in what can only be a ghost/padding cell layer that M3C
// synthesizes to close the marching-cubes cube on a domain that is only 1 cell thick in Z.
//
// Likely cause: M3C's ghost-shell handling clamps/replicates real feature labels into the padding layer
// on a 1-cell-thick domain instead of using a background label, so the padding cube looks like an
// ordinary interior 4-feature junction to the exterior-triple-line filtering logic.
//
// This is a PRE-EXISTING M3C defect: this branch changed nothing in M3C's meshing (it only appended a
// triple-line-generation call at the end of finalizeMesh), so M3C's mesh and FaceLabels here are
// byte-identical to before this branch. The assertions below encode the CORRECT expected behavior (an
// EdgeGeom confined to the real domain, z in [0, 1]); they currently fail against the actual (buggy)
// output. The [!shouldfail] tag makes Catch2 report that as an expected failure so CI stays green.
// Once M3C's ghost-shell handling is fixed, this test will start passing -- at that point remove the
// [!shouldfail] tag so it becomes a normal regression test.
