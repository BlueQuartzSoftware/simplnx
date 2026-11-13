#include "OrientationAnalysis/Filters/Algorithms/ReadH5EspritData.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include <H5Support/H5Lite.h>
#include <H5Support/H5ScopedSentinel.h>
#include <H5Support/H5Utilities.h>
#include <catch2/catch.hpp>
#include <limits>

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

TEST_CASE("OrientationAnalysis::ReadH5EspritData: HeaderValidation", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  const std::string field = GENERATE("columns", "rows", "xstep", "ystep", "width", "height", "symmetry");
  const auto variant = GENERATE(0, 1, 2, 3);
  const bool laterScan = GENERATE(false, true);
  CAPTURE(field, variant, laterScan);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "header_validation_Esprit.h5";
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
        REQUIRE(H5Lite::writeScalarDataset<float64>(id, header + name, invalidHeader && field == target ? values[variant] : 0.5) >= 0);
      };

      REQUIRE(H5Support::H5Utilities::createGroupsFromPath(header + "Phases/1", id) >= 0);
      REQUIRE(H5Support::H5Utilities::createGroupsFromPath(scanName + "/EBSD/Data", id) >= 0);
      intValue(ebsdlib::H5Esprit::NCOLS, "columns", 1);
      intValue(ebsdlib::H5Esprit::NROWS, "rows", 1);
      intValue(ebsdlib::H5Esprit::PatternWidth, "width", 1);
      intValue(ebsdlib::H5Esprit::PatternHeight, "height", 1);
      REQUIRE(H5Lite::writeScalarDataset<int32>(id, header + ebsdlib::H5Esprit::NPoints, 1) >= 0);
      stepValue(ebsdlib::H5Esprit::XSTEP, "xstep");
      stepValue(ebsdlib::H5Esprit::YSTEP, "ystep");
      for(const auto& key : {ebsdlib::H5Esprit::GridType, ebsdlib::H5Esprit::OriginalFile})
      {
        REQUIRE(H5Lite::writeStringDataset(id, header + key, "SqrGrid") >= 0);
      }
      for(const auto& key : {ebsdlib::H5Esprit::SEPixelSizeX, ebsdlib::H5Esprit::SEPixelSizeY, ebsdlib::H5Esprit::ZOffset})
      {
        REQUIRE(H5Lite::writeScalarDataset<float64>(id, header + key, 1.0) >= 0);
      }
      const std::string phase = header + "Phases/1/";
      for(const auto& key : {ebsdlib::H5Esprit::Formula, ebsdlib::H5Esprit::Name, ebsdlib::H5Esprit::SpaceGroup})
      {
        REQUIRE(H5Lite::writeStringDataset(id, phase + key, "Nickel") >= 0);
      }
      REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::H5Esprit::IT, invalidHeader && field == "symmetry" ? 999 : 225) >= 0);
      REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::H5Esprit::Setting, 1) >= 0);
      REQUIRE(H5Lite::writeVectorDataset<float32>(id, phase + ebsdlib::H5Esprit::LatticeConstants, {6}, {1, 1, 1, 90, 90, 90}) >= 0);
      REQUIRE(H5Lite::writeVectorDataset<uint8>(id, scanName + "/EBSD/Data/" + ebsdlib::H5Esprit::RawPatterns, {1, 1, 1}, {0}) >= 0);
    }
  }
  DataStructure ds;
  ReadH5EspritDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType selection = {path, ebsdlib::RefFrameZDir::LowtoHigh, {"Scan", "Scan2"}};
  args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(field == "width" || field == "height"));
  const auto argsForValidation = args;
  auto result = filter.preflight(ds, argsForValidation).outputActions;

  if((field == "xstep" || field == "ystep") && variant == 3)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -67607);
    selection.scanNames = {laterScan ? "Scan2" : "Scan"};
    args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
    auto singleScan = filter.preflight(ds, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(singleScan);
    const auto* geometryAction = dynamic_cast<const CreateImageGeometryAction*>(singleScan.value().actions[0].get());
    REQUIRE(geometryAction != nullptr);
    REQUIRE(geometryAction->spacing()[field == "xstep" ? 0 : 1] == 1.0F);
    return;
  }

  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  const auto executeResult = filter.execute(ds, argsForValidation);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == result.errors()[0].code);
  ReadH5DataInputValues directValues{};
  directValues.SelectedScanNames = selection;
  directValues.ReadPatternData = field == "width" || field == "height";
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  ReadH5EspritDataInputValues esprit{};
  ReadH5EspritData algorithm(ds, handler, cancel, &directValues, &esprit);
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
void WriteStructuralEspritFixture(const fs::path& path, const std::string& field)
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
      REQUIRE(H5Lite::writeScalarDataset<float64>(id, header + name, scanName == "Scan2" && field == target ? 0.75 : 0.5) >= 0);
    };

    REQUIRE(H5Support::H5Utilities::createGroupsFromPath(header + "Phases/1", id) >= 0);
    REQUIRE(H5Support::H5Utilities::createGroupsFromPath(scanName + "/EBSD/Data", id) >= 0);
    intValue(ebsdlib::H5Esprit::NCOLS, "columns", field == "short" ? 2 : 1);
    intValue(ebsdlib::H5Esprit::NROWS, "rows", 1);
    intValue(ebsdlib::H5Esprit::PatternWidth, "width", 1);
    intValue(ebsdlib::H5Esprit::PatternHeight, "height", 1);
    REQUIRE(H5Lite::writeScalarDataset<int32>(id, header + ebsdlib::H5Esprit::NPoints, 1) >= 0);
    stepValue(ebsdlib::H5Esprit::XSTEP, "xstep");
    stepValue(ebsdlib::H5Esprit::YSTEP, "ystep");
    for(const auto& key : {ebsdlib::H5Esprit::GridType, ebsdlib::H5Esprit::OriginalFile})
    {
      REQUIRE(H5Lite::writeStringDataset(id, header + key, "isometric") >= 0);
    }
    for(const auto& key : {ebsdlib::H5Esprit::SEPixelSizeX, ebsdlib::H5Esprit::SEPixelSizeY, ebsdlib::H5Esprit::ZOffset})
    {
      REQUIRE(H5Lite::writeScalarDataset<float64>(id, header + key, 1.0) >= 0);
    }
    const std::string phase = header + "Phases/1/";
    for(const auto& key : {ebsdlib::H5Esprit::Formula, ebsdlib::H5Esprit::Name, ebsdlib::H5Esprit::SpaceGroup})
    {
      REQUIRE(H5Lite::writeStringDataset(id, phase + key, "Nickel") >= 0);
    }
    REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::H5Esprit::IT, 225) >= 0);
    REQUIRE(H5Lite::writeScalarDataset<int32>(id, phase + ebsdlib::H5Esprit::Setting, 1) >= 0);
    REQUIRE(H5Lite::writeVectorDataset<float32>(id, phase + ebsdlib::H5Esprit::LatticeConstants, {6}, {1, 1, 1, 90, 90, 90}) >= 0);

    if(scanName == "Scan2" && field == "phases")
    {
      REQUIRE(H5Ocopy(id, (header + "Phases/1").c_str(), id, (header + "Phases/2").c_str(), H5P_DEFAULT, H5P_DEFAULT) >= 0);
    }
    if(scanName == "Scan2" && field == "material")
    {
      REQUIRE(H5Ldelete(id, (phase + ebsdlib::H5Esprit::Name).c_str(), H5P_DEFAULT) >= 0);
      REQUIRE(H5Lite::writeStringDataset(id, phase + ebsdlib::H5Esprit::Name, "Iron") >= 0);
    }
    REQUIRE(H5Support::H5Utilities::createGroupsFromPath(scanName + "/EBSD/Data", id) >= 0);
    if(field == "data")
    {
      for(const auto& name : {ebsdlib::H5Esprit::phi1, ebsdlib::H5Esprit::PHI, ebsdlib::H5Esprit::phi2, ebsdlib::H5Esprit::MAD, ebsdlib::H5Esprit::RadonQuality})
      {
        REQUIRE(H5Lite::writeVectorDataset<float32>(id, scanName + "/EBSD/Data/" + name, {1}, {0.25F}) >= 0);
      }
      for(const auto& name : {ebsdlib::H5Esprit::NIndexedBands, ebsdlib::H5Esprit::Phase, ebsdlib::H5Esprit::RadonBandCount, ebsdlib::H5Esprit::XBEAM, ebsdlib::H5Esprit::YBEAM})
      {
        REQUIRE(H5Lite::writeVectorDataset<int32>(id, scanName + "/EBSD/Data/" + name, {1}, {name == ebsdlib::H5Esprit::Phase ? 1 : 0}) >= 0);
      }
    }
    const std::string dataset = scanName + "/EBSD/Data/" + ebsdlib::H5Esprit::RawPatterns;
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

