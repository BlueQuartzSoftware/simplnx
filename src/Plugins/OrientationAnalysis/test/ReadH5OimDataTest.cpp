#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5OimData.hpp"
#include "OrientationAnalysis/Filters/ReadH5OimDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include <EbsdLib/IO/TSL/AngConstants.h>
#include <EbsdLib/IO/TSL/AngFields.h>
#include <EbsdLib/IO/TSL/H5OIMReader.h>
#include <H5Support/H5Lite.h>
#include <H5Support/H5ScopedSentinel.h>
#include <H5Support/H5Utilities.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_ScanName_1 = "Scan 1";
const std::string k_ScanName_2 = "Scan 2";
const std::string k_ScanName_3 = "Scan 3";

void WriteOimScans(const fs::path& h5TestFile, int32 secondColumns, int32 secondRows)
{
  const hid_t fileId = H5Support::H5Utilities::createFile(h5TestFile.string());
  REQUIRE(fileId >= 0);
  H5Support::H5ScopedFileSentinel fileSentinel(fileId, false);
  REQUIRE(H5Support::H5Lite::writeStringDataset(fileId, ebsdlib::H5OIM::Manufacturer, ebsdlib::H5OIM::EDAX) >= 0);
  REQUIRE(H5Support::H5Lite::writeStringDataset(fileId, ebsdlib::H5OIM::Version8, ebsdlib::H5OIM::OIMAnalysisVersion8) >= 0);

  const auto createGroup = [&fileSentinel](hid_t parentId, const std::string& name) {
    const hid_t groupId = H5Support::H5Utilities::createGroup(parentId, name);
    REQUIRE(groupId >= 0);
    fileSentinel.addGroupId(groupId);
    return groupId;
  };
  const auto writeScan = [&](const std::string& scanName, int32 columns, int32 rows) {
    const hid_t scanId = createGroup(fileId, scanName);
    const hid_t ebsdId = createGroup(scanId, ebsdlib::H5OIM::EBSD);
    const hid_t headerId = createGroup(ebsdId, ebsdlib::H5OIM::Header);
    const hid_t patternCenterId = createGroup(headerId, ebsdlib::H5OIM::PatternCenterCalibration);
    for(const auto& name : {ebsdlib::Ang::XStar, ebsdlib::Ang::YStar, ebsdlib::Ang::ZStar})
    {
      REQUIRE(H5Support::H5Lite::writeScalarDataset<float32>(patternCenterId, name, 0.5f) >= 0);
    }
    REQUIRE(H5Support::H5Lite::writeScalarDataset<float32>(headerId, ebsdlib::Ang::Working_Distance, 10.0f) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<float32>(headerId, ebsdlib::Ang::StepX, 1.0f) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<float32>(headerId, ebsdlib::Ang::StepY, 1.0f) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(headerId, ebsdlib::Ang::nColumns, columns) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(headerId, ebsdlib::Ang::nRows, rows) >= 0);
    REQUIRE(H5Support::H5Lite::writeStringDataset(headerId, ebsdlib::Ang::GridType, ebsdlib::Ang::SquareGrid) >= 0);

    const hid_t phasesId = createGroup(headerId, ebsdlib::H5OIM::Phase);
    const hid_t phaseId = createGroup(phasesId, "1");
    REQUIRE(H5Support::H5Lite::writeStringDataset(phaseId, ebsdlib::Ang::MaterialName, "Nickel") >= 0);
    REQUIRE(H5Support::H5Lite::writeStringDataset(phaseId, ebsdlib::Ang::Formula, "Ni") >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(phaseId, ebsdlib::Ang::Symmetry, 43) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(phaseId, ebsdlib::Ang::NumberFamilies, 0) >= 0);
    for(const auto& name : {ebsdlib::Ang::LatticeConstantA, ebsdlib::Ang::LatticeConstantB, ebsdlib::Ang::LatticeConstantC})
    {
      REQUIRE(H5Support::H5Lite::writeScalarDataset<float32>(phaseId, name, 3.52f) >= 0);
    }
    for(const auto& name : {ebsdlib::Ang::LatticeConstantAlpha, ebsdlib::Ang::LatticeConstantBeta, ebsdlib::Ang::LatticeConstantGamma})
    {
      REQUIRE(H5Support::H5Lite::writeScalarDataset<float32>(phaseId, name, 90.0f) >= 0);
    }

    const hid_t dataId = createGroup(ebsdId, ebsdlib::H5OIM::Data);
    const usize count = static_cast<usize>(columns) * static_cast<usize>(rows);
    const std::vector<hsize_t> dims = {static_cast<hsize_t>(count)};
    for(const auto& name : {ebsdlib::Ang::Phi1, ebsdlib::Ang::Phi, ebsdlib::Ang::Phi2, ebsdlib::Ang::IQ, ebsdlib::Ang::CI, ebsdlib::Ang::Fit, ebsdlib::Ang::SEMSignal})
    {
      REQUIRE(H5Support::H5Lite::writeVectorDataset<float32>(dataId, name, dims, std::vector<float32>(count, 0.5f)) >= 0);
    }
    REQUIRE(H5Support::H5Lite::writeVectorDataset<int32>(dataId, ebsdlib::Ang::Phase, dims, std::vector<int32>(count, 1)) >= 0);
    std::vector<float32> xPositions(count);
    std::vector<float32> yPositions(count);
    for(usize index = 0; index < count; ++index)
    {
      xPositions[index] = static_cast<float32>(index % static_cast<usize>(columns));
      yPositions[index] = static_cast<float32>(index / static_cast<usize>(columns));
    }
    REQUIRE(H5Support::H5Lite::writeVectorDataset<float32>(dataId, ebsdlib::Ang::XPosition, dims, xPositions) >= 0);
    REQUIRE(H5Support::H5Lite::writeVectorDataset<float32>(dataId, ebsdlib::Ang::YPosition, dims, yPositions) >= 0);
  };

  writeScan(k_ScanName_1, 4, 3);
  writeScan(k_ScanName_2, secondColumns, secondRows);
}

