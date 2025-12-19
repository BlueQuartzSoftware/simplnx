/**
 * This file is auto generated from the original SimplnxCore/ReadNotesFileFilter
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[ReadNotesFileFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

// TODO: PARAMETER_INCLUDES
#include "SimplnxCore/Filters/ReadNotesFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::ReadNotesFileFilter: Valid Filter Execution", "[SimplnxCore][ReadNotesFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ReadNotesFileFilter.tar.gz", "ReadNotesFileFilter");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/ReadNotesFileFilter/ReadNotesFileFilter.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    Arguments args;
    ReadNotesFileFilter filter;

    // Create default Parameters for the filter.
    // This next line is an example, your filter may be different
    // args.insertOrAssign(ReadNotesFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/ReadNotesFileFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Compare exemplar data arrays with computed data arrays
  // TODO: Insert verification codes

  // This should be in every unit test. If you think it does not apply, please review with another engineer
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// TEST_CASE("SimplnxCore::ReadNotesFileFilter: InValid Filter Execution")
//{
//
// }
