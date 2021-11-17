/**
 * This file is auto generated from the original DREAM3DReview/ImportQMMeltpoolH5File
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
 * When you start working on this unit test remove "[ImportQMMeltpoolH5File][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/MultiInputFileFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/ImportQMMeltpoolH5File.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::ImportQMMeltpoolH5File: Instantiation and Parameter Check", "[DREAM3DReview][ImportQMMeltpoolH5File][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportQMMeltpoolH5File filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insert(ImportQMMeltpoolH5File::k_InputFiles_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(ImportQMMeltpoolH5File::k_SliceRange_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(2)));
  args.insert(ImportQMMeltpoolH5File::k_DataContainerPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(ImportQMMeltpoolH5File::k_VertexAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(ImportQMMeltpoolH5File::k_Power_Key, std::make_any<float32>(1.23345f));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::ImportQMMeltpoolH5File: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::ImportQMMeltpoolH5File: InValid filter execution")
//{
//
//}
