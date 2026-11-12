#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include <H5Support/H5Lite.h>
#include <H5Support/H5ScopedSentinel.h>
#include <H5Support/H5Utilities.h>
#include <catch2/catch.hpp>
#include <limits>

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

#include "EbsdLib/IO/BrukerNano/EspritConstants.h"

#include <EbsdLib/IO/TSL/AngFields.h>

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_ScanName_435 = "Section_435";
const std::string k_ScanName_436 = "Section_436";
const std::string k_ScanName_437 = "Section_437";
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
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE_FALSE(result.value().actions.empty());
    const auto* geometryAction = dynamic_cast<const CreateImageGeometryAction*>(result.value().actions[0].get());
    REQUIRE(geometryAction != nullptr);
    REQUIRE(geometryAction->spacing()[field == "xstep" ? 0 : 1] == (laterScan ? 0.5F : 1.0F));
    return;
  }

  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  const auto executeResult = filter.execute(ds, argsForValidation);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == result.errors()[0].code);
  // Zero patterns already have an existing error. Negative patterns need positive-domain validation.
  if((field == "width" || field == "height") && variant % 2 == 0)
  {
    return;
  }
  REQUIRE(result.errors()[0].code == (field == "symmetry" ? -67600 : field == "columns" || field == "rows" || field == "width" || field == "height" ? -67601 : -67602));
}
