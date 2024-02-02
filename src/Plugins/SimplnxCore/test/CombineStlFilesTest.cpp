#include <catch2/catch.hpp>

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CombineStlFilesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
inline const DataPath k_ComputedTriangleDataContainerName({"ComputedTriangleDataContainer"});
inline const DataPath k_ExemplarTriangleDataContainerName({k_TriangleDataContainerName});
} // namespace

TEST_CASE("SimplnxCore::CombineStlFilesFilter: Valid Filter Execution", "[SimplnxCore][CombineStlFilesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_combine_stl_files_v2.tar.gz",
                                                              "6_6_combine_stl_files.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_combine_stl_files_v2/6_6_combine_stl_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  std::string inputStlDir = fmt::format("{}/6_6_combine_stl_files_v2/STL_Models", unit_test::k_TestFilesDir.view());

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CombineStlFilesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleDataContainerName_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>("Face Normals"));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const TriangleGeom& computedTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ComputedTriangleDataContainerName);
  const TriangleGeom& exemplarTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarTriangleDataContainerName);
  REQUIRE(computedTriangleGeom.getNumberOfFaces() == exemplarTriangleGeom.getNumberOfFaces());
  REQUIRE(computedTriangleGeom.getNumberOfVertices() == exemplarTriangleGeom.getNumberOfVertices());
  REQUIRE(computedTriangleGeom.getFaceAttributeMatrix()->getShape() == exemplarTriangleGeom.getFaceAttributeMatrix()->getShape());

  UnitTest::CompareArrays<float64>(dataStructure, k_ExemplarTriangleDataContainerName.createChildPath(k_FaceData).createChildPath(k_FaceNormals),
                                   k_ComputedTriangleDataContainerName.createChildPath(k_FaceData).createChildPath("Face Normals"));
}

TEST_CASE("SimplnxCore::CombineStlFilesFilter: InValid Filter Execution")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CombineStlFilesFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create invalid Parameters for the filter : no stl files in directory
  std::string inputStlDir = fmt::format("{}/Data/Input", unit_test::k_DREAM3DDataDir.view());

  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleDataContainerName_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>(k_FaceNormals));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
