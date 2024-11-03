#include "OrientationAnalysis/Filters/ReadChannel5DataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("OrientationAnalysis::ReadChannel5Data:Native_Data", "[OrientationAnalysis][ReadChannel5Data]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_ReadChannel5_Test.tar.gz", "7_ReadChannel5_Test");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_ReadChannel5_Test/7_ReadChannel5_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadChannel5DataFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const fs::path inputCtfFile(fmt::format("{}/7_ReadChannel5_Test/17NZ42_Dauphinetwinnedsample_ plaglens.cpr", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadChannel5DataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputCtfFile));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare Cell Data
  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-No-Options", "Cell Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath(k_Cell_Data);
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }
  // Compare Ensemble Data
  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-No-Options", "Cell Ensemble Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath("Cell Ensemble Data");
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }
  // Compare Geometries
  {
    auto* exemplarPtr = exemplarDataStructure.getDataAs<ImageGeom>(DataPath({"Exemplar-All-Options"}));
    auto* computedPtr = dataStructure.getDataAs<ImageGeom>(k_DataContainerPath);

    CompareImageGeometry(exemplarPtr, computedPtr);
  }
}

TEST_CASE("OrientationAnalysis::ReadChannel5Data:SIMPLNX_Data", "[OrientationAnalysis][ReadChannel5Data]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_ReadChannel5_Test.tar.gz", "7_ReadChannel5_Test");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_ReadChannel5_Test/7_ReadChannel5_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadChannel5DataFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const fs::path inputCtfFile(fmt::format("{}/7_ReadChannel5_Test/17NZ42_Dauphinetwinnedsample_ plaglens.cpr", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadChannel5DataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputCtfFile));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare Cell Data
  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-All-Options", "Cell Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath(k_Cell_Data);
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }
  // Compare Ensemble Data
  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-All-Options", "Cell Ensemble Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath("Cell Ensemble Data");
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }

  // Compare Geometries
  {
    auto* exemplarPtr = exemplarDataStructure.getDataAs<ImageGeom>(DataPath({"Exemplar-All-Options"}));
    auto* computedPtr = dataStructure.getDataAs<ImageGeom>(k_DataContainerPath);

    CompareImageGeometry(exemplarPtr, computedPtr);
  }
}
