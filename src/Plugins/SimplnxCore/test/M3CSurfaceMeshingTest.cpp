#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <cstdlib>
#include <filesystem>
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

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Exemplar Comparison", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");
  const nx::core::UnitTest::TestFileSentinel exemplarSentinel(nx::core::unit_test::k_TestFilesDir, "M3CSurfaceMeshingExemplar_v2.tar.gz", "M3CSurfaceMeshingExemplar_v2");

  DataStructure dataStructure = LoadSmallIn100Input();
  RunM3COnSmallIn100(dataStructure);

  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarMeshPath));
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<TriangleGeom>(k_ExemplarMeshPath));
  const auto& computedGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarMeshPath);
  const auto& exemplarGeom = exemplarDS.getDataRefAs<TriangleGeom>(k_ExemplarMeshPath);

  REQUIRE(computedGeom.getNumberOfVertices() == exemplarGeom.getNumberOfVertices());
  REQUIRE(computedGeom.getNumberOfFaces() == exemplarGeom.getNumberOfFaces());

  // Vertices, faces, and the face/vertex attribute arrays must match the golden reference exactly.
  UnitTest::CompareArrays<float32>(computedGeom.getVertices(), exemplarGeom.getVertices());
  UnitTest::CompareArrays<IGeometry::MeshIndexType>(computedGeom.getFaces(), exemplarGeom.getFaces());

  const DataPath faceLabelsPath = k_ExemplarMeshPath.createChildPath(k_FaceDataGroupName).createChildPath(k_Face_Labels);
  const DataPath nodeTypesPath = k_ExemplarMeshPath.createChildPath(k_VertexDataGroupName).createChildPath(k_NodeTypeArrayName);
  UnitTest::CompareArrays<int32>(dataStructure.getDataAs<IArray>(faceLabelsPath), exemplarDS.getDataAs<IArray>(faceLabelsPath));
  UnitTest::CompareArrays<int8>(dataStructure.getDataAs<IArray>(nodeTypesPath), exemplarDS.getDataAs<IArray>(nodeTypesPath));

  // Independent structural validation of the (default, multithreaded) output on real data.
  CheckMeshIntegrity(dataStructure, k_ExemplarMeshPath, faceLabelsPath, nodeTypesPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
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
      setenv(m_Name.c_str(), value, 1);
    }
    else
    {
      unsetenv(m_Name.c_str());
    }
  }

  ~ScopedEnvVar()
  {
    if(m_HadPrevious)
    {
      setenv(m_Name.c_str(), m_PreviousValue.c_str(), 1);
    }
    else
    {
      unsetenv(m_Name.c_str());
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

// Runs the flush-with-bottom cylinder (Task 1's shared test dataset) through M3C with Omit Bounding
// Box Skin enabled, forcing the sweep path selected by the two M3C_* environment variables (see
// M3CSurfaceMeshing::operator()()). Passing false/false for both selects the default parallel
// sliding-window path.
SurfaceMeshingTest::MeshResult RunM3COmitSkinOnPath(bool wholeVolume, bool serial)
{
  const ScopedEnvVar wholeVolumeVar("M3C_WHOLE_VOLUME", wholeVolume ? "1" : nullptr);
  const ScopedEnvVar serialVar("M3C_SERIAL", serial ? "1" : nullptr);
  return SurfaceMeshingTest::RunMesher<M3CSurfaceMeshingFilter>(SurfaceMeshingTest::CreateCylinderInBox(true), k_SweepPathTriGeomPath, /*omitSkin*/ true, [](Arguments&) {});
}
} // namespace

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Omit Bounding Box Skin agrees across sweep paths", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // M3C's two serial paths (runEntireVolume and runWindowed(false)) are documented as byte-identical to
  // each other, including the exact triangulation. The default parallel path (runWindowed(true)) is
  // documented to produce byte-identical vertices, FaceLabels, and NodeTypes, but MAY legitimately
  // triangulate the same interfaces differently (the legacy per-cube loop triangulation depends on
  // cross-cube edge-flip propagation, which is inherently serial) -- so its Shared Faces List is not
  // compared for exact equality here. Confirm those same guarantees still hold with Omit Bounding Box
  // Skin enabled: the prune runs once, inside the shared finalizeMesh(), after all three paths have
  // produced their triangles/mCubeID vectors, so it should not disturb this invariant either way.
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
