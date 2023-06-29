#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CombineStlFilesFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
inline const DataPath k_ComputedTriangleDataContainerName({"ComputedTriangleDataContainer"});
inline const DataPath k_ExemplarTriangleDataContainerName({k_TriangleDataContainerName});
} // namespace

TEST_CASE("ComplexCore::CombineStlFilesFilter: Valid Filter Execution", "[ComplexCore][CombineStlFilesFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_combine_stl_files.tar.gz", "6_6_combine_stl_files.dream3d",
                                                             complex::unit_test::k_BinaryTestOutputDir);

  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "STL_Models.tar.gz", "STL_Models",
                                                              complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_combine_stl_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  std::string inputStlDir = fmt::format("{}/STL_Models", unit_test::k_TestFilesDir.view());

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CombineStlFilesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleDataContainerName_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>(k_FaceNormals));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  const TriangleGeom& computedTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ComputedTriangleDataContainerName);
  const TriangleGeom& exemplarTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarTriangleDataContainerName);
  REQUIRE(computedTriangleGeom.getNumberOfFaces() == exemplarTriangleGeom.getNumberOfFaces());
  REQUIRE(computedTriangleGeom.getNumberOfVertices() == exemplarTriangleGeom.getNumberOfVertices());
  REQUIRE(computedTriangleGeom.getFaceAttributeMatrix()->getShape() == exemplarTriangleGeom.getFaceAttributeMatrix()->getShape());

  UnitTest::CompareArrays<float64>(dataStructure, k_ExemplarTriangleDataContainerName.createChildPath(k_FaceData).createChildPath(k_FaceNormals),
                                   k_ComputedTriangleDataContainerName.createChildPath(k_FaceData).createChildPath(k_FaceNormals));
}

TEST_CASE("ComplexCore::CombineStlFilesFilter: InValid Filter Execution")
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
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
