#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindNumFeaturesFilter.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const std::string k_FeatureCounts("Feature Count");
const std::string k_FeatureCountsNX("Feature Count NX");

const DataPath k_FeaturePhasesPath({Constants::k_DataContainer, Constants::k_FeatureData, Constants::k_Phases});
const DataPath k_IncorrectFeaturePhasesPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_Phases});

const DataPath k_FeatureCountsPath({Constants::k_DataContainer, Constants::k_CellEnsembleData, k_FeatureCounts});
const DataPath k_FeatureCountsPathNX({Constants::k_DataContainer, Constants::k_CellEnsembleData, k_FeatureCountsNX});

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_volume_fraction_feature_count.dream3d", unit_test::k_DREAM3DDataDir));
} // namespace

TEST_CASE("ComplexCore::FindNumFeaturesFilter: Instantiation and Parameter Check", "[ComplexCore][FindNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindNumFeaturesFilter filter;
  Arguments args;

  DataStructure dataGraph = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(FindNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(FindNumFeaturesFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}

TEST_CASE("ComplexCore::FindNumFeaturesFilter: Valid filter execution", "[ComplexCore][FindNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindNumFeaturesFilter filter;
  Arguments args;

  DataStructure dataGraph = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(FindNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(FindNumFeaturesFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  auto& d3dFeatureCountsArrayRef = dataGraph.getDataRefAs<Int32Array>(k_FeatureCountsPath);
  auto& nxFeatureCountsArrayRef = dataGraph.getDataRefAs<Int32Array>(k_FeatureCountsPathNX);

  for(usize index = 0; index < d3dFeatureCountsArrayRef.getSize(); index++)
  {
    REQUIRE(d3dFeatureCountsArrayRef[index] == nxFeatureCountsArrayRef[index]);
  }
}

TEST_CASE("ComplexCore::FindNumFeaturesFilter: InValid filter execution", "[ComplexCore][FindNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindNumFeaturesFilter filter;
  Arguments args;

  DataStructure dataGraph = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(FindNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_IncorrectFeaturePhasesPath));
  args.insertOrAssign(FindNumFeaturesFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  auto& d3dFeatureCountsArrayRef = dataGraph.getDataRefAs<Int32Array>(k_FeatureCountsPath);
  auto& nxFeatureCountsArrayRef = dataGraph.getDataRefAs<Int32Array>(k_FeatureCountsPathNX);

  for(usize index = 1; index < d3dFeatureCountsArrayRef.getSize(); index++)
  {
    REQUIRE(d3dFeatureCountsArrayRef[index] != nxFeatureCountsArrayRef[index]);
  }
}
