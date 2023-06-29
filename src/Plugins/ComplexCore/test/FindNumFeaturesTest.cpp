#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindNumFeaturesFilter.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

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

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_volume_fraction_feature_count.dream3d", unit_test::k_TestFilesDir));
} // namespace

TEST_CASE("ComplexCore::FindNumFeaturesFilter: Valid filter execution", "[ComplexCore][FindNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindNumFeaturesFilter filter;
  Arguments args;

  const std::string kDataInputArchive = "6_6_volume_fraction_feature_count.dream3d.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_volume_fraction_feature_count.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(FindNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(FindNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX.getParent()));
  args.insertOrAssign(FindNumFeaturesFilter::k_NumFeaturesArrayPath_Key, std::make_any<std::string>(k_FeatureCountsNX));

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

TEST_CASE("ComplexCore::FindNumFeaturesFilter: InValid filter execution", "[ComplexCore][FindNumFeaturesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindNumFeaturesFilter filter;
  Arguments args;

  const std::string kDataInputArchive = "6_6_volume_fraction_feature_count.dream3d.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_volume_fraction_feature_count.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(FindNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_IncorrectFeaturePhasesPath));
  args.insertOrAssign(FindNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX.getParent()));
  args.insertOrAssign(FindNumFeaturesFilter::k_NumFeaturesArrayPath_Key, std::make_any<std::string>(k_FeatureCountsNX));

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
