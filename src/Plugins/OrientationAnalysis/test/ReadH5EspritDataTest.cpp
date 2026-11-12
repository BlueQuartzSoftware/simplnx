#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ReadH5EspritDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include <EbsdLib/IO/BrukerNano/EspritConstants.h>
#include <EbsdLib/IO/BrukerNano/H5EspritReader.h>
#include <EbsdLib/IO/TSL/AngFields.h>
#include <H5Support/H5Lite.h>
#include <H5Support/H5ScopedSentinel.h>
#include <H5Support/H5Utilities.h>

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
const std::string k_ScanName_435 = "Section_435";
const std::string k_ScanName_436 = "Section_436";
const std::string k_ScanName_437 = "Section_437";

void WriteEspritScans(const fs::path& path, int32 columns, int32 rows)
{
  using namespace ebsdlib::H5Esprit;
  const hid_t fileId = H5Support::H5Utilities::createFile(path.string());
  REQUIRE(fileId >= 0);
  H5Support::H5ScopedFileSentinel sentinel(fileId, false);
  REQUIRE(H5Support::H5Lite::writeStringDataset(fileId, Manufacturer, BrukerNano) >= 0);
  REQUIRE(H5Support::H5Lite::writeStringDataset(fileId, Version, "Esprit 2.X") >= 0);
  const auto group = [&](hid_t parent, const std::string& name) {
    const hid_t id = H5Support::H5Utilities::createGroup(parent, name);
    REQUIRE(id >= 0);
    sentinel.addGroupId(id);
    return id;
  };
  const auto scan = [&](const std::string& name, int32 x, int32 y) {
    const hid_t ebsd = group(group(fileId, name), EBSD);
    const hid_t header = group(ebsd, Header);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(header, NCOLS, x) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(header, NROWS, y) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(header, NPoints, x * y) >= 0);
    for(const auto& key : {PatternWidth, PatternHeight})
    {
      REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(header, key, 0) >= 0);
    }
    for(const auto& key : {XSTEP, YSTEP, SEPixelSizeX, SEPixelSizeY, ZOffset, CameraTilt, KV, MADMax, Magnification, MapStepFactor, SampleTilt, TopClip, WD})
    {
      REQUIRE(H5Support::H5Lite::writeScalarDataset<float64>(header, key, 1.0) >= 0);
    }
    REQUIRE(H5Support::H5Lite::writeStringDataset(header, GridType, Isometric) >= 0);
    REQUIRE(H5Support::H5Lite::writeStringDataset(header, OriginalFile, "inline.bcf") >= 0);
    const hid_t phase = group(group(header, Phases), "1");
    REQUIRE(H5Support::H5Lite::writeStringDataset(phase, Formula, "Ni") >= 0);
    REQUIRE(H5Support::H5Lite::writeStringDataset(phase, Name, "Nickel") >= 0);
    REQUIRE(H5Support::H5Lite::writeStringDataset(phase, SpaceGroup, "F m#ovl3m") >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(phase, IT, 225) >= 0);
    REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(phase, Setting, 1) >= 0);
    REQUIRE(H5Support::H5Lite::writeVectorDataset<float32>(phase, LatticeConstants, {6}, {3.52f, 3.52f, 3.52f, 90.0f, 90.0f, 90.0f}) >= 0);
    group(phase, "AtomPositions");
  };
  scan(k_ScanName_1, 4, 3);
  scan(k_ScanName_2, columns, rows);
}

} // namespace

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: Single Scan", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/7_read_oem_ebsd_h5_files_v3.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5EspritDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_435}};

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5EspritDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5EspritDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath exemplarImageGeomPath = DataPath({"Bruker_Single_Scan"});
  const DataPath generatedImageGeomPath({ImageGeom::k_TypeName});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(generatedImageGeomPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(generatedImageGeomPath);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath));
  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  // Independent oracle (simplnx #1804): every generated cell must hold the measurement whose beam coordinates name that cell.
  {
    const DataPath xBeamPath = generatedImageGeomPath.createChildPath(k_Cell_Data).createChildPath(ebsdlib::H5Esprit::XBEAM);
    const DataPath yBeamPath = generatedImageGeomPath.createChildPath(k_Cell_Data).createChildPath(ebsdlib::H5Esprit::YBEAM);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(xBeamPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(yBeamPath));
    const auto& xBeam = dataStructure.getDataRefAs<Int32Array>(xBeamPath);
    const auto& yBeam = dataStructure.getDataRefAs<Int32Array>(yBeamPath);
    REQUIRE(xBeam.getNumberOfTuples() == imageGeom.getNumberOfCells());
    REQUIRE(yBeam.getNumberOfTuples() == imageGeom.getNumberOfCells());
    const SizeVec3 dims = imageGeom.getDimensions();
    const usize numCols = dims[0];
    const usize tuplesPerSlice = dims[0] * dims[1];
    usize misplaced = 0;
    for(usize i = 0; i < xBeam.getNumberOfTuples(); i++)
    {
      const usize sliceIndex = i % tuplesPerSlice;
      if(xBeam[i] != static_cast<int32>(sliceIndex % numCols) || yBeam[i] != static_cast<int32>(sliceIndex / numCols))
      {
        misplaced++;
      }
    }
    REQUIRE(misplaced == 0);
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: Multi Scan", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/7_read_oem_ebsd_h5_files_v3.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5EspritDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_435, k_ScanName_436, k_ScanName_437}};

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5EspritDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5EspritDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath exemplarImageGeomPath = DataPath({"Bruker_Multi_Scan"});
  const DataPath generatedImageGeomPath({ImageGeom::k_TypeName});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(generatedImageGeomPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(generatedImageGeomPath);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath));
  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  // Independent oracle (simplnx #1804): every generated cell must hold the measurement whose beam coordinates name that cell.
  {
    const DataPath xBeamPath = generatedImageGeomPath.createChildPath(k_Cell_Data).createChildPath(ebsdlib::H5Esprit::XBEAM);
    const DataPath yBeamPath = generatedImageGeomPath.createChildPath(k_Cell_Data).createChildPath(ebsdlib::H5Esprit::YBEAM);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(xBeamPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(yBeamPath));
    const auto& xBeam = dataStructure.getDataRefAs<Int32Array>(xBeamPath);
    const auto& yBeam = dataStructure.getDataRefAs<Int32Array>(yBeamPath);
    REQUIRE(xBeam.getNumberOfTuples() == imageGeom.getNumberOfCells());
    REQUIRE(yBeam.getNumberOfTuples() == imageGeom.getNumberOfCells());
    const SizeVec3 dims = imageGeom.getDimensions();
    const usize numCols = dims[0];
    const usize tuplesPerSlice = dims[0] * dims[1];
    usize misplaced = 0;
    for(usize i = 0; i < xBeam.getNumberOfTuples(); i++)
    {
      const usize sliceIndex = i % tuplesPerSlice;
      if(xBeam[i] != static_cast<int32>(sliceIndex % numCols) || yBeam[i] != static_cast<int32>(sliceIndex / numCols))
      {
        misplaced++;
      }
    }
    REQUIRE(misplaced == 0);
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, generatedImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: InValid Filter Execution", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files_v3.tar.gz", "7_read_oem_ebsd_h5_files_v3");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5EspritDataFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadH5EspritDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5EspritDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_435}};

  SECTION("Invalid Z Spacing")
  {
    args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(0.0f));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("No Scan Names Selected")
  {
    scanSelections.scanNames.clear();
    args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("Invalid h5 file type (incompatible manufacturer)")
  {
    h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files_v3/EdaxOIMData.h5", unit_test::k_TestFilesDir));
    scanSelections.inputFilePath = h5TestFile;
    args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("Pattern data doesn't exist error")
  {
    args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(true));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ReadH5EspritDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadH5EspritDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadH5EspritDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadH5EspritDataFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(ReadH5EspritDataFilter::k_ZSpacing_Key) == 2.5f);
      CHECK(args.value<bool>(ReadH5EspritDataFilter::k_DegreesToRadians_Key) == true);
      CHECK(args.value<bool>(ReadH5EspritDataFilter::k_ReadPatternData_Key) == true);
      CHECK(args.value<DataPath>(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key) == "TestName");
      const auto scanSel = args.value<OEMEbsdScanSelectionParameter::ValueType>(ReadH5EspritDataFilter::k_SelectedScanNames_Key);
      CHECK(scanSel.inputFilePath == "/test/path/file.h5");
      CHECK(scanSel.scanNames == std::list<std::string>{"Scan1"});
    }
  }
}

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: Scans With Different Dimensions", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path h5TestFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadH5EspritData_Dimensions.h5";
  fs::create_directories(h5TestFile.parent_path());
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(h5TestFile, ec);
  });
  const auto [columns, rows] = GENERATE(std::pair<int32, int32>{2, 2}, std::pair<int32, int32>{3, 4}, std::pair<int32, int32>{4, 2}, std::pair<int32, int32>{2, 3}, std::pair<int32, int32>{4, 3});
  CAPTURE(columns, rows);
  WriteEspritScans(h5TestFile, columns, rows);
  for(const auto& scan : {k_ScanName_1, k_ScanName_2})
  {
    auto reader = ebsdlib::H5EspritReader::New();
    reader->setFileName(h5TestFile.string());
    reader->setHDF5Path(scan);
    const auto readResult = reader->readHeaderOnly();
    INFO(reader->getErrorMessage());
    REQUIRE(readResult >= 0);
    REQUIRE(reader->getXDimension() == (scan == k_ScanName_1 ? 4 : columns));
    REQUIRE(reader->getYDimension() == (scan == k_ScanName_1 ? 3 : rows));
    REQUIRE(reader->getPhaseVector().size() == 1);
  }
  DataStructure dataStructure;
  ReadH5EspritDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, ebsdlib::RefFrameZDir::LowtoHigh, {k_ScanName_1, k_ScanName_2}};
  args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ReadH5EspritDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5EspritDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));
  auto result = filter.preflight(dataStructure, args);
  if(columns == 4 && rows == 3)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -9684);
    const auto& message = result.outputActions.errors().front().message;
    REQUIRE(message.find("Scan '" + k_ScanName_2 + "' in file '" + h5TestFile.string() + "' has " + std::to_string(columns) + " columns and " + std::to_string(rows) + " rows") != std::string::npos);
    REQUIRE(message.find("first selected scan '" + k_ScanName_1 + "' has 4 columns and 3 rows") != std::string::npos);
  }
}
