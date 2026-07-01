#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
// Runs M3C on the shared QuickSurfaceMesh Small IN100 test dataset and returns nothing;
// asserts a valid, non-empty, well-formed mesh. `repairWinding` toggles the winding pass.
void RunM3C(bool repairWinding, const std::string& outputName)
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

    // NOTE: temporarily unconditional so the mesh can be visually inspected during bring-up.
    // Restore the `#ifdef SIMPLNX_WRITE_TEST_OUTPUT` guard before finalizing this test.
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, outputName)));
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
void RunM3COnToy(usize xDim, usize yDim, usize zDim, LabelFuncT&& labeler, const std::string& outputName)
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

  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, outputName)));
}
} // namespace

// 3D 2-label checkerboard: every marching square has corners [A,B,A,B] (aBit[0..3]=1, both
// diagonals equal) => get_square_index == 15 => treat_anomaly runs on EVERY effective square.
// This is the minimal deterministic regression for the null-neighbor crash in treat_anomaly.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Checkerboard Saddle", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(
      8, 8, 8, [](usize x, usize y, usize z) -> int32 { return static_cast<int32>(((x + y + z) & 1U) + 1); }, "M3CSurfaceMeshingFilterTest_Checkerboard.dream3d");
}

// 8-label octant pattern (1 + x%2 + 2*(y%2) + 4*(z%2)): a repeating 2x2x2 of 8 distinct labels, so
// squares present 4 distinct corners (quad points, get_square_index == 19) plus triple configs.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Octant Quad Points", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(
      8, 8, 8, [](usize x, usize y, usize z) -> int32 { return static_cast<int32>(1 + (x & 1U) + 2 * (y & 1U) + 4 * (z & 1U)); }, "M3CSurfaceMeshingFilterTest_Octant.dream3d");
}

// Three regions meeting along a vertical line (L-shaped split) => triple lines
// (get_square_index in {7,11,13,14}) plus ordinary binary interfaces.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Triple Line", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(
      8, 8, 8, [](usize x, usize y, usize /*z*/) -> int32 { return (x < 4) ? 1 : ((y < 4) ? 2 : 3); }, "M3CSurfaceMeshingFilterTest_TripleLine.dream3d");
}

// Single isolated interior voxel in a uniform matrix: the four neighboring squares are [B,A,A,A]
// rotations, exercising get_square_index in {3,6,9,12} (the single-corner binary configurations).
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Single Voxel Inclusion", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(
      8, 8, 8, [](usize x, usize y, usize z) -> int32 { return (x == 4 && y == 4 && z == 4) ? 2 : 1; }, "M3CSurfaceMeshingFilterTest_Inclusion.dream3d");
}

// Interleaved 3-label tiling (rows "1 2" / "3 1"): squares [1,2,1,3] and [2,1,3,1] have all edges
// differing with exactly one diagonal equal, exercising get_square_index 17 and 18.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Toy Interleaved Diagonal", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  RunM3COnToy(
      8, 8, 8, [](usize x, usize y, usize /*z*/) -> int32 { return (y % 2 == 0) ? ((x % 2 == 0) ? 1 : 2) : ((x % 2 == 0) ? 3 : 1); }, "M3CSurfaceMeshingFilterTest_Interleaved.dream3d");
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
const fs::path k_ExemplarFile = fs::path(nx::core::unit_test::k_TestFilesDir.view()) / "M3CSurfaceMeshingExemplar" / "M3CSurfaceMeshingExemplar.dream3d";
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

// Hidden test: regenerate the exemplar .dream3d from the current output. Run explicitly with the tag
// [.][M3CGenerateExemplar], then upload the file to the DREAM3D Data_Archive.
TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Generate Exemplar", "[.][M3CGenerateExemplar]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  DataStructure dataStructure = LoadSmallIn100Input();
  RunM3COnSmallIn100(dataStructure);

  // Drop the input container so the exemplar holds only the generated mesh.
  dataStructure.removeData(DataPath({k_DataContainer}));
  fs::create_directories(k_ExemplarFile.parent_path());
  WriteTestDataStructure(dataStructure, k_ExemplarFile);
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Exemplar Comparison", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();
  if(!fs::exists(k_ExemplarFile))
  {
    // TODO: once the exemplar is published to the Data_Archive, replace this guard with a
    // TestFileSentinel download. Until then, run the "Generate Exemplar" test to create it locally.
    WARN(fmt::format("Skipping M3C exemplar comparison: exemplar not found at {}", k_ExemplarFile.string()));
    return;
  }

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
