#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ImportH5OimDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include "EbsdLib/IO/TSL/AngFields.h"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_ScanName = "Scan 1";
}

TEST_CASE("OrientationAnalysis::ImportH5OimDataFilter: Valid Filter Execution", "[OrientationAnalysis][ImportH5OimDataFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_ImportH5Data.tar.gz", "6_6_ImportH5Data");

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_ImportH5Data/6_6_import_h5_oim_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportH5OimDataFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto h5TestFile = fs::path(fmt::format("{}/6_6_ImportH5Data/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, EbsdLib::RefFrameZDir::LowtoHigh, {k_ScanName}};

  // Create default Parameters for the filter.
  args.insertOrAssign(ImportH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
  args.insertOrAssign(ImportH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(ImportH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ImportH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ImportH5OimDataFilter::k_ImageGeometryName_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ImportH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ImportH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({ImageGeom::k_TypeName}));
  const auto& exemplarImageGeom = exemplarDataStructure.getDataRefAs<ImageGeom>(DataPath({k_ExemplarDataContainer}));
  REQUIRE(imageGeom.getDimensions() == exemplarImageGeom.getDimensions());
  REQUIRE(imageGeom.getSpacing() == exemplarImageGeom.getSpacing());
  REQUIRE(imageGeom.getOrigin() == exemplarImageGeom.getOrigin());
  REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, DataPath({ImageGeom::k_TypeName, k_CellData}), k_ExemplarDataContainer);

  const DataPath cellEnsemblePath({ImageGeom::k_TypeName, k_CellEnsembleData});
  const DataPath exemplarCellEnsemblePath({k_ExemplarDataContainer, k_CellEnsembleData});
  const auto& crystalStructures = dataStructure.getDataRefAs<UInt32Array>(cellEnsemblePath.createChildPath(EbsdLib::AngFile::CrystalStructures));
  const auto& crystalStructuresExemplar = exemplarDataStructure.getDataRefAs<UInt32Array>(exemplarCellEnsemblePath.createChildPath(EbsdLib::AngFile::CrystalStructures));
  UnitTest::CompareDataArrays<uint32>(crystalStructures, crystalStructuresExemplar);
  const auto& latticeConstants = dataStructure.getDataRefAs<Float32Array>(cellEnsemblePath.createChildPath(EbsdLib::AngFile::LatticeConstants));
  const auto& latticeConstantsExemplar = exemplarDataStructure.getDataRefAs<Float32Array>(exemplarCellEnsemblePath.createChildPath(EbsdLib::AngFile::LatticeConstants));
  UnitTest::CompareDataArrays<float32>(latticeConstants, latticeConstantsExemplar);
  const auto& materialName = dataStructure.getDataRefAs<StringArray>(cellEnsemblePath.createChildPath(EbsdLib::AngFile::MaterialName));
  const auto& materialNameExemplar = exemplarDataStructure.getDataRefAs<StringArray>(exemplarCellEnsemblePath.createChildPath(EbsdLib::AngFile::MaterialName));
  UnitTest::CompareStringArrays(materialNameExemplar, materialName);
}

TEST_CASE("OrientationAnalysis::ImportH5OimDataFilter: InValid Filter Execution", "[OrientationAnalysis][ImportH5OimDataFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_ImportH5Data.tar.gz", "6_6_ImportH5Data");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportH5OimDataFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ImportH5OimDataFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3, 0.0f)));
  args.insertOrAssign(ImportH5OimDataFilter::k_ImageGeometryName_Key, std::make_any<DataPath>(DataPath({ImageGeom::k_TypeName})));
  args.insertOrAssign(ImportH5OimDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ImportH5OimDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleData));

  auto h5TestFile = fs::path(fmt::format("{}/6_6_ImportH5Data/EdaxOIMData.h5", unit_test::k_TestFilesDir));
  OEMEbsdScanSelectionParameter::ValueType scanSelections = {h5TestFile, EbsdLib::RefFrameZDir::LowtoHigh, {k_ScanName}};

  SECTION("Invalid Z Spacing")
  {
    args.insertOrAssign(ImportH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ImportH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(0.0f));
    args.insertOrAssign(ImportH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("No Scan Names Selected")
  {
    scanSelections.scanNames.clear();
    args.insertOrAssign(ImportH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ImportH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ImportH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }
  SECTION("Invalid h5 file type (incompatible manufacturer)")
  {
    h5TestFile = fs::path(fmt::format("{}/6_6_ImportH5Data/H5EspritReaderTest.h5", unit_test::k_TestFilesDir));
    scanSelections.inputFilePath = h5TestFile;
    args.insertOrAssign(ImportH5OimDataFilter::k_SelectedScanNames_Key, std::make_any<OEMEbsdScanSelectionParameter::ValueType>(scanSelections));
    args.insertOrAssign(ImportH5OimDataFilter::k_ZSpacing_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(ImportH5OimDataFilter::k_ReadPatternData_Key, std::make_any<bool>(false));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
