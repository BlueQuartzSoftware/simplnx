#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindBoundaryElementFractionsFilter.hpp"

using namespace complex;

namespace
{
const std::string k_BCFName = "Boundary Cell Fractions";

const DataPath k_FeatureDataAMPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data});

const DataPath k_ExemplarBCFPath = k_FeatureDataAMPath.createChildPath(" Surface Element Fractions");
const DataPath k_GeneratedBCFPath = k_FeatureDataAMPath.createChildPath(k_BCFName);
} // namespace

TEST_CASE("ComplexCore::FindBoundaryElementFractionsFilter: Valid Filter Execution", "[ComplexCore][FindBoundaryElementFractionsFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_find_feature_boundary_element_fractions.tar.gz",
                                                             "6_6_find_feature_boundary_element_fractions");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_boundary_element_fractions/6_6_find_feature_boundary_element_fractions.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindBoundaryElementFractionsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key,
                        std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BoundaryCells"})));
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_FeatureDataAMPath_Key, std::make_any<DataPath>(k_FeatureDataAMPath));
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key, std::make_any<std::string>(::k_BCFName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarBCFPath, k_GeneratedBCFPath);
}
