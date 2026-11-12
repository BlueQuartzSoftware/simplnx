#include "OrientationAnalysis/Filters/Algorithms/ReadH5OimData.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include <H5Support/H5Lite.h>
#include <H5Support/H5ScopedSentinel.h>
#include <H5Support/H5Utilities.h>
#include <catch2/catch.hpp>
#include <limits>

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

TEST_CASE("OrientationAnalysis::ReadH5OimData: HeaderValidation", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  const std::string field = GENERATE("columns", "rows", "xstep", "ystep", "width", "height", "symmetry");
  const auto variant = GENERATE(0, 1, 2, 3);
  const bool laterScan = GENERATE(false, true);
  CAPTURE(field, variant, laterScan);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "header_validation_Oim.h5";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  {
    namespace H5Lite = H5Support::H5Lite;
    const hid_t id = H5Support::H5Utilities::createFile(path.string());
    REQUIRE(id >= 0);
    H5Support::H5ScopedFileSentinel sentinel(id, true);
    for(const std::string scanName : {"Scan", "Scan2"})
    {
      const bool invalidHeader = scanName == (laterScan ? "Scan2" : "Scan");
      const std::string header = scanName + "/EBSD/Header/";
      const auto intValue = [&](const std::string& name, const std::string& target, int32 normal) {
        REQUIRE(H5Lite::writeScalarDataset<int32>(id, header + name, invalidHeader && field == target ? (variant % 2 == 0 ? 0 : -1) : normal) >= 0);
      };
      const auto stepValue = [&](const std::string& name, const std::string& target) {
        const std::array<double, 4> values = {-1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), 0.0};
        REQUIRE(H5Lite::writeScalarDataset<float32>(id, header + name, invalidHeader && field == target ? values[variant] : 0.5) >= 0);
      };

      REQUIRE(H5Support::H5Utilities::createGroupsFromPath(header + "Pattern Center Calibration", id) >= 0);
      REQUIRE(H5Support::H5Utilities::createGroupsFromPath(header + "Phase/1", id) >= 0);
      for(const auto& key : {ebsdlib::Ang::XStar, ebsdlib::Ang::YStar, ebsdlib::Ang::ZStar})
      {
        REQUIRE(H5Lite::writeScalarDataset<float32>(id, header + "Pattern Center Calibration/" + key, 0.5F) >= 0);
      }
      REQUIRE(H5Lite::writeScalarDataset<float32>(id, header + ebsdlib::Ang::Working_Distance, 15.0F) >= 0);
      intValue(ebsdlib::Ang::nColumns, "columns", 1);
      intValue(ebsdlib::Ang::nRows, "rows", 1);
      intValue(ebsdlib::Ang::PatternWidth, "width", 1);
      intValue(ebsdlib::Ang::PatternHeight, "height", 1);
      stepValue(ebsdlib::Ang::StepX, "xstep");
      stepValue(ebsdlib::Ang::StepY, "ystep");
      REQUIRE(H5Lite::writeStringDataset(id, header + ebsdlib::Ang::GridType, "SqrGrid") >= 0);
      const std::string phase = header + "Phase/1/";
      for(const auto& key : {ebsdlib::Ang::MaterialName, ebsdlib::Ang::Formula})
      {
        REQUIRE(H5Lite::writeStringDataset(id, phase + key, "Nickel") >= 0);
      }
      REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::Ang::Symmetry, invalidHeader && field == "symmetry" ? 999 : 43) >= 0);
      REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::Ang::NumberFamilies, 0) >= 0);
      for(const auto& key : {ebsdlib::Ang::LatticeConstantA, ebsdlib::Ang::LatticeConstantB, ebsdlib::Ang::LatticeConstantC, ebsdlib::Ang::LatticeConstantAlpha, ebsdlib::Ang::LatticeConstantBeta,
                             ebsdlib::Ang::LatticeConstantGamma})
      {
        REQUIRE(H5Lite::writeScalarDataset<float32>(id, phase + key, 1.0F) >= 0);
      }
    }
  }
  DataStructure ds;
  ReadH5OimDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType selection = {path, ebsdlib::RefFrameZDir::LowtoHigh, {"Scan", "Scan2"}};
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(field == "width" || field == "height"));
  const auto argsForValidation = args;
  auto result = filter.preflight(ds, argsForValidation).outputActions;

  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  const auto executeResult = filter.execute(ds, argsForValidation);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == result.errors()[0].code);
  ReadH5DataInputValues directValues{};
  directValues.SelectedScanNames = selection;
  directValues.ReadPatternData = field == "width" || field == "height";
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  ReadH5OimData algorithm(ds, handler, cancel, &directValues);
  auto direct = algorithm.readData(laterScan ? "Scan2" : "Scan");
  SIMPLNX_RESULT_REQUIRE_INVALID(direct);
  REQUIRE(direct.errors().front().code == result.errors().front().code);
  // Zero patterns already have an existing error. Negative patterns need positive-domain validation.
  if((field == "width" || field == "height") && variant % 2 == 0)
  {
    return;
  }
  REQUIRE(result.errors()[0].code == (field == "symmetry" ? -67600 : field == "columns" || field == "rows" || field == "width" || field == "height" ? -67601 : -67602));
}

