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
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
DataPath grainDataPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data});
DataPath neighborListPath = grainDataPath.createChildPath("NeighborList");
DataPath avgQuatsPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
DataPath featurePhasesPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

const std::string k_f1s = "F1List_COMPUTED";
const std::string k_f1spts = "F1sptList_COMPUTED";
const std::string k_f7s = "F7List_COMPUTED";
const std::string k_mPrimes = "mPrimeList_COMPUTED";
} // namespace

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz",
                                                              "feature_boundary_neighbor_slip_transmission_1");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission_1/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeSlipTransmissionMetricsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(neighborListPath));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));

    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1ListArrayName_Key, std::make_any<std::string>(k_f1s));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key, std::make_any<std::string>(k_f1spts));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F7ListArrayName_Key, std::make_any<std::string>(k_f7s));
    args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key, std::make_any<std::string>(k_mPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_ComputeSlipTransmissionMetricsFilter.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("F1List"), grainDataPath.createChildPath(k_f1s));
  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("F1sptList"), grainDataPath.createChildPath(k_f1spts));
  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("F7List"), grainDataPath.createChildPath(k_f7s));
  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("mPrimeList"), grainDataPath.createChildPath(k_mPrimes));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter]")
{
  UnitTest::LoadPlugins();

  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz", "feature_boundary_neighbor_slip_transmission_1");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "feature_boundary_neighbor_slip_transmission_1" / "6_6_feature_boundary_neighbor_slip_transmission.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featurePhasesPath));
  auto& featurePhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath));
  auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32NeighborList>(neighborListPath));
  auto& neighborList = dataStructure.getDataRefAs<Int32NeighborList>(neighborListPath);
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
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(neighborListPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1ListArrayName_Key, std::make_any<std::string>(k_f1s));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key, std::make_any<std::string>(k_f1spts));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_F7ListArrayName_Key, std::make_any<std::string>(k_f7s));
  args.insertOrAssign(ComputeSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key, std::make_any<std::string>(k_mPrimes));

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeSlipTransmissionMetricsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeSlipTransmissionMetricsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeSlipTransmissionMetricsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeSlipTransmissionMetricsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeSlipTransmissionMetricsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
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
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {3});
  auto* ensembles = AttributeMatrix::Create(dataStructure, "Ensembles", {3});
  auto* phases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {3}, {1}, features->getId());
  (*phases)[0] = 0;
  (*phases)[1] = fixture == 0 ? 2 : 1;
  (*phases)[2] = fixture == 2 ? 1 : 2;
  auto* structures = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, "Structures", {3}, {1}, ensembles->getId());
  // Laue indices 0 and 1 denote Hexagonal_High and Cubic_High, respectively.
  (*structures)[0] = 999;
  (*structures)[1] = fixture == 1 ? 1 : 0;
  (*structures)[2] = 1;
  auto* quats = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", {3}, {4}, features->getId());
  quats->fill(0);
  (*quats)[7] = 1;
  (*quats)[11] = 1;
  auto* neighbors = Int32NeighborList::Create(dataStructure, "Neighbors", ShapeType{3}, features->getId());
  neighbors->addEntry(1, 2);
  neighbors->addEntry(2, 1);
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
    const auto* output = dataStructure.getDataAs<Float32NeighborList>(featuresPath.createChildPath(names[metric]));
    REQUIRE(output != nullptr);
    for(usize feature = 1; feature <= 2; feature++)
    {
      CAPTURE(metric, feature);
      REQUIRE(output->getList(feature).size() == 1);
      CHECK(output->getList(feature)[0] == Approx(fixture == 2 ? 0.0F : expected[metric]).margin(1.0e-5F));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
