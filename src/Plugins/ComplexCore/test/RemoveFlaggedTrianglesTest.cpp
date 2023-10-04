#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
 fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_remove_flagged_triangles/6_6_remove_flagged_triangles.dream3d", complex::unit_test::k_TestFilesDir));
}

TEST_CASE("ComplexCore::RemoveFlaggedTrianglesFilter: Valid Filter Execution", "[ComplexCore][RemoveFlaggedTrianglesFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_remove_flagged_triangles.tar.gz",
                                                             "6_6_remove_flagged_triangles.dream3d");

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
