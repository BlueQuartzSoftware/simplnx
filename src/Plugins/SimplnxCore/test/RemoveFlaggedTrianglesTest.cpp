#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_Remove_Flagged_Triangles/6_6_remove_flagged_triangles.dream3d", unit_test::k_TestFilesDir));
}

TEST_CASE("SimplnxCore::RemoveFlaggedTrianglesFilter: Valid Filter Execution", "[SimplnxCore][RemoveFlaggedTrianglesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "6_6_Remove_Flagged_Triangles.tar.gz", "6_6_Remove_Flagged_Triangles.dream3d");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedTrianglesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_InputGeometry_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_OutputGeometry_Key, std::make_any<DataPath>(DataPath{}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
}
