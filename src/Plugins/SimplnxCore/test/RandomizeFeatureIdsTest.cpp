#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/RandomizeFeatureIdsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
} // namespace

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: Baseline Test", "[SimplnxCore][RandomizeFeatureIdsFilter]")
{
  DataStructure dataStructure;

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RandomizeFeatureIdsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>());
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>());

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
}
