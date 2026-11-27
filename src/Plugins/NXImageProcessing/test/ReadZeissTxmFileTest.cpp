#include "simplnx/UnitTest/ReaderGeometryTest.hpp"

#include <catch2/catch.hpp>

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "NXImageProcessing/Filters/ReadZeissTxmFileFilter.hpp"
#include "NXImageProcessing/NXImageProcessing_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::constants;
using namespace nx::core::unit_test;

using namespace nx::core;

namespace
{
const DataPath k_CreatedImageGeometryPath({"Computed Geometry"});
const std::string k_CellAttributeMatrixName("Cell Data");
const std::string k_CTDataArrayName("CT Data");

const DataPath k_ExemplarFullVolumePath({"Exemplar Full Volume"});
const DataPath k_ExemplarSubVolumePath({"Exemplar Sub Volume"});

} // namespace

TEST_CASE("NXImageProcessing::ReadZeissTxmFileFilter:Read_Full_Volume", "[NXImageProcessing][ReadZeissTxmFileFilter]")
{

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ReadZeissTxmFileTest_v2.tar.gz", "ReadZeissTxmFileTest");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadZeissTxmFileFilter filter;
  Arguments args;

  const std::string exemplaryGeomName = "ImageGeometry 001 (CroppingOptions=[NoCropping, False, False, False])";
  auto exemplarFullVolumePath = DataPath({exemplaryGeomName});

  std::filesystem::path filePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.txm", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadZeissTxmFileFilter::k_TxmInputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(filePath));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(k_CreatedImageGeometryPath));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CellAttributeMatrixName));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CTDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CTDataArrayName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_zeiss_txm_file_test_output.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  unit_test::CompareImageGeometry(dataStructure, exemplarFullVolumePath, ::k_CreatedImageGeometryPath);

  unit_test::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFullVolumePath.createChildPath("Cell Data"), dataStructure, ::k_CreatedImageGeometryPath.createChildPath("Cell Data"));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadZeissTxmFileFilter:Read_Sub_Volume", "[NXImageProcessing][ReadZeissTxmFileFilter]")
{
  // ************************************************************************************************
  // This section creates all the possible cropping options and then uses GENERATE_COPY to execute the full test case for each cropping option
  unit_test::cropping::AxisBoundsChoices bounds;
  bounds.voxel_x = {SizeVec2Type{10, 30}};
  bounds.voxel_y = {SizeVec2Type{10, 30}};
  bounds.voxel_z = {SizeVec2Type{50, 100}};
  bounds.phys_x = {FloatVec2Type{10.0f, 100.0f}};
  bounds.phys_y = {FloatVec2Type{80.0f, 120.0f}};
  bounds.phys_z = {FloatVec2Type{150.0f, 200.0f}};
  auto allCropVals = GenerateAllCropValues(bounds);
  auto croppingOptions = GENERATE_COPY(from_range(allCropVals));
  // ************************************************************************************************

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ReadZeissTxmFileTest_v2.tar.gz", "ReadZeissTxmFileTest");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/ReadZeissTxmFileTest/ReadZeissTxmFileTest.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  static std::atomic<int> geomCounter{1};
  const int myId = geomCounter.fetch_add(1);

  const std::string exemplaryGeomName =
      fmt::format("ImageGeometry {:0>3} (CroppingOptions=[{}, {}, {}, {}])", myId, unit_test::cropping::CropTypeToString(croppingOptions.type), unit_test::cropping::BoolToString(croppingOptions.crop_x),
                  unit_test::cropping::BoolToString(croppingOptions.crop_y), unit_test::cropping::BoolToString(croppingOptions.crop_z));

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_zeiss_txm_file_test_output.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    unit_test::CompareImageGeometry(dataStructure, DataPath({exemplaryGeomName}), DataPath({k_CreatedImageGeometryPath}));

    auto exemplaryAttrMatrixPath = DataPath({exemplaryGeomName}).createChildPath(constants::k_CellDataWithSpaces);
    auto computedAttrMatrixPath = DataPath({k_CreatedImageGeometryPath}).createChildPath(constants::k_CellDataWithSpaces);
    unit_test::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplaryAttrMatrixPath, dataStructure, computedAttrMatrixPath, true);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadZeissTxmFileFilter: PhysicalCrop_MaxAtBound", "[NXImageProcessing][ReadZeissTxmFileFilter]")
{
  unit_test::LoadPlugins();
  const DataPath geomPath({"Physical Crop"});
  const TestFileSentinel sentinel(unit_test::k_TestFilesDir, "ReadZeissTxmFileTest_v2.tar.gz", "ReadZeissTxmFileTest");
  const fs::path filePath = fs::path(unit_test::k_TestFilesDir.view()) / "ReadZeissTxmFileTest" / "ReadZeissTxmFileTest.txm";
  Arguments args;
  args.insertOrAssign(ReadZeissTxmFileFilter::k_TxmInputFilePath_Key, filePath);
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CreatedImageGeometryPath_Key, geomPath);
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CellAttributeMatrixName_Key, std::string("Cell Data"));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CTDataArrayName_Key, std::string("ImageData"));
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CroppingOptions_Key, CropGeometryParameter::ValueType{});

  ReadZeissTxmFileFilter filter;
  DataStructure fullDataStructure;
  auto fullPreflight = filter.preflight(fullDataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(fullPreflight.output_actions);
  auto fullActions = fullPreflight.output_actions.value().applyAll(fullDataStructure, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(fullActions);
  REQUIRE_NOTHROW(fullDataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& fullGeom = fullDataStructure.getDataRefAs<ImageGeom>(geomPath);
  const auto origin = fullGeom.getOrigin();
  const auto spacing = fullGeom.getSpacing();
  const auto dims = fullGeom.getDimensions();

  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  crop.crop_x = true;
  crop.crop_y = false;
  crop.crop_z = false;
  crop.x_bound_physical = {origin[0], origin[0] + static_cast<float32>(dims[0]) * spacing[0]};
  CAPTURE(origin[0], spacing[0], dims[0], crop.x_bound_physical[1]);
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CroppingOptions_Key, crop);
  DataStructure croppedDataStructure;
  auto croppedPreflight = filter.preflight(croppedDataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(croppedPreflight.output_actions);
  auto croppedActions = croppedPreflight.output_actions.value().applyAll(croppedDataStructure, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(croppedActions);
  REQUIRE_NOTHROW(croppedDataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& croppedGeom = croppedDataStructure.getDataRefAs<ImageGeom>(geomPath);
  REQUIRE(croppedGeom.getDimensions() == dims);
  REQUIRE(croppedGeom.getOrigin() == origin);
  unit_test::CheckArraysInheritTupleDims(croppedDataStructure);
}

TEST_CASE("NXImageProcessing::ReadZeissTxmFileFilter: replace origin and spacing", "[NXImageProcessing][ReadZeissTxmFileFilter]")
{
  const TestFileSentinel sentinel(unit_test::k_TestFilesDir, "ReadZeissTxmFileTest_v2.tar.gz", "ReadZeissTxmFileTest");
  const fs::path file = fs::path(unit_test::k_TestFilesDir.view()) / "ReadZeissTxmFileTest" / "ReadZeissTxmFileTest.txm";
  const DataPath geometry({"Override Zeiss"});
  Arguments args;
  args.insertOrAssign(ReadZeissTxmFileFilter::k_TxmInputFilePath_Key, file);
  args.insertOrAssign(ReadZeissTxmFileFilter::k_CreatedImageGeometryPath_Key, geometry);
  unit_test::SetReaderGeometryOverrides(args, {4, -3, 8}, {2, 3, 4});
  unit_test::RequireReaderGeometryExecution(ReadZeissTxmFileFilter{}, args, geometry, {4, -3, 8}, {2, 3, 4});
}
