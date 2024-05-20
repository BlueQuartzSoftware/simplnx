#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeFeaturePhasesBinaryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
const std::string k_BinaryFeaturePhasesName = "BinaryFeaturePhases";

const DataPath k_ExemplarArray = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BinaryPhases"});
const DataPath k_GeneratedArray = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_BinaryFeaturePhasesName});
} // namespace

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Valid Filter Execution", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "bin_feature_phases.tar.gz", "bin_feature_phases.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeaturePhasesBinaryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_Mask})));
    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CellDataAMPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData})));

    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_BinaryFeaturePhasesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, k_ExemplarArray, k_GeneratedArray);
}
