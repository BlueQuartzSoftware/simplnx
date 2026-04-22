#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/MergeTwinsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("OrientationAnalysis::MergeTwinsFilter: Valid Execution", "[OrientationAnalysis][MergeTwinsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_MergeTwins.tar.gz", "6_5_MergeTwins/6_5_MergeTwins.dream3d");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_MergeTwins/6_5_MergeTwins.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath k_CellDataPath({k_DataContainer, k_CellData});
  const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath(k_FeatureIds);
  const DataPath k_ContiguousNeighborPath = k_DataContainerPath.createChildPath(k_FeatureData).createChildPath("NeighborList2");
  const DataPath k_AvgQuatsPath = k_DataContainerPath.createChildPath(k_FeatureData).createChildPath(k_AvgQuats);
  const DataPath k_FeaturePhasesArrayPath = k_DataContainerPath.createChildPath(k_FeatureData).createChildPath(k_Phases);

  const std::string k_CellParentIdsArrayName = "ParentIds";
  const std::string k_NewFeatureAttributeMatrixName = "NewGrain Data";
  const std::string k_FeatureParentIdsName = "ParentIds";
  const std::string k_ActiveArrayName = "Active";
  {
    MergeTwinsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(MergeTwinsFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_ContiguousNeighborPath));
    args.insertOrAssign(MergeTwinsFilter::k_AxisTolerance_Key, std::make_any<float32>(3.0f));
    args.insertOrAssign(MergeTwinsFilter::k_AngleTolerance_Key, std::make_any<float32>(2.0f));
    args.insertOrAssign(MergeTwinsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesArrayPath));
    args.insertOrAssign(MergeTwinsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_AvgQuatsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(MergeTwinsFilter::k_CellParentIdsArrayName_Key, std::make_any<std::string>(k_CellParentIdsArrayName));
    args.insertOrAssign(MergeTwinsFilter::k_CreatedFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_NewFeatureAttributeMatrixName));
    args.insertOrAssign(MergeTwinsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<std::string>(k_FeatureParentIdsName));
    args.insertOrAssign(MergeTwinsFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveArrayName));

    args.insertOrAssign(MergeTwinsFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(MergeTwinsFilter::k_SeedValue_Key, std::make_any<uint64>(5349));
    args.insertOrAssign(MergeTwinsFilter::k_SeedArrayName_Key, std::make_any<std::string>("MergeTwins SeedValue"));
    args.insertOrAssign(MergeTwinsFilter::k_RandomizeParentIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/merge_twins.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath("6_5_ParentIds"));
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsArrayName));
    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(k_DataContainerPath.createChildPath(k_FeatureData).createChildPath("6_5_ParentIds"));
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(k_DataContainerPath.createChildPath(k_FeatureData).createChildPath(k_CellParentIdsArrayName));
    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  AttributeMatrix* newAMPtr = dataStructure.getDataAs<AttributeMatrix>(k_DataContainerPath.createChildPath(k_NewFeatureAttributeMatrixName));
  REQUIRE(newAMPtr != nullptr);

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<BoolArray>(k_DataContainerPath.createChildPath("6_5_NewGrain Data").createChildPath("Active"));
    const auto& exemplarDataArray = dataStructure.getDataRefAs<BoolArray>(k_DataContainerPath.createChildPath(k_NewFeatureAttributeMatrixName).createChildPath(k_ActiveArrayName));
    UnitTest::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::MergeTwinsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][MergeTwinsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "MergeTwinsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "MergeTwinsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<MergeTwinsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(MergeTwinsFilter::k_ContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<float32>(MergeTwinsFilter::k_AxisTolerance_Key) == 2.5f);
      CHECK(args.value<float32>(MergeTwinsFilter::k_AngleTolerance_Key) == 2.5f);
      CHECK(args.value<DataPath>(MergeTwinsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(MergeTwinsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(MergeTwinsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(MergeTwinsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(MergeTwinsFilter::k_CellParentIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(MergeTwinsFilter::k_CreatedFeatureAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(MergeTwinsFilter::k_FeatureParentIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(MergeTwinsFilter::k_ActiveArrayName_Key) == "TestName");
    }
  }
}
