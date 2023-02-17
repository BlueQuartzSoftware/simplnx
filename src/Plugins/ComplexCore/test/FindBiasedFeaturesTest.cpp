#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindBiasedFeaturesFilter.hpp"

using namespace complex;

namespace
{
const std::string k_ComputedBiasedFeaturesName = "ComputedBiasedFeatures";
const DataPath k_GeometryPath({Constants::k_SmallIN100});
const DataPath k_CentroidsPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Centroids});
const DataPath k_SurfaceFeaturesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_SurfaceFeatures});
const DataPath k_PhasesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
const DataPath k_ExemplarBiasedFeaturesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, "BiasedFeatures"});
const DataPath k_ComputedBiasedFeaturesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, k_ComputedBiasedFeaturesName});
const DataPath k_WrongSurfaceFeaturePath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
} // namespace

TEST_CASE("ComplexCore::FindBiasedFeaturesFilter: Valid filter execution", "[FindBiasedFeaturesFilter]")
{
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_bounding_box_features.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindBiasedFeaturesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_ComputedBiasedFeaturesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<bool>(dataStructure, k_ExemplarBiasedFeaturesPath, k_ComputedBiasedFeaturesPath);
}

TEST_CASE("ComplexCore::FindBiasedFeaturesFilter: Invalid filter execution", "[FindBiasedFeaturesFilter]")
{
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_bounding_box_features.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindBiasedFeaturesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_WrongSurfaceFeaturePath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(FindBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_ComputedBiasedFeaturesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
