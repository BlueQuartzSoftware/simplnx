#include "SimplnxCore/Filters/ComputeNumFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_FeatureCounts("Feature Count");
const std::string k_FeatureCountsNX("Feature Count NX");

const DataPath k_FeaturePhasesPath({Constants::k_DataContainer, Constants::k_FeatureData, Constants::k_Phases});
const DataPath k_IncorrectFeaturePhasesPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_Phases});

const DataPath k_FeatureCountsPath({Constants::k_DataContainer, Constants::k_CellEnsembleData, k_FeatureCounts});
const DataPath k_FeatureCountsPathNX({Constants::k_DataContainer, Constants::k_CellEnsembleData, k_FeatureCountsNX});

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_volume_fraction_feature_count.dream3d", unit_test::k_TestFilesDir));
} // namespace

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Valid filter execution", "[SimplnxCore][ComputeNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeNumFeaturesFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz",
                                                              "6_6_volume_fraction_feature_count.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX.getParent()));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_NumFeaturesArrayName_Key, std::make_any<std::string>(k_FeatureCountsNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& d3dFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPath);
  auto& nxFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPathNX);

  for(usize index = 0; index < d3dFeatureCountsArrayRef.getSize(); index++)
  {
    REQUIRE(d3dFeatureCountsArrayRef[index] == nxFeatureCountsArrayRef[index]);
  }
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: InValid filter execution", "[SimplnxCore][ComputeNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeNumFeaturesFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz",
                                                              "6_6_volume_fraction_feature_count.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_IncorrectFeaturePhasesPath));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX.getParent()));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_NumFeaturesArrayName_Key, std::make_any<std::string>(k_FeatureCountsNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& d3dFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPath);
  auto& nxFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPathNX);

  for(usize index = 1; index < d3dFeatureCountsArrayRef.getSize(); index++)
  {
    REQUIRE(d3dFeatureCountsArrayRef[index] != nxFeatureCountsArrayRef[index]);
  }
}
