#include "OrientationAnalysis/Filters/ComputeKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_KernelAverageMisorientationsArrayName_Exemplar("KernelAverageMisorientations");
const std::string k_KernelAverageMisorientationsArrayName("CalculatedKernelAverageMisorientations");
} // namespace

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    // Parameters
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_KernelSize_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{1, 1, 1}));
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    // Cell Arrays
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    // Ensemble Arrays
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    // Output Array
    args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key, std::make_any<std::string>(k_KernelAverageMisorientationsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Cell Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellData, k_KernelAverageMisorientationsArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellData, k_KernelAverageMisorientationsArrayName_Exemplar});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_kernel_average_misorientations.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeKernelAvgMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeKernelAvgMisorientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeKernelAvgMisorientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (IntVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key) == "TestName");
    }
  }
}
