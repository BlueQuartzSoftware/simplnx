#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "OrientationAnalysis/Filters/ComputeSlipTransmissionMetricsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include <array>

using namespace nx::core;
using namespace nx::core::unit_test;
namespace fs = std::filesystem;

namespace
{
DataPath GrainDataPath = DataPath({constants::k_SmallIN100, constants::k_GrainDataWithSpaces});
DataPath NeighborListPath = GrainDataPath.createChildPath("NeighborList");
DataPath AvgQuatsPath = DataPath({constants::k_SmallIN100, constants::k_GrainDataWithSpaces, constants::k_AvgQuats});
DataPath FeaturePhasesPath = DataPath({constants::k_SmallIN100, constants::k_GrainDataWithSpaces, constants::k_Phases});
DataPath CrystalStructuresPath = DataPath({constants::k_SmallIN100, constants::k_PhaseData, constants::k_CrystalStructures});

const std::string k_F1s = "F1List_COMPUTED";
const std::string k_F1spts = "F1sptList_COMPUTED";
const std::string k_F7s = "F7List_COMPUTED";
const std::string k_MPrimes = "mPrimeList_COMPUTED";
} // namespace

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz",
                                                               "feature_boundary_neighbor_slip_transmission_1");

  DataStructure dataStructure =
      unit_test::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission_1/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeSlipTransmissionMetricsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(NeighborListPath));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(AvgQuatsPath));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(FeaturePhasesPath));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(CrystalStructuresPath));

    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1ListArrayName_Key, std::make_any<std::string>(k_F1s));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key, std::make_any<std::string>(k_F1spts));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F7ListArrayName_Key, std::make_any<std::string>(k_F7s));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key, std::make_any<std::string>(k_MPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_ComputeSlipTransmissionMetricsFilter.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  unit_test::CompareNeighborLists<float32>(dataStructure, GrainDataPath.createChildPath("F1List"), GrainDataPath.createChildPath(k_F1s));
  unit_test::CompareNeighborLists<float32>(dataStructure, GrainDataPath.createChildPath("F1sptList"), GrainDataPath.createChildPath(k_F1spts));
  unit_test::CompareNeighborLists<float32>(dataStructure, GrainDataPath.createChildPath("F7List"), GrainDataPath.createChildPath(k_F7s));
  unit_test::CompareNeighborLists<float32>(dataStructure, GrainDataPath.createChildPath("mPrimeList"), GrainDataPath.createChildPath(k_MPrimes));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter]")
{
  unit_test::LoadPlugins();

  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz", "feature_boundary_neighbor_slip_transmission_1");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "feature_boundary_neighbor_slip_transmission_1" / "6_6_feature_boundary_neighbor_slip_transmission.dream3d";
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFile);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(FeaturePhasesPath));
  auto& featurePhasesArrayRef = dataStructure.getDataRefAs<Int32ArrayType>(FeaturePhasesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(CrystalStructuresPath));
  auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32ArrayType>(CrystalStructuresPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32NeighborListType>(NeighborListPath));
  auto& neighborList = dataStructure.getDataRefAs<Int32NeighborListType>(NeighborListPath);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(featurePhasesArrayRef.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }

  usize featureIdx = 1;
  while(featureIdx < neighborList.getNumberOfTuples() && neighborList[featureIdx].empty())
  {
    featureIdx++;
  }
  REQUIRE(featureIdx < neighborList.getNumberOfTuples());
  const int32 neighborFeatureIdx = neighborList[featureIdx][0];
  REQUIRE(neighborFeatureIdx > 0);
  REQUIRE(static_cast<usize>(neighborFeatureIdx) < featurePhasesArrayRef.getNumberOfTuples());

  ComputeSlipTransmissionMetricsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(NeighborListPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(AvgQuatsPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(FeaturePhasesPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(CrystalStructuresPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1ListArrayName_Key, std::make_any<std::string>(k_F1s));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key, std::make_any<std::string>(k_F1spts));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F7ListArrayName_Key, std::make_any<std::string>(k_F7s));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key, std::make_any<std::string>(k_MPrimes));

  SECTION("Participating Phase returns an error")
  {
    featurePhasesArrayRef.getDataStoreRef()[featureIdx] = static_cast<int32>(crystalStructuresArrayRef.getNumberOfTuples());
    featurePhasesArrayRef.getDataStoreRef()[neighborFeatureIdx] = 1;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94742);
  }

  SECTION("Participating Laue index returns an error")
  {
    featurePhasesArrayRef.getDataStoreRef()[featureIdx] = 1;
    featurePhasesArrayRef.getDataStoreRef()[neighborFeatureIdx] = 1;
    crystalStructuresArrayRef.getDataStoreRef()[1] = 999U;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94743);
  }

  SECTION("Phase outside an ineligible pair returns an error")
  {
    auto& featurePhasesStoreRef = featurePhasesArrayRef.getDataStoreRef();
    for(usize otherFeatureIdx = 0; otherFeatureIdx < featurePhasesStoreRef.getNumberOfTuples(); otherFeatureIdx++)
    {
      featurePhasesStoreRef[otherFeatureIdx] = 2;
    }
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94742);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeSlipTransmissionMetricsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeSlipTransmissionMetricsFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeSlipTransmissionMetricsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<DataPath>(ComputeSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeSlipTransmissionMetricsFilter::k_F1ListArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSlipTransmissionMetricsFilter::k_F7ListArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: Cubic eligibility uses crystal structure rather than phase index", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter]")
{
  const usize fixture = GENERATE(0, 1, 2);
  CAPTURE(fixture);
  DataStructure dataStructure;
  const DataPath featuresPath({"Features"});
  const DataPath ensemblesPath({"Ensembles"});
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {3});
  auto* ensemblesPtr = AttributeMatrix::Create(dataStructure, "Ensembles", {3});
  auto* phasesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {3}, {1}, featuresPtr->getId());
  (*phasesPtr)[0] = 0;
  (*phasesPtr)[1] = fixture == 0 ? 2 : 1;
  (*phasesPtr)[2] = fixture == 2 ? 1 : 2;
  auto* structuresPtr = UInt32ArrayType::CreateWithStore<DataStore<uint32>>(dataStructure, "Structures", {3}, {1}, ensemblesPtr->getId());
  // Laue indices 0 and 1 denote Hexagonal_High and Cubic_High, respectively.
  (*structuresPtr)[0] = 999;
  (*structuresPtr)[1] = fixture == 1 ? 1 : 0;
  (*structuresPtr)[2] = 1;
  auto* quatsPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", {3}, {4}, featuresPtr->getId());
  quatsPtr->fill(0);
  (*quatsPtr)[7] = 1;
  (*quatsPtr)[11] = 1;
  auto* neighborsPtr = Int32NeighborListType::Create(dataStructure, "Neighbors", ShapeType{3}, featuresPtr->getId());
  neighborsPtr->addEntry(1, 2);
  neighborsPtr->addEntry(2, 1);
  ComputeSlipTransmissionMetricsFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(featuresPath.createChildPath("Neighbors")));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(featuresPath.createChildPath("Quats")));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featuresPath.createChildPath("Phases")));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ensemblesPath.createChildPath("Structures")));
  const std::array<std::string, 4> names = {"mPrime", "F1", "F1spt", "F7"};
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key, std::make_any<std::string>(names[0]));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1ListArrayName_Key, std::make_any<std::string>(names[1]));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key, std::make_any<std::string>(names[2]));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F7ListArrayName_Key, std::make_any<std::string>(names[3]));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(fixture == 2)
  {
    REQUIRE(result.result.warnings().size() == 1);
    CHECK(result.result.warnings()[0].code == -94739);
  }
  else
  {
    CHECK(result.result.warnings().empty());
  }
  // Identical FCC systems give m'=1. Schmid=1/sqrt(6); direction and plane alignment sums each equal 6.
  // Thus F1=sqrt(3), F1spt=6*sqrt(3), F7=3. Unsupported hexagonal pairs give zero.
  const std::array<float32, 4> expected = {1.0F, 1.7320508075688772F, 10.392304845413264F, 3.0F};
  for(usize metric = 0; metric < names.size(); metric++)
  {
    const auto* outputPtr = dataStructure.getDataAs<Float32NeighborListType>(featuresPath.createChildPath(names[metric]));
    REQUIRE(outputPtr != nullptr);
    for(usize feature = 1; feature <= 2; feature++)
    {
      CAPTURE(metric, feature);
      REQUIRE(outputPtr->getList(feature).size() == 1);
      CHECK(outputPtr->getList(feature)[0] == Approx(fixture == 2 ? 0.0F : expected[metric]).margin(1.0e-5F));
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