TEST_CASE("OrientationAnalysis::ReadH5EspritData: StructuralValidation", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  const std::string field = GENERATE("columns", "rows", "xstep", "ystep", "phases", "material", "width", "height", "short", "type", "rank", "components");
  CAPTURE(field);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "structural_Esprit.h5";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteStructuralEspritFixture(path, field);
  DataStructure ds;
  ReadH5EspritDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType selection = {path, ebsdlib::RefFrameZDir::LowtoHigh, {"Scan", "Scan2"}};
  args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key,
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
  WriteStructuralEspritFixture(path, "valid");
  auto valid = filter.preflight(ds, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(valid);
  auto applied = valid.value().applyRegular(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
  WriteStructuralEspritFixture(path, field);
  ReadH5DataInputValues values{};
  values.SelectedScanNames = selection;
  values.ReadPatternData = field == "width" || field == "height" || field == "short" || field == "type" || field == "rank" || field == "components" || field == "missing";
  const auto defaults = filter.getDefaultArguments();
  values.ImageGeometryPath = defaults.value<DataPath>(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key);
  values.CellAttributeMatrixPath = values.ImageGeometryPath.createChildPath(defaults.value<std::string>(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key));
  values.CellEnsembleAttributeMatrixPath = values.ImageGeometryPath.createChildPath(defaults.value<std::string>(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key));
  const std::atomic_bool cancel = false;
  ReadH5EspritDataInputValues esprit{};
  const IFilter::MessageHandler messageHandler;
  ReadH5EspritData algorithm(ds, messageHandler, cancel, &values, &esprit);
  auto direct = algorithm.readData(field == "short" || field == "type" || field == "rank" || field == "components" ? "Scan" : "Scan2");
  SIMPLNX_RESULT_REQUIRE_INVALID(direct);
  REQUIRE(direct.errors().front().code == expected);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ReadH5EspritData: StructuralValidation valid patterns and stale destination", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  const int stale = GENERATE(0, 1, 2, 3);
  CAPTURE(stale);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "structural_Esprit_patterns.h5";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteStructuralEspritFixture(path, "data");
  ReadH5EspritDataFilter filter;
  Arguments args;
  OEMEbsdScanSelectionParameter::ValueType selection = {path, ebsdlib::RefFrameZDir::LowtoHigh, {"Scan", "Scan2"}};
  const DataPath geomPath({"Structural"});
  args.insertOrAssign(ReadH5EspritDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(selection));
  args.insertOrAssign(ReadH5EspritDataFilter::k_ReadPatternData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cells"));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>("Phases"));
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
        REQUIRE(H5Lmove(id, (scan + "/EBSD/Header/Phases/1").c_str(), id, (scan + "/EBSD/Header/Phases/2").c_str(), H5P_DEFAULT, H5P_DEFAULT) >= 0);
      }
    }
    const std::atomic_bool cancel = false;
    ReadH5EspritDataInputValues esprit{};
    auto result = ReadH5EspritData(ds, {}, cancel, &values, &esprit)();
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == (stale == 1 ? -67616 : stale == 2 ? -67617 : -67618));
  }
  else
  {
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto dataPath = geomPath.createChildPath("Cells").createChildPath(ebsdlib::H5Esprit::RawPatterns);
    REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(dataPath));
    const auto& patterns = ds.getDataRefAs<UInt8Array>(dataPath);
    REQUIRE(patterns.getSize() == 2);
    REQUIRE(patterns[0] == 17);
    REQUIRE(patterns[1] == 29);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
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
    // -67606 is the scan grid mismatch code that the OIM and Esprit readers share.
    REQUIRE(result.outputActions.errors().front().code == -67606);
    const auto& message = result.outputActions.errors().front().message;
    REQUIRE(message.find("Scan '" + k_ScanName_2 + "' in file '" + h5TestFile.string() + "' has " + std::to_string(columns) + " columns and " + std::to_string(rows) + " rows") != std::string::npos);
    REQUIRE(message.find("first selected scan '" + k_ScanName_1 + "' has 4 columns and 3 rows") != std::string::npos);
  }
}
