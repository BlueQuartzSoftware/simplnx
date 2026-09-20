#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/ComputeBoundaryStrengthsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
DataPath faceDataPath = DataPath({Constants::k_TriangleDataContainerName, Constants::k_FaceData});
DataPath faceLabelsPath = faceDataPath.createChildPath(Constants::k_FaceLabels);
DataPath avgQuatsPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
DataPath featurePhasesPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

const std::string k_f1s = "F1List_COMPUTED";
const std::string k_f1spts = "F1sptList_COMPUTED";
const std::string k_f7s = "F7List_COMPUTED";
const std::string k_mPrimes = "mPrimeList_COMPUTED";
} // namespace

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz",
                                                              "feature_boundary_neighbor_slip_transmission");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission_1/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeBoundaryStrengthsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0, 0, 1}));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key, std::make_any<std::string>(k_f1s));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key, std::make_any<std::string>(k_f1spts));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key, std::make_any<std::string>(k_f7s));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key, std::make_any<std::string>(k_mPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_ComputeBoundaryStrengthsFilter.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1s"), faceDataPath.createChildPath(k_f1s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1spts"), faceDataPath.createChildPath(k_f1spts));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F7s"), faceDataPath.createChildPath(k_f7s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("mPrimes"), faceDataPath.createChildPath(k_mPrimes));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeBoundaryStrengthsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeBoundaryStrengthsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeBoundaryStrengthsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (DoubleVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 invalidPhaseIdx = GENERATE(-1, 2);
  CAPTURE(invalidPhaseIdx);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  DataStructure dataStructure;
  auto faceLabelsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"FaceLabels"}), {1}, {2});
  auto* faceLabelsArrayPtr = Int32Array::Create(dataStructure, "FaceLabels", faceLabelsStore);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(faceLabelsArrayPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  (*faceLabelsArrayPtr)[0] = 1;
  (*faceLabelsArrayPtr)[1] = 2;

  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, DataPath({"AvgQuats"}), {3}, {4});
  auto* avgQuatsArrayPtr = Float32Array::Create(dataStructure, "AvgQuats", avgQuatsStore);
  avgQuatsArrayPtr->fill(0.0F);
  (*avgQuatsArrayPtr)[7] = 1.0F;
  (*avgQuatsArrayPtr)[11] = 1.0F;

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"FeaturePhases"}), {3}, {1});
  auto* featurePhasesArrayPtr = Int32Array::Create(dataStructure, "FeaturePhases", featurePhasesStore);
  featurePhasesArrayPtr->fill(0);
  (*featurePhasesArrayPtr)[1] = 1;
  (*featurePhasesArrayPtr)[2] = 1;

  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, DataPath({"CrystalStructures"}), {2}, {1});
  auto* crystalStructuresArrayPtr = UInt32Array::Create(dataStructure, "CrystalStructures", crystalStructuresStore);
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Cubic_High;

  ComputeBoundaryStrengthsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0.0, 0.0, 1.0}));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"FaceLabels"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"AvgQuats"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"FeaturePhases"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));

  SECTION("Participating Phase returns an error")
  {
    (*featurePhasesArrayPtr)[2] = invalidPhaseIdx;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94740);
  }

  SECTION("Participating Laue index returns an error")
  {
    (*crystalStructuresArrayPtr)[1] = 999U;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94741);
  }

  SECTION("External Face is ignored")
  {
    (*faceLabelsArrayPtr)[1] = -1;
    (*featurePhasesArrayPtr)[1] = invalidPhaseIdx;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