namespace
{
void WriteStructuralOimFixture(const fs::path& path, const std::string& field)
{
  namespace H5Lite = H5Support::H5Lite;
  const hid_t id = H5Support::H5Utilities::createFile(path.string());
  REQUIRE(id >= 0);
  H5Support::H5ScopedFileSentinel sentinel(id, true);
  for(const std::string scanName : {"Scan", "Scan2"})
  {
    const std::string header = scanName + "/EBSD/Header/";
    const auto intValue = [&](const std::string& name, const std::string& target, int32 normal) {
      REQUIRE(H5Lite::writeScalarDataset<int32>(id, header + name, scanName == "Scan" && field == target ? 2 : normal) >= 0);
    };
    const auto stepValue = [&](const std::string& name, const std::string& target) {
      REQUIRE(H5Lite::writeScalarDataset<float32>(id, header + name, scanName == "Scan2" && field == target ? 0.75 : 0.5) >= 0);
    };

    REQUIRE(H5Support::H5Utilities::createGroupsFromPath(header + "Pattern Center Calibration", id) >= 0);
    REQUIRE(H5Support::H5Utilities::createGroupsFromPath(header + "Phase/1", id) >= 0);
    for(const auto& key : {ebsdlib::Ang::XStar, ebsdlib::Ang::YStar, ebsdlib::Ang::ZStar})
    {
      REQUIRE(H5Lite::writeScalarDataset<float32>(id, header + "Pattern Center Calibration/" + key, 0.5F) >= 0);
    }
    REQUIRE(H5Lite::writeScalarDataset<float32>(id, header + ebsdlib::Ang::Working_Distance, 15.0F) >= 0);
    intValue(ebsdlib::Ang::nColumns, "columns", field == "short" ? 2 : 1);
    intValue(ebsdlib::Ang::nRows, "rows", 1);
    intValue(ebsdlib::Ang::PatternWidth, "width", 1);
    intValue(ebsdlib::Ang::PatternHeight, "height", 1);
    stepValue(ebsdlib::Ang::StepX, "xstep");
    stepValue(ebsdlib::Ang::StepY, "ystep");
    REQUIRE(H5Lite::writeStringDataset(id, header + ebsdlib::Ang::GridType, "SqrGrid") >= 0);
    const std::string phase = header + "Phase/1/";
    for(const auto& key : {ebsdlib::Ang::MaterialName, ebsdlib::Ang::Formula})
    {
      REQUIRE(H5Lite::writeStringDataset(id, phase + key, "Nickel") >= 0);
    }
    REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::Ang::Symmetry, 43) >= 0);
    REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::Ang::NumberFamilies, 0) >= 0);
    for(const auto& key : {ebsdlib::Ang::LatticeConstantA, ebsdlib::Ang::LatticeConstantB, ebsdlib::Ang::LatticeConstantC, ebsdlib::Ang::LatticeConstantAlpha, ebsdlib::Ang::LatticeConstantBeta,
                           ebsdlib::Ang::LatticeConstantGamma})
    {
      REQUIRE(H5Lite::writeScalarDataset<float32>(id, phase + key, 1.0F) >= 0);
    }

    if(scanName == "Scan2" && field == "phases")
    {
      REQUIRE(H5Ocopy(id, (header + "Phase/1").c_str(), id, (header + "Phase/2").c_str(), H5P_DEFAULT, H5P_DEFAULT) >= 0);
    }
    if(scanName == "Scan2" && field == "material")
    {
      REQUIRE(H5Ldelete(id, (phase + ebsdlib::Ang::MaterialName).c_str(), H5P_DEFAULT) >= 0);
      REQUIRE(H5Lite::writeStringDataset(id, phase + ebsdlib::Ang::MaterialName, "Iron") >= 0);
    }
    REQUIRE(H5Support::H5Utilities::createGroupsFromPath(scanName + "/EBSD/Data", id) >= 0);
    if(field == "data")
    {
      for(const auto& name :
          {ebsdlib::Ang::Phi1, ebsdlib::Ang::Phi, ebsdlib::Ang::Phi2, ebsdlib::Ang::IQ, ebsdlib::Ang::CI, ebsdlib::Ang::XPosition, ebsdlib::Ang::YPosition, ebsdlib::Ang::Fit, ebsdlib::Ang::SEMSignal})
      {
        REQUIRE(H5Lite::writeVectorDataset<float32>(id, scanName + "/EBSD/Data/" + name, {1}, {0.25F}) >= 0);
      }
      for(const auto& name : {ebsdlib::Ang::Phase})
      {
        REQUIRE(H5Lite::writeVectorDataset<int32>(id, scanName + "/EBSD/Data/" + name, {1}, {name == ebsdlib::Ang::Phase ? 1 : 0}) >= 0);
      }
    }
    const std::string dataset = scanName + "/EBSD/Data/" + ebsdlib::Ang::PatternData;
    if(!(scanName == "Scan2" && field == "missing"))
    {
      if(field == "type")
      {
        REQUIRE(H5Lite::writeVectorDataset<float32>(id, dataset, {1, 1, 1}, {0.0F}) >= 0);
      }
      else if(field == "components")
      {
        REQUIRE(H5Lite::writeVectorDataset<uint8>(id, dataset, {1, 1, 2}, {0, 1}) >= 0);
      }
      else if(field == "rank")
      {
        REQUIRE(H5Lite::writeVectorDataset<uint8>(id, dataset, {1, 1}, {0}) >= 0);
      }
      else
      {
        REQUIRE(H5Lite::writeVectorDataset<uint8>(id, dataset, {1, 1, 1}, {static_cast<uint8>(scanName == "Scan" ? 17 : 29)}) >= 0);
      }
    }
  }
}
} // namespace

