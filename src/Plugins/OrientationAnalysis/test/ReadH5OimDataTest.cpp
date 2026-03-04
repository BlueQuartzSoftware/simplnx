#include <catch2/catch.hpp>

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ReadH5OimDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include <EbsdLib/IO/TSL/AngFields.h>

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_ScanName_1 = "Scan 1";
const std::string k_ScanName_2 = "Scan 2";
const std::string k_ScanName_3 = "Scan 3";
} // namespace

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Single Scan", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files.tar.gz", "7_read_oem_ebsd_h5_files");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/7_read_oem_ebsd_h5_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5OimDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1}};

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath exemplarImageGeomPath = DataPath({"EDAX_Single_Scan"});

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));

  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == AbstractGeometry::LengthUnit::Micrometer);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Multi Scan", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files.tar.gz", "7_read_oem_ebsd_h5_files");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/7_read_oem_ebsd_h5_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5OimDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1, k_ScanName_2, k_ScanName_3}};

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath exemplarImageGeomPath = DataPath({"EDAX_Multi_Scan"});

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));

  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == AbstractGeometry::LengthUnit::Micrometer);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: InValid Filter Execution", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files.tar.gz", "7_read_oem_ebsd_h5_files");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5OimDataFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1}};

  SECTION("Invalid Z Spacing")
  {
    args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(0.0f));
    args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("No Scan Names Selected")
  {
    scanSelections.scanNames.clear();
    args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("Invalid h5 file type (incompatible manufacturer)")
  {
    h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
    scanSelections.inputFilePath = h5TestFile;
    args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
