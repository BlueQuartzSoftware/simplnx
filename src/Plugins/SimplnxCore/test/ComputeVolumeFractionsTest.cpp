#include "SimplnxCore/Filters/ComputeVolumeFractionsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_VolumeFractions("Volume Fractions");
const std::string k_VolumeFractionsNX("Volume Fractions NX");

const DataPath k_IncorrectCellPhasesPath({Constants::k_DataContainer, Constants::k_FeatureData, Constants::k_Phases});

const DataPath k_VolumeFractionsPath = Constants::k_CellEnsembleAttributeMatrixPath.createChildPath(k_VolumeFractions);
const DataPath k_VolumeFractionsPathNX = Constants::k_CellEnsembleAttributeMatrixPath.createChildPath(k_VolumeFractionsNX);

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_volume_fraction_feature_count.dream3d", unit_test::k_TestFilesDir));
} // namespace

TEST_CASE("SimplnxCore::ComputeVolumeFractionsFilter: Valid filter execution", "[SimplnxCore::ComputeVolumeFractionsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeVolumeFractionsFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz",
                                                              "6_6_volume_fraction_feature_count.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(Constants::k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_VolFractionsArrayName_Key, std::make_any<std::string>(k_VolumeFractionsNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& d3dVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPath);
  auto& nxVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPathNX);

  for(usize index = 0; index < d3dVolumeFractionsArrayRef.getSize(); index++)
  {
    float32 result = fabsf(d3dVolumeFractionsArrayRef[index] - nxVolumeFractionsArrayRef[index]);
    REQUIRE(result < UnitTest::EPSILON);
  }
}

TEST_CASE("SimplnxCore::ComputeVolumeFractionsFilter: InValid filter execution", "[SimplnxCore::ComputeVolumeFractionsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeVolumeFractionsFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz",
                                                              "6_6_volume_fraction_feature_count.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_volFractions_and_numFeatures_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_IncorrectCellPhasesPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(Constants::k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_VolFractionsArrayName_Key, std::make_any<std::string>(k_VolumeFractionsNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& d3dVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPath);
  auto& nxVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPathNX);

  for(usize index = 0; index < d3dVolumeFractionsArrayRef.getSize(); index++)
  {
    float32 result = fabsf(d3dVolumeFractionsArrayRef[index] - nxVolumeFractionsArrayRef[index]);
    REQUIRE(result > UnitTest::EPSILON);
  }
}
