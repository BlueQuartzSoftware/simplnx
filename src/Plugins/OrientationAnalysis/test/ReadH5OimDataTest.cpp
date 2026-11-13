#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ReadH5OimDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include <EbsdLib/IO/TSL/AngConstants.h>
#include <EbsdLib/IO/TSL/AngFields.h>

#include <H5Support/H5ScopedSentinel.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_ScanName_1 = "Scan 1";
const std::string k_ScanName_2 = "Scan 2";
const std::string k_ScanName_3 = "Scan 3";

void RequireAcquisitionCoordinates(const DataStructure& dataStructure, usize scanCount)
{
  // EdaxOIMData.h5 Scan 1/EBSD/Header describes a 186-column, 151-row SqrGrid
  // with Step X = Step Y = 1.5. Acquisition positions start at zero, so the
  // maxima are (186 - 1) * 1.5 = 277.5 and (151 - 1) * 1.5 = 225.0.
  constexpr usize k_Columns = 186;
  constexpr usize k_Rows = 151;
  constexpr float32 k_Step = 1.5f;
  constexpr float32 k_MinPosition = 0.0f;
  constexpr float32 k_MaxXPosition = 277.5f;
  constexpr float32 k_MaxYPosition = 225.0f;
  const DataPath geometryPath({ImageGeom::k_TypeName});
  const DataPath cellDataPath = geometryPath.createChildPath(k_Cell_Data);
  const DataPath xPositionPath = cellDataPath.createChildPath(ebsdlib::Ang::XPosition);
  const DataPath yPositionPath = cellDataPath.createChildPath(ebsdlib::Ang::YPosition);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geometryPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(xPositionPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(yPositionPath));
  const auto& geometry = dataStructure.getDataRefAs<ImageGeom>(geometryPath);
  const auto& xPositions = dataStructure.getDataRefAs<Float32Array>(xPositionPath);
  const auto& yPositions = dataStructure.getDataRefAs<Float32Array>(yPositionPath);
  REQUIRE(geometry.getDimensions() == SizeVec3{k_Columns, k_Rows, scanCount});
  const auto spacing = geometry.getSpacing();
  REQUIRE(spacing[0] == k_Step);
  REQUIRE(spacing[1] == k_Step);
  REQUIRE(xPositions.getNumberOfTuples() == k_Columns * k_Rows * scanCount);
  REQUIRE(yPositions.getNumberOfTuples() == xPositions.getNumberOfTuples());
  REQUIRE(xPositions.getNumberOfComponents() == 1);
  REQUIRE(yPositions.getNumberOfComponents() == 1);

  const auto [minX, maxX] = std::minmax_element(xPositions.cbegin(), xPositions.cend());
  const auto [minY, maxY] = std::minmax_element(yPositions.cbegin(), yPositions.cend());
  CHECK(*minX == k_MinPosition);
  CHECK(*maxX == k_MaxXPosition);
  CHECK(*minY == k_MinPosition);
  CHECK(*maxY == k_MaxYPosition);
  for(usize slice = 0; slice < scanCount; slice++)
  {
    for(usize row = 0; row < k_Rows; row++)
    {
      for(usize column = 0; column < k_Columns; column++)
      {
        const usize tupleIndex = (slice * k_Rows + row) * k_Columns + column;
        CAPTURE(slice, row, column);
        REQUIRE(xPositions[tupleIndex] == static_cast<float32>(column) * spacing[0]);
        REQUIRE(yPositions[tupleIndex] == static_cast<float32>(row) * spacing[1]);
      }
    }
  }
}
} // namespace

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Scan Stacking Order", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");
  const auto sourceFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  const auto inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadH5OimData_StackingOrder.h5";
  fs::copy_file(sourceFile, inputFile, fs::copy_options::overwrite_existing);

  constexpr usize k_TuplesPerScan = 186 * 151;
  {
    const hid_t fileId = H5Fopen(inputFile.string().c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    REQUIRE(fileId >= 0);
    H5Support::H5ScopedFileSentinel sentinel(fileId, true);
    // The downloaded scans have identical IQ values. Give scan s the exact
    // integer-valued signal IQ(s, p) = 100000*s + p, where p is the pixel index.
    for(int32 scan = 1; scan <= 3; scan++)
    {
      std::vector<float32> values(k_TuplesPerScan);
      for(usize pixel = 0; pixel < k_TuplesPerScan; pixel++)
      {
        values[pixel] = static_cast<float32>(100000 * scan + pixel);
      }
      const std::string datasetPath = fmt::format("/Scan {}/EBSD/Data/IQ", scan);
      const hid_t datasetId = H5Dopen2(fileId, datasetPath.c_str(), H5P_DEFAULT);
      REQUIRE(datasetId >= 0);
      const herr_t writeResult = H5Dwrite(datasetId, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
      REQUIRE(H5Dclose(datasetId) >= 0);
      REQUIRE(writeResult >= 0);
    }
  }

  OEMEbsdScanSelectionParameter::ValueType scanSelections = {inputFile, RefFrameZDir::k_LowtoHigh, {k_ScanName_2, k_ScanName_1, k_ScanName_3}};
  std::vector<int32> expectedScans;
  // For N selected scans, selection index i occupies Z slice i (Low to High)
  // or N-1-i (High to Low). Thus [2,1,3] becomes [2,1,3] or [3,1,2] in Z.
  SECTION("Low to High")
  {
    expectedScans = {2, 1, 3};
  }
  SECTION("High to Low")
  {
    scanSelections.stackingOrder = RefFrameZDir::k_HightoLow;
    expectedScans = {3, 1, 2};
  }
  SECTION("High to Low single scan")
  {
    scanSelections.stackingOrder = RefFrameZDir::k_HightoLow;
    scanSelections.scanNames = {k_ScanName_2};
    expectedScans = {2};
  }

  DataStructure dataStructure;
  ReadH5OimDataFilter filter;
  Arguments args;
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  const DataPath geometryPath({ImageGeom::k_TypeName});
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  REQUIRE(dataStructure.getDataRefAs<ImageGeom>(geometryPath).getDimensions() == SizeVec3{186, 151, expectedScans.size()});
  const auto& imageQuality = dataStructure.getDataRefAs<Float32Array>(geometryPath.createChildPath(k_Cell_Data).createChildPath(ebsdlib::Ang::ImageQuality));
  REQUIRE(imageQuality.getNumberOfTuples() == k_TuplesPerScan * expectedScans.size());
  for(usize slice = 0; slice < expectedScans.size(); slice++)
  {
    for(usize pixel = 0; pixel < k_TuplesPerScan; pixel++)
    {
      CAPTURE(slice, pixel);
      REQUIRE(imageQuality[slice * k_TuplesPerScan + pixel] == static_cast<float32>(100000 * expectedScans[slice] + pixel));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Acquisition Coordinates", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");
  const auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1}};
  SECTION("Single scan")
  {
    scanSelections.scanNames = {k_ScanName_1};
  }
  SECTION("Three scan slices")
  {
    scanSelections.scanNames = {k_ScanName_1, k_ScanName_2, k_ScanName_3};
  }

  DataStructure dataStructure;
  ReadH5OimDataFilter filter;
  Arguments args;
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  // The fixture has no pattern datasets.
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  RequireAcquisitionCoordinates(dataStructure, scanSelections.scanNames.size());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Single Scan", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");

  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/7_read_oem_ebsd_h5_files_v3.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  ReadH5OimDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1}};

  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath exemplarImageGeomPath = DataPath({"EDAX_Single_Scan"});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName})));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath));
  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  const DataPath generatedImageGeomPath({ImageGeom::k_TypeName});
  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Multi Scan", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");

  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/7_read_oem_ebsd_h5_files_v3.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  ReadH5OimDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1, k_ScanName_2, k_ScanName_3}};

  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath exemplarImageGeomPath = DataPath({"EDAX_Multi_Scan"});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName})));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath));
  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  const DataPath generatedImageGeomPath({ImageGeom::k_TypeName});
  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: InValid Filter Execution", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");

  ReadH5OimDataFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/EdaxOIMData.h5", unit_test::k_TestFilesDir));
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
    h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
    scanSelections.inputFilePath = h5TestFile;
    args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ReadH5OimDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadH5OimDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadH5OimDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadH5OimDataFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(ReadH5OimDataFilter::k_ZSpacing_Key) == 2.5f);
      CHECK(args.value<bool>(ReadH5OimDataFilter::k_ReadPatternData_Key) == true);
      CHECK(args.value<DataPath>(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key) == "TestName");
      const auto scanSel = args.value<OEMEbsdScanSelectionParameter::ValueType>(ReadH5OimDataFilter::k_SelectedScanNames_Key);
      CHECK(scanSel.inputFilePath == "/test/path/file.h5");
      CHECK(scanSel.scanNames == std::list<std::string>{"Scan1"});
    }
  }
}