TEST_CASE("OrientationAnalysis::ReadH5OimData: StructuralValidation", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  const std::string field = GENERATE("columns", "rows", "xstep", "ystep", "phases", "material", "width", "height", "short", "type", "rank", "components", "missing");
  CAPTURE(field);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "structural_Oim.h5";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteStructuralOimFixture(path, field);
  DataStructure ds;
  ReadH5OimDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType selection = {path, ebsdlib::RefFrameZDir::LowtoHigh, {"Scan", "Scan2"}};
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key,
                      std::make_any<bool>(field == "width" || field == "height" || field == "short" || field == "type" || field == "rank" || field == "components" || field == "missing"));
  auto result = filter.preflight(ds, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  const int32 expected = field == "columns" || field == "rows" ? -67606 :
                         field == "xstep" || field == "ystep"  ? -67607 :
                         field == "phases"                     ? -67608 :
                         field == "material"                   ? -67619 :
                         field == "width" || field == "height" ? -67620 :
                         field == "missing"                    ? -67609 :
                         field == "type"                       ? -67610 :
                         field == "rank"                       ? -67611 :
                                                                 -67612;
  REQUIRE(result.errors().front().code == expected);
  REQUIRE(result.errors().front().message.find(path.string()) != std::string::npos);
  WriteStructuralOimFixture(path, "valid");
  auto valid = filter.preflight(ds, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(valid);
  auto applied = valid.value().applyRegular(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
  WriteStructuralOimFixture(path, field);
  ReadH5DataInputValues values{};
  values.SelectedScanNames = selection;
  values.ReadPatternData = field == "width" || field == "height" || field == "short" || field == "type" || field == "rank" || field == "components" || field == "missing";
  const auto defaults = filter.getDefaultArguments();
  values.ImageGeometryPath = defaults.value<DataPath>(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key);
  values.CellAttributeMatrixPath = values.ImageGeometryPath.createChildPath(defaults.value<std::string>(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key));
  values.CellEnsembleAttributeMatrixPath = values.ImageGeometryPath.createChildPath(defaults.value<std::string>(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key));
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler messageHandler;
  ReadH5OimData algorithm(ds, messageHandler, cancel, &values);
  auto direct = algorithm.readData(field == "short" || field == "type" || field == "rank" || field == "components" ? "Scan" : "Scan2");
  SIMPLNX_RESULT_REQUIRE_INVALID(direct);
  REQUIRE(direct.errors().front().code == expected);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ReadH5OimData: StructuralValidation valid patterns and stale destination", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  const int stale = GENERATE(0, 1, 2, 3);
  CAPTURE(stale);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "structural_Oim_patterns.h5";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteStructuralOimFixture(path, "data");
  ReadH5OimDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType selection = {path, ebsdlib::RefFrameZDir::LowtoHigh, {"Scan", "Scan2"}};
  const DataPath geomPath({"Structural"});
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cells"));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>("Phases"));
  DataStructure ds;
  if(stale)
  {
    auto preflight = filter.preflight(ds, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(preflight);
    auto applied = preflight.value().applyRegular(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(applied);
    ReadH5DataInputValues values{};
    values.SelectedScanNames = selection;
    if(stale == 1)
      values.SelectedScanNames.scanNames.push_back("Scan");
    values.ReadPatternData = true;
    values.ImageGeometryPath = geomPath;
    values.CellAttributeMatrixPath = geomPath.createChildPath("Cells");
    values.CellEnsembleAttributeMatrixPath = geomPath.createChildPath("Phases");
    if(stale == 2)
      values.CellAttributeMatrixPath = geomPath.createChildPath("Missing Cells");
    if(stale == 3)
    {
      const hid_t id = H5Support::H5Utilities::openFile(path.string(), false);
      REQUIRE(id >= 0);
      H5Support::H5ScopedFileSentinel sentinel(id, true);
      for(const std::string scan : {"Scan", "Scan2"})
      {
        REQUIRE(H5Lmove(id, (scan + "/EBSD/Header/Phase/1").c_str(), id, (scan + "/EBSD/Header/Phase/2").c_str(), H5P_DEFAULT, H5P_DEFAULT) >= 0);
      }
    }
    const std::atomic_bool cancel = false;
    auto result = ReadH5OimData(ds, {}, cancel, &values)();
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == (stale == 1 ? -67616 : stale == 2 ? -67617 : -67618));
  }
  else
  {
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto dataPath = geomPath.createChildPath("Cells").createChildPath(ebsdlib::Ang::PatternData);
    REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(dataPath));
    const auto& patterns = ds.getDataRefAs<UInt8Array>(dataPath);
    REQUIRE(patterns.getSize() == 2);
    REQUIRE(patterns[0] == 17);
    REQUIRE(patterns[1] == 29);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}
