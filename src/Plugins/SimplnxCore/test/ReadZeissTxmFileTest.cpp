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
  // ************************************************************************************************
  // This section creates all the possible cropping options and then uses GENERATE_COPY to execute the full test case for each cropping option
  UnitTest::Cropping::AxisBoundsChoices bounds;
  bounds.voxelX = {IntVec2Type{10, 30}};
  bounds.voxelY = {IntVec2Type{10, 30}};
  bounds.voxelZ = {IntVec2Type{50, 100}};
  bounds.physX = {FloatVec2Type{10.0f, 100.0f}};
  bounds.physY = {FloatVec2Type{80.0f, 120.0f}};
  bounds.physZ = {FloatVec2Type{150.0f, 200.0f}};
  auto allCropVals = GenerateAllCropValues(bounds);
  auto croppingOptions = GENERATE_COPY(from_range(allCropVals));
  // ************************************************************************************************

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ReadZeissTxmFileTest_v2.tar.gz", "ReadZeissTxmFileTest");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  static std::atomic<int> geomCounter{1};
  const int myId = geomCounter.fetch_add(1);

  const std::string exemplaryGeomName =
      fmt::format("ImageGeometry {:0>3} (CroppingOptions=[{}, {}, {}, {}])", myId, UnitTest::Cropping::CropTypeToString(croppingOptions.type), UnitTest::Cropping::BoolToString(croppingOptions.cropX),
                  UnitTest::Cropping::BoolToString(croppingOptions.cropY), UnitTest::Cropping::BoolToString(croppingOptions.cropZ));

  DYNAMIC_SECTION(exemplaryGeomName)
  {
    ReadZeissTxmFileFilter filter;
    Arguments args;

    std::filesystem::path filePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.txm", unit_test::k_TestFilesDir));

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadZeissTxmFileFilter::k_TxmInputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(filePath));
    args.insertOrAssign(ReadZeissTxmFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(k_CreatedImageGeometryPath));
    args.insertOrAssign(ReadZeissTxmFileFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName));
    args.insertOrAssign(ReadZeissTxmFileFilter::k_CTDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CTDataArrayName));
    args.insertOrAssign(ReadZeissTxmFileFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(croppingOptions));

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

    UnitTest::CompareImageGeometry(dataStructure, DataPath({exemplaryGeomName}), DataPath({k_CreatedImageGeometryPath}));

    auto exemplaryAttrMatrixPath = DataPath({exemplaryGeomName}).createChildPath(Constants::k_Cell_Data);
    auto computedAttrMatrixPath = DataPath({k_CreatedImageGeometryPath}).createChildPath(Constants::k_Cell_Data);
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplaryAttrMatrixPath, dataStructure, computedAttrMatrixPath, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