Arguments OimScanArguments(const fs::path& h5TestFile)
{
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1, k_ScanName_2}};
  args.insertOrAssign(ReadH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5OimDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  return args;
}

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

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Scans With Different Dimensions", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path h5TestFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadH5OimData_Dimensions.h5";
  fs::create_directories(h5TestFile.parent_path());
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(h5TestFile, ec);
  });
  const auto [columns, rows] = GENERATE(std::pair<int32, int32>{2, 2}, std::pair<int32, int32>{3, 4}, std::pair<int32, int32>{4, 2}, std::pair<int32, int32>{2, 3}, std::pair<int32, int32>{4, 3});
  CAPTURE(columns, rows);
  WriteOimScans(h5TestFile, columns, rows);
  for(const auto& scan : {k_ScanName_1, k_ScanName_2})
  {
    auto reader = ebsdlib::H5OIMReader::New();
    reader->setFileName(h5TestFile.string());
    reader->setHDF5Path(scan);
    reader->setReadPatternData(false);
    const auto readResult = reader->readFile();
    INFO(reader->getErrorMessage());
    REQUIRE(readResult >= 0);
    REQUIRE(reader->getXDimension() == (scan == k_ScanName_1 ? 4 : columns));
    REQUIRE(reader->getYDimension() == (scan == k_ScanName_1 ? 3 : rows));
  }
  DataStructure dataStructure;
  ReadH5OimDataFilter filter;
  auto result = filter.preflight(dataStructure, OimScanArguments(h5TestFile));
  if(columns == 4 && rows == 3)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -9592);
    const auto& message = result.outputActions.errors().front().message;
    REQUIRE(message.find("Scan '" + k_ScanName_2 + "' in file '" + h5TestFile.string() + "' has " + std::to_string(columns) + " columns and " + std::to_string(rows) + " rows") != std::string::npos);
    REQUIRE(message.find("first selected scan '" + k_ScanName_1 + "' has 4 columns and 3 rows") != std::string::npos);
  }
}

TEST_CASE("OrientationAnalysis::ReadH5OimDataFilter: Dimensions Changed After Preflight", "[OrientationAnalysis][ReadH5OimDataFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path h5TestFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadH5OimData_ChangedDimensions.h5";
  fs::create_directories(h5TestFile.parent_path());
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(h5TestFile, ec);
  });
  WriteOimScans(h5TestFile, 4, 3);
  DataStructure dataStructure;
  ReadH5OimDataFilter filter;
  auto result = filter.preflight(dataStructure, OimScanArguments(h5TestFile));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  auto applyResult = result.outputActions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  // Each source has at least 12 points, so a missing guard cannot over-read it.
  const auto [columns, rows] = GENERATE(std::pair<int32, int32>{3, 4}, std::pair<int32, int32>{4, 4}, std::pair<int32, int32>{5, 3});
  CAPTURE(columns, rows);
  WriteOimScans(h5TestFile, columns, rows);
  ReadH5DataInputValues inputValues{};
  inputValues.SelectedScanNames = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1, k_ScanName_2}};
  inputValues.ImageGeometryPath = DataPath({ImageGeom::k_TypeName});
  inputValues.CellAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(k_Cell_Data);
  inputValues.CellEnsembleAttributeMatrixPath = inputValues.ImageGeometryPath.createChildPath(k_Cell_Ensemble_Data);
  const IFilter::MessageHandler handler;
  const std::atomic_bool cancel = false;
  auto executeResult = ReadH5OimData(dataStructure, handler, cancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult);
  REQUIRE(executeResult.errors().front().code == -8972);
  const auto& message = executeResult.errors().front().message;
  REQUIRE(message.find("Scan '" + k_ScanName_2 + "' in file '" + h5TestFile.string() + "' has " + std::to_string(columns) + " columns and " + std::to_string(rows) + " rows") != std::string::npos);
  REQUIRE(message.find("Image Geometry '" + inputValues.ImageGeometryPath.toString() + "' has 4 columns and 3 rows") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
