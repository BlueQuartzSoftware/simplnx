#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ReadZeissTxmFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

using namespace nx::core;

namespace
{
const DataPath k_CreatedImageGeometryPath({"Computed Geometry"});
const std::string k_CellAttributeMatrixName("Cell Data");
const std::string k_CTDataArrayName("CT Data");

const DataPath k_ExemplarFullVolumePath({"Exemplar Full Volume"});
const DataPath k_ExemplarSubVolumePath({"Exemplar Sub Volume"});

} // namespace

TEST_CASE("SimplnxReview::ReadZeissTxmFileFilter:Read_Full_Volume", "[SimplnxReview][ReadZeissTxmFileFilter]")
{

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ReadZeissTxmFileTest.tar.gz", "ReadZeissTxmFileTest");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadZeissTxmFileFilter filter;
  Arguments args;

  std::filesystem::path filePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.txm", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadZeissTxmFileFilter::k_TxmInputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(filePath));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(k_CreatedImageGeometryPath));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CTDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CTDataArrayName));

  args.insertOrAssign(ReadZeissTxmFileFilter::k_Use_SubVolume_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_SubVolumeStartSlice_Key, std::make_any<UInt32Parameter::ValueType>(1));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_SubVolumeEndSlice_Key, std::make_any<UInt32Parameter::ValueType>(1));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_zeiss_txm_file_test_output.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarFullVolumePath, ::k_CreatedImageGeometryPath);

  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, ::k_ExemplarFullVolumePath.createChildPath("Cell Data"), dataStructure, ::k_CreatedImageGeometryPath.createChildPath("Cell Data"));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxReview::ReadZeissTxmFileFilter:Read_Sub_Volume", "[SimplnxReview][ReadZeissTxmFileFilter]")
{

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ReadZeissTxmFileTest.tar.gz", "ReadZeissTxmFileTest");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadZeissTxmFileFilter filter;
  Arguments args;

  std::filesystem::path filePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.txm", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadZeissTxmFileFilter::k_TxmInputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(filePath));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(k_CreatedImageGeometryPath));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CTDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CTDataArrayName));

  args.insertOrAssign(ReadZeissTxmFileFilter::k_Use_SubVolume_Key, std::make_any<BoolParameter::ValueType>(true));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_SubVolumeStartSlice_Key, std::make_any<UInt32Parameter::ValueType>(50));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_SubVolumeEndSlice_Key, std::make_any<UInt32Parameter::ValueType>(55));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_zeiss_txm_file_test_output.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarSubVolumePath, ::k_CreatedImageGeometryPath);

  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, ::k_ExemplarSubVolumePath.createChildPath("Cell Data"), dataStructure, ::k_CreatedImageGeometryPath.createChildPath("Cell Data"));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
