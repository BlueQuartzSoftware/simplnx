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
using namespace nx::core::constants;
using namespace nx::core::unit_test;

TEST_CASE("OrientationAnalysis::MergeTwinsFilter: Valid Execution", "[OrientationAnalysis][MergeTwinsFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_MergeTwins.tar.gz", "6_5_MergeTwins/6_5_MergeTwins.dream3d");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_MergeTwins/6_5_MergeTwins.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  const DataPath kCellDataPath({k_DataContainer, k_CellData});
  const DataPath kFeatureIdsPath = kCellDataPath.createChildPath(k_FeatureIds);
  const DataPath kContiguousNeighborPath = k_DataContainerPath.createChildPath(k_FeatureData).createChildPath("NeighborList2");
  const DataPath kAvgQuatsPath = k_DataContainerPath.createChildPath(k_FeatureData).createChildPath(k_AvgQuats);
  const DataPath kFeaturePhasesArrayPath = k_DataContainerPath.createChildPath(k_FeatureData).createChildPath(k_Phases);

  const std::string kCellParentIdsArrayName = "ParentIds";
  const std::string kNewFeatureAttributeMatrixName = "NewGrain Data";
  const std::string kFeatureParentIdsName = "ParentIds";
  const std::string kActiveArrayName = "Active";
  {
    MergeTwinsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(MergeTwinsFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(kContiguousNeighborPath));
    args.insertOrAssign(MergeTwinsFilter::k_AxisTolerance_Key, std::make_any<float32>(3.0f));
    args.insertOrAssign(MergeTwinsFilter::k_AngleTolerance_Key, std::make_any<float32>(2.0f));
    args.insertOrAssign(MergeTwinsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(kFeaturePhasesArrayPath));
    args.insertOrAssign(MergeTwinsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(kAvgQuatsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(MergeTwinsFilter::k_CellParentIdsArrayName_Key, std::make_any<std::string>(kCellParentIdsArrayName));
    args.insertOrAssign(MergeTwinsFilter::k_CreatedFeatureAttributeMatrixName_Key, std::make_any<std::string>(kNewFeatureAttributeMatrixName));
    args.insertOrAssign(MergeTwinsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<std::string>(kFeatureParentIdsName));
    args.insertOrAssign(MergeTwinsFilter::k_ActiveArrayName_Key, std::make_any<std::string>(kActiveArrayName));

    args.insertOrAssign(MergeTwinsFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(MergeTwinsFilter::k_SeedValue_Key, std::make_any<uint64>(5349));
    args.insertOrAssign(MergeTwinsFilter::k_SeedArrayName_Key, std::make_any<std::string>("MergeTwins SeedValue"));
    args.insertOrAssign(MergeTwinsFilter::k_RandomizeParentIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/merge_twins.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32ArrayType>(kCellDataPath.createChildPath("6_5_ParentIds"));
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32ArrayType>(kCellDataPath.createChildPath(kCellParentIdsArrayName));
    unit_test::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32ArrayType>(k_DataContainerPath.createChildPath(k_FeatureData).createChildPath("6_5_ParentIds"));
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32ArrayType>(k_DataContainerPath.createChildPath(k_FeatureData).createChildPath(kCellParentIdsArrayName));
    unit_test::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  AttributeMatrix* newAMPtr = dataStructure.getDataAs<AttributeMatrix>(k_DataContainerPath.createChildPath(kNewFeatureAttributeMatrixName));
  REQUIRE(newAMPtr != nullptr);

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<BoolArrayType>(k_DataContainerPath.createChildPath("6_5_NewGrain Data").createChildPath("Active"));
    const auto& exemplarDataArray = dataStructure.getDataRefAs<BoolArrayType>(k_DataContainerPath.createChildPath(kNewFeatureAttributeMatrixName).createChildPath(kActiveArrayName));
    unit_test::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::MergeTwinsFilter: Phase Index Bounds", "[OrientationAnalysis][MergeTwinsFilter]")
{
  unit_test::LoadPlugins();

  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_MergeTwins.tar.gz", "6_5_MergeTwins/6_5_MergeTwins.dream3d");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "6_5_MergeTwins" / "6_5_MergeTwins.dream3d";
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFile);

  const DataPath featureDataPath = k_DataContainerPath.createChildPath(k_FeatureData);
  const DataPath featurePhasesPath = featureDataPath.createChildPath(k_Phases);
  const DataPath avgQuatsPath = featureDataPath.createChildPath(k_AvgQuats);
  const DataPath contiguousNeighborPath = featureDataPath.createChildPath("NeighborList2");
  const DataPath featureIdsPath = k_DataContainerPath.createChildPath(k_CellData).createChildPath(k_FeatureIds);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(featurePhasesPath));
  auto& featurePhasesArrayRef = dataStructure.getDataRefAs<Int32ArrayType>(featurePhasesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(k_CrystalStructuresArrayPath));
  const auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32ArrayType>(k_CrystalStructuresArrayPath);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(featurePhasesArrayRef.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }

  MergeTwinsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(MergeTwinsFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(contiguousNeighborPath));
  args.insertOrAssign(MergeTwinsFilter::k_AxisTolerance_Key, std::make_any<float32>(3.0F));
  args.insertOrAssign(MergeTwinsFilter::k_AngleTolerance_Key, std::make_any<float32>(2.0F));
  args.insertOrAssign(MergeTwinsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
  args.insertOrAssign(MergeTwinsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
  args.insertOrAssign(MergeTwinsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(MergeTwinsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(MergeTwinsFilter::k_CellParentIdsArrayName_Key, std::make_any<std::string>("Bounds ParentIds"));
  args.insertOrAssign(MergeTwinsFilter::k_CreatedFeatureAttributeMatrixName_Key, std::make_any<std::string>("Bounds Parent Data"));
  args.insertOrAssign(MergeTwinsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<std::string>("Bounds ParentIds"));
  args.insertOrAssign(MergeTwinsFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(MergeTwinsFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(MergeTwinsFilter::k_SeedValue_Key, std::make_any<uint64>(5349));
  args.insertOrAssign(MergeTwinsFilter::k_SeedArrayName_Key, std::make_any<std::string>("Bounds SeedValue"));
  args.insertOrAssign(MergeTwinsFilter::k_RandomizeParentIds_Key, std::make_any<bool>(false));

  SECTION("Participating Phase returns an error")
  {
    auto& featurePhasesStoreRef = featurePhasesArrayRef.getDataStoreRef();
    for(usize featureIdx = 1; featureIdx < featurePhasesStoreRef.getNumberOfTuples(); featureIdx++)
    {
      featurePhasesStoreRef[featureIdx] = static_cast<int32>(crystalStructuresArrayRef.getNumberOfTuples());
    }
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -23502);
  }

  SECTION("Nonpositive Phases are ignored")
  {
    auto& featurePhasesStoreRef = featurePhasesArrayRef.getDataStoreRef();
    for(usize featureIdx = 1; featureIdx < featurePhasesStoreRef.getNumberOfTuples(); featureIdx++)
    {
      featurePhasesStoreRef[featureIdx] = -1;
    }
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::MergeTwinsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][MergeTwinsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "MergeTwinsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "MergeTwinsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<MergeTwinsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
