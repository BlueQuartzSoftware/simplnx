/*
# Test Plan

Input Files:
DREAM3D_Data/TestFiles/ASCIIData/EulerAngles.csv
DREAM3D_Data/TestFiles/ASCIIData/Phases.csv

Output DataArrays:
IPFColors (3 component UInt8 Array)

Comparison Files:
DREAM3D_Data/TestFiles/ASCIIData/IPFColor.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. The values should be exactly the same.

*/
#include "OrientationAnalysis/Filters/ComputeIPFColorsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <EbsdLib/LaueOps/OrthoRhombicOps.h>
#include <EbsdLib/Orientation/Euler.hpp>
#include <EbsdLib/Orientation/Homochoric.hpp>
#include <EbsdLib/Orientation/Rodrigues.hpp>

using namespace ebsdlib;

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

namespace nx::core::Constants
{
constexpr StringLiteral k_ImageDataContainer("ImageDataContainer");
constexpr StringLiteral k_OutputIPFColors("IPF Colors_Test_Output");
} // namespace nx::core::Constants

TEST_CASE("OrientationAnalysis::ComputeIPFColors", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{

  {
    HomochoricDType ho(0.021797740480252403, 0.027063934475102136, 0.035554288118377242);
    std::cout << ho << std::endl;
    RodriguesDType rod = ho.toRodrigues();
    std::cout << rod << std::endl;
    OrthoRhombicOps ops;
    rod = ops.getODFFZRod(rod);
    std::cout << rod << "\n";
    EulerDType eu = rod.toEuler();
    std::cout << eu << std::endl;
  }

  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "so3_cubic_high_ipf_001.tar.gz", "so3_cubic_high_ipf_001.dream3d");

  DataStructure dataStructure;
  {

    // This test file was produced by SIMPL/DREAM3D. our results should match theirs
    auto exemplarFilePath = fs::path(fmt::format("{}/so3_cubic_high_ipf_001.dream3d", unit_test::k_TestFilesDir));
    REQUIRE(fs::exists(exemplarFilePath));
    auto result = DREAM3D::ImportDataStructureFromFile(exemplarFilePath, false);
    REQUIRE(result.valid());
    dataStructure = result.value();
  }

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    ComputeIPFColorsFilter filter;
    Arguments args;

    DataPath cellEulerAnglesPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_EulerAngles});
    DataPath cellPhasesArrayPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Phases});
    DataPath goodVoxelsPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Mask});
    DataPath crystalStructuresArrayPath({Constants::k_ImageDataContainer, Constants::k_CellEnsembleData, Constants::k_CrystalStructures});
    DataPath cellIPFColorsArrayName({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_OutputIPFColors});

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeIPFColorsFilter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F}));
    args.insertOrAssign(ComputeIPFColorsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(cellEulerAnglesPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesArrayPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(goodVoxelsPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresArrayPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key, std::make_any<std::string>(Constants::k_OutputIPFColors));

    REQUIRE(dataStructure.getData(goodVoxelsPath) != nullptr);
    REQUIRE(dataStructure.getData(cellEulerAnglesPath) != nullptr);
    REQUIRE(dataStructure.getData(cellPhasesArrayPath) != nullptr);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    {
      // Write out the DataStructure for later viewing/debugging
      auto fileWriter = nx::core::HDF5::FileIO::WriteFile(std::filesystem::path(fmt::format("{}/ComputeIPFColors_Test.dream3d", unit_test::k_BinaryTestOutputDir)));
      auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
      SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
    }

    DataPath ipfColors({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Ipf_Colors});

    UInt8Array& exemplar = dataStructure.getDataRefAs<UInt8Array>(ipfColors);
    UInt8Array& output = dataStructure.getDataRefAs<UInt8Array>(cellIPFColorsArrayName);

    size_t totalElements = exemplar.getSize();
    bool valid = true;
    for(size_t i = 0; i < totalElements; i++)
    {
      if(exemplar[i] != output[i])
      {
        valid = false;
        break;
      }
    }
    REQUIRE(valid == true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Plumbing test: the k_ColorKey_Key choice index must route through executeImpl's
// switch into the right `ebsdlib::ColorKeyKind` and reach generateIPFColor. The
// per-Laue-class correctness of TSL / PUCM / Nolze-Hielscher is covered by
// EbsdLib's ColorKeyKindTest; here we only assert that the simplnx side wiring
// is intact -- non-default choices must produce a different output array than
// the default (TSL) run on the same input data.
TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: ColorKey choice reaches algorithm", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "so3_cubic_high_ipf_001.tar.gz", "so3_cubic_high_ipf_001.dream3d");

  auto exemplarFilePath = fs::path(fmt::format("{}/so3_cubic_high_ipf_001.dream3d", unit_test::k_TestFilesDir));
  REQUIRE(fs::exists(exemplarFilePath));
  auto importResult = DREAM3D::ImportDataStructureFromFile(exemplarFilePath, false);
  REQUIRE(importResult.valid());
  DataStructure dataStructure = importResult.value();

  const DataPath cellEulerAnglesPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_EulerAngles});
  const DataPath cellPhasesArrayPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Phases});
  const DataPath goodVoxelsPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Mask});
  const DataPath crystalStructuresArrayPath({Constants::k_ImageDataContainer, Constants::k_CellEnsembleData, Constants::k_CrystalStructures});

  // Run the filter once per kind, writing into a uniquely-named output array.
  auto runWithKind = [&](ChoicesParameter::ValueType kindIndex, const std::string& outputName) {
    ComputeIPFColorsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeIPFColorsFilter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F}));
    args.insertOrAssign(ComputeIPFColorsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(cellEulerAnglesPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesArrayPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(goodVoxelsPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresArrayPath));
    args.insertOrAssign(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key, std::make_any<std::string>(outputName));
    args.insertOrAssign(ComputeIPFColorsFilter::k_ColorKey_Key, std::make_any<ChoicesParameter::ValueType>(kindIndex));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  runWithKind(0, "IPFColors_TSL");
  runWithKind(1, "IPFColors_PUCM");
  runWithKind(2, "IPFColors_NH");

  const auto& tslColors = dataStructure.getDataRefAs<UInt8Array>(DataPath({Constants::k_ImageDataContainer, Constants::k_CellData, "IPFColors_TSL"}));
  const auto& pucmColors = dataStructure.getDataRefAs<UInt8Array>(DataPath({Constants::k_ImageDataContainer, Constants::k_CellData, "IPFColors_PUCM"}));
  const auto& nhColors = dataStructure.getDataRefAs<UInt8Array>(DataPath({Constants::k_ImageDataContainer, Constants::k_CellData, "IPFColors_NH"}));

  REQUIRE(tslColors.getSize() == pucmColors.getSize());
  REQUIRE(tslColors.getSize() == nhColors.getSize());

  // Sanity: at least one tuple must differ between TSL and each other kind. If
  // the switch in executeImpl ever silently collapsed every kind onto TSL,
  // these arrays would be identical.
  REQUIRE(!std::equal(tslColors.cbegin(), tslColors.cend(), pucmColors.cbegin()));
  REQUIRE(!std::equal(tslColors.cbegin(), tslColors.cend(), nhColors.cbegin()));
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeIPFColorsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeIPFColorsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeIPFColorsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeIPFColorsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(ComputeIPFColorsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key) == "TestName");
    }
  }
}
