#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindFeaturePhasesBinaryFilter.hpp"

using namespace complex;

namespace
{
const std::string k_BinaryFeaturePhasesName = "BinaryFeaturePhases";

const DataPath k_ExemplarArray = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BinaryPhases"});
const DataPath k_GeneratedArray = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_BinaryFeaturePhasesName});
} // namespace

TEST_CASE("ComplexCore::FindFeaturePhasesBinaryFilter: Valid Filter Execution", "[ComplexCore][FindFeaturePhasesBinaryFilter]")
{
  const std::string kDataInputArchive = "bin_feature_phases.tar.gz";
  const std::string kExpectedOutputTopLevel = "bin_feature_phases.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindFeaturePhasesBinaryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_Mask})));
    args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_CellDataAMPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData})));

    args.insertOrAssign(FindFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_BinaryFeaturePhasesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, k_ExemplarArray, k_GeneratedArray);
}
