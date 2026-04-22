/*
# Test Plan

Input Files:
DREAM3D_Data/TestFiles/ASCIIData/FeatureIds.csv (int32, 1 component)
DREAM3D_Data/TestFiles/ASCIIData/Quats.csv (float32, 4 component)
DREAM3D_Data/TestFiles/ASCIIData/Phases.csv (int32, 1 component)

Output DataArrays:
AvgEulerAngles  (float32, 3 component)
AvgQuats  (float32, 4 component)

Comparison Files:
DREAM3D_Data/TestFiles/ASCIIData/AvgEulerAngles.csv
DREAM3D_Data/TestFiles/ASCIIData/AvgQuats.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. Due to going back and forth between ASCII and Binary you will
probably have to compare using a tolerance of about .0001. Look at the 'ConvertOrientationsTest' at the bottom for an example
of doing that.

*/
#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace compute_avg_orientation
{
const std::string k_ImageDataContainerName = "ImageGeom";
const DataPath k_ImageDataContainerPath({k_ImageDataContainerName});
const DataPath k_CellDataPath = k_ImageDataContainerPath.createChildPath("Cell Data");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");

const DataPath k_CrystalStructuresPath = k_ImageDataContainerPath.createChildPath("Cell Ensemble Data").createChildPath("CrystalStructures");

const DataPath k_FeatureAttrMatPath = k_ImageDataContainerPath.createChildPath("Cell Feature Data");

const std::string k_AvgQuatsName = "Computed AvgQuats";
const std::string k_AvgEulersName = "Computed AvgEulerAngles";

const std::string k_ComputedWatsonAvgQuatsName = "Computed Watson AvgQuats";
const std::string k_ComputedWatsonAvgEulersName = "Computed Watson AvgEulerAngles";
const std::string k_ComputedWatsonKappasName = "Computed Watson Kappas";

const std::string k_ComputedVMFAvgQuatsName = "Computed VMF AvgQuats";
const std::string k_ComputedVMFAvgEulersName = "Computed VMF AvgEulerAngles";
const std::string k_ComputedVMFKappasName = "Computed VMF Kappas";

const std::string k_Exemplar_AvgQuatsName = "AvgQuats";
const std::string k_Exemplar_AvgEulersName = "AvgEulerAngles";

const std::string k_Exemplar_WatsonAvgQuatsName = "Watson Avg Quats";
const std::string k_Exemplar_WatsonAvgEulersName = "Watson Avg EulerAngles";

const std::string k_Exemplar_VMFAvgQuatsName = "vMF Avg Quats";
const std::string k_Exemplar_VMFAvgEulersName = "vMF Avg EulerAngles";
} // namespace compute_avg_orientation

TEST_CASE("OrientationAnalysis::ComputeAvgOrientations", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_ComputeAvgOrientation_v2.tar.gz", "7_ComputeAvgOrientation_v2");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/7_ComputeAvgOrientation_v2/7_ComputeAvgOrientation_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeAvgOrientationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_FeatureIdsPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_PhasesPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_QuatsPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_CrystalStructuresPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_FeatureAttrMatPath));

  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_AvgEulersName));

  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_ComputedWatsonAvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_ComputedWatsonAvgEulersName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_ComputedWatsonKappasName));

  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_ComputedVMFAvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_ComputedVMFAvgEulersName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_ComputedVMFKappasName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/compute_average_orientations.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    DataPath computedAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_AvgEulersName);
    DataPath exemplarAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_AvgEulersName);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgEulersPath, exemplarAvgEulersPath, 5.0E-7f, false);

    DataPath computedAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_AvgQuatsName);
    DataPath exemplarAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_AvgQuatsName);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgQuatsPath, exemplarAvgQuatsPath, 5.0E-7f, false);
  }

  {
    DataPath computedAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_ComputedWatsonAvgEulersName);
    DataPath exemplarAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_WatsonAvgEulersName);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgEulersPath, exemplarAvgEulersPath, 5.0E-7f, false);

    DataPath computedAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_ComputedWatsonAvgQuatsName);
    DataPath exemplarAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_WatsonAvgQuatsName);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgQuatsPath, exemplarAvgQuatsPath, 5.0E-7f, false);
  }

  {
    DataPath computedAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_ComputedVMFAvgEulersName);
    DataPath exemplarAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_VMFAvgEulersName);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgEulersPath, exemplarAvgEulersPath, 5.0E-7f, false);

    DataPath computedAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_ComputedVMFAvgQuatsName);
    DataPath exemplarAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_VMFAvgQuatsName);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgQuatsPath, exemplarAvgQuatsPath, 5.0E-7f, false);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeAvgOrientationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeAvgOrientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeAvgOrientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeAvgOrientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeAvgOrientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key) == "TestArray");
      CHECK(args.value<std::string>(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key) == "TestArray");
    }
  }
}
