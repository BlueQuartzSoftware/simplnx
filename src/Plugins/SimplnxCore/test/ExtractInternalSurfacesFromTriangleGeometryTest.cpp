#include "SimplnxCore/Filters/ExtractInternalSurfacesFromTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr StringLiteral k_TriangleGeomName = "TriangleGeom";
constexpr StringLiteral k_ComputedTriangleGeomName = "Internal Surface";

const DataPath k_TriangleGeomPath({k_TriangleGeomName});
const DataPath k_ComputedTrianglePath({k_ComputedTriangleGeomName});
const DataPath k_ExemplarFaceAttrMat({"Exemplar Internal Surface", "Face Data"});
const DataPath k_ExemplarVertexAttrMat({"Exemplar Internal Surface", "Vertex Data"});
const DataPath k_ComputedFaceAttrMat({k_ComputedTriangleGeomName, "Face Data"});
const DataPath k_ComputedVertexAttrMat({k_ComputedTriangleGeomName, "Vertex Data"});

const std::vector<DataPath> k_VertexArrays = {DataPath::FromString("TriangleGeom/Vertex Data/Node Types").value()};
const std::vector<DataPath> k_TriangleArrays = {DataPath::FromString("TriangleGeom/Face Data/Confidence Index").value(), DataPath::FromString("TriangleGeom/Face Data/EulerAngles").value(),
                                                DataPath::FromString("TriangleGeom/Face Data/FaceLabels").value()

};

} // namespace

TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter(Instantiate)", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "extract_internal_surface.tar.gz", "extract_internal_surface");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/extract_internal_surface/extract_internal_surface.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  ExtractInternalSurfacesFromTriangleGeometryFilter filter;
  Arguments args;

  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_NodeTypesPath_Key, std::make_any<DataPath>(k_VertexArrays[0]));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_VertexArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_TriangleArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>("Vertex Data"));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Face Data"));

  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter(Failed Vertex Copy)", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "extract_internal_surface.tar.gz", "extract_internal_surface");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/extract_internal_surface/extract_internal_surface.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  ExtractInternalSurfacesFromTriangleGeometryFilter filter;
  Arguments args;

  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_NodeTypesPath_Key, std::make_any<DataPath>(k_VertexArrays[0]));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_TriangleArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_TriangleArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>("Vertex Data"));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Face Data"));

  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
}

TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter(Failed Face Copy)", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();
  const TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "extract_internal_surface.tar.gz", "extract_internal_surface");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/extract_internal_surface/extract_internal_surface.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  ExtractInternalSurfacesFromTriangleGeometryFilter filter;
  Arguments args;

  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_NodeTypesPath_Key, std::make_any<DataPath>(k_VertexArrays[0]));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_VertexArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_VertexArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>("Vertex Data"));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Face Data"));

  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
}
TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter(Data)", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter]")
{
  LoadPlugins();
  const TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "extract_internal_surface.tar.gz", "extract_internal_surface");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/extract_internal_surface/extract_internal_surface.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  ExtractInternalSurfacesFromTriangleGeometryFilter filter;
  Arguments args;

  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_NodeTypesPath_Key, std::make_any<DataPath>(k_VertexArrays[0]));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_VertexArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_TriangleArrays));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>("Vertex Data"));
  args.insert(ExtractInternalSurfacesFromTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Face Data"));

  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/extract_internal_surface_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  CompareExemplarToGenerateAttributeMatrix(dataStructure, k_ExemplarFaceAttrMat, dataStructure, k_ComputedFaceAttrMat, true);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, k_ExemplarVertexAttrMat, dataStructure, k_ComputedVertexAttrMat, true);

  CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ExtractInternalSurfacesFromTriangleGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ExtractInternalSurfacesFromTriangleGeometryFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ExtractInternalSurfacesFromTriangleGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ExtractInternalSurfacesFromTriangleGeometryFilter::k_NodeTypesPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (StringToDataPathFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

namespace
{
// Meshes the flush cylinder from SurfaceMeshingTestUtils with QuickSurfaceMesh, leaving Fix
// Problem Voxels off so the mesh is deterministic. omitSkin selects whether QuickSurfaceMesh's own
// Bounding Box Skin option runs, so the same helper can build both the "full" mesh (skin left
// on, to be pruned by this filter) and the "direct" oracle mesh (skin already omitted by the mesher).
const DataPath k_QuickMeshPath({"QuickMesh"});

SurfaceMeshingTest::MeshResult RunQuickSurfaceMeshForExtraction(ChoicesParameter::ValueType boundingBoxSkinMode)
{
  return SurfaceMeshingTest::RunMesher<QuickSurfaceMeshFilter>(SurfaceMeshingTest::CreateCylinderInBox(true), k_QuickMeshPath, boundingBoxSkinMode,
                                                               [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); });
}
} // namespace

TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter: Face Labels criterion", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();

  // Mesh the flush cylinder WITHOUT omitting the skin, then strip it with the Face Labels
  // criterion. The result must match what QuickSurfaceMesh produces with the option ON.
  SurfaceMeshingTest::MeshResult fullMeshResult = RunQuickSurfaceMeshForExtraction(BoundingBoxSkinMode::k_Off);
  DataStructure dataStructure = std::move(fullMeshResult.Structure);
  const DataPath faceLabelsPath = fullMeshResult.FaceLabelsPath;

  const DataPath extractedPath({"Internal"});
  {
    ExtractInternalSurfacesFromTriangleGeometryFilter filter;
    Arguments args;
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(k_QuickMeshPath));
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CriterionMode_Key, std::make_any<ChoicesParameter::ValueType>(1));
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_FaceLabelsPath_Key, std::make_any<DataPath>(faceLabelsPath));
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(extractedPath));
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>("Vertex Data"));
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Face Data"));
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyVertexPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
    // Copy Face Labels along into the extracted geometry too, so the "both code paths agree" claim
    // below can be checked array-for-array rather than just by counting faces and vertices.
    args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyTrianglePaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{faceLabelsPath}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(extractedPath));
  const auto& extractedGeom = dataStructure.getDataRefAs<TriangleGeom>(extractedPath);

  // Face-label stripping does not erode the rim, so the cylinder stays closed.
  REQUIRE(SurfaceMeshingTest::IsWatertight(extractedGeom));

  // Same face and vertex counts as meshing with QuickSurfaceMesh's own option turned on directly:
  // both code paths must agree on what "internal" means.
  SurfaceMeshingTest::MeshResult directMeshResult = RunQuickSurfaceMeshForExtraction(BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
  const auto& directGeom = directMeshResult.Structure.getDataRefAs<TriangleGeom>(k_QuickMeshPath);

  REQUIRE(extractedGeom.getNumberOfFaces() == directGeom.getNumberOfFaces());
  REQUIRE(extractedGeom.getNumberOfVertices() == directGeom.getNumberOfVertices());

  // The two paths must agree exactly, not just on counts: compare the Face Labels array copied
  // into the extracted geometry against the one QuickSurfaceMesh produced directly.
  const DataPath extractedFaceLabelsPath = extractedPath.createChildPath("Face Data").createChildPath(faceLabelsPath.getTargetName());
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(extractedFaceLabelsPath));
  REQUIRE_NOTHROW(directMeshResult.Structure.getDataRefAs<Int32Array>(directMeshResult.FaceLabelsPath));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(extractedFaceLabelsPath), directMeshResult.Structure.getDataRefAs<Int32Array>(directMeshResult.FaceLabelsPath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractInternalSurfacesFromTriangleGeometryFilter: Face Labels criterion requires a valid array", "[SimplnxCore][ExtractInternalSurfacesFromTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();

  SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMeshForExtraction(BoundingBoxSkinMode::k_Off);
  DataStructure dataStructure = std::move(meshResult.Structure);

  ExtractInternalSurfacesFromTriangleGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(k_QuickMeshPath));
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CriterionMode_Key, std::make_any<ChoicesParameter::ValueType>(1));
  // Face Labels path is left at its default (an empty DataPath), which does not exist in the
  // DataStructure. Preflight must reject this with a clear error instead of crashing in execute.
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Internal"})));
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>("Vertex Data"));
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Face Data"));
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyVertexPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(ExtractInternalSurfacesFromTriangleGeometryFilter::k_CopyTrianglePaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}
