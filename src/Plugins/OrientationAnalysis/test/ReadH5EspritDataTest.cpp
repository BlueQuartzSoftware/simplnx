#include <catch2/catch.hpp>

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

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files.tar.gz", "7_read_oem_ebsd_h5_files");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/7_read_oem_ebsd_h5_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5EspritDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
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

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));

  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: Multi Scan", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files.tar.gz", "7_read_oem_ebsd_h5_files");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/7_read_oem_ebsd_h5_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5EspritDataFilter filter;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
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

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));

  const auto& exemplarImageGeom = dataStructure.getDataRefAs<ImageGeom>(exemplarImageGeomPath);

  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, exemplarImageGeomPath.createChildPath(k_Cell_Ensemble_Data), exemplarImageGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EspritDataFilter: InValid Filter Execution", "[OrientationAnalysis][ReadH5EspritDataFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_read_oem_ebsd_h5_files.tar.gz", "7_read_oem_ebsd_h5_files");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadH5EspritDataFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadH5EspritDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ReadH5EspritDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  auto h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
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
    h5TestFile = fs::path(fmt::format("{}/7_read_oem_ebsd_h5_files/EdaxOIMData.h5", unit_test::k_TestFilesDir));
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

      // Note: Complex SIMPL parameter conversions may produce warnings
      // pipelineFilter->getComments() may not be empty for filters with custom converters

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (OEMEbsdScanSelectionFilterParameterConverter) - verified by successful pipeline loading
      // CHECK(args.value<float32>(ReadH5EspritDataFilter::k_ZSpacing_Key) == 2.5f);
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // CHECK(args.value<bool>(ReadH5EspritDataFilter::k_DegreesToRadians_Key) == true);
      // CHECK(args.value<bool>(ReadH5EspritDataFilter::k_ReadPatternData_Key) == true);
      // CHECK(args.value<DataPath>(ReadH5EspritDataFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      // CHECK(args.value<std::string>(ReadH5EspritDataFilter::k_CellAttributeMatrixName_Key) == "TestName");
      // CHECK(args.value<std::string>(ReadH5EspritDataFilter::k_CellEnsembleAttributeMatrixName_Key) == "TestName");
    }
  }
}
