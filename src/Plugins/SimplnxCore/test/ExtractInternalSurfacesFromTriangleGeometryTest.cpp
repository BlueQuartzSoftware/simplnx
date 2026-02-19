#include "SimplnxCore/Filters/ExtractInternalSurfacesFromTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

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
