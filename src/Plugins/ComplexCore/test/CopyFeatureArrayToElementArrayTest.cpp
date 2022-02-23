/**
 * This file is auto generated from the original Core/CopyFeatureArrayToElementArray
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
 * When you start working on this unit test remove "[CopyFeatureArrayToElementArray][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CopyFeatureArrayToElementArray.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const int k_EmptyArrayPath = -201;
const std::string k_FeaturePhasesFileName = "Feature_Phases.raw";
const std::string k_FeatureIdsFileName = "FeatureIds.raw";
const DataPath k_FeaturePhasesPath({"Feature Phases"});
const DataPath k_FeatureIDsPath({"Feature IDs"});
const DataPath k_CreatedArrayPath({"New Array"});

DataStructure createDataStructure()
{
  DataStructure ds;
  Arguments args;

  RawBinaryReaderFilter rbrFilter;
  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeaturePhasesFileName));
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  args.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{796}}, {}, {}}));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));

  auto result = rbrFilter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  args.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  return ds;
}
} // namespace

TEST_CASE("Core::CopyFeatureArrayToElementArray: Instantiation and Parameter Check", "[Core][CopyFeatureArrayToElementArray]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CopyFeatureArrayToElementArray filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_SelectedFeatureArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_CreatedArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 3);
  for(const Error& err : preflightResult.outputActions.errors())
  {
    REQUIRE(err.code == k_EmptyArrayPath);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().size() == 3);
  for(const Error& err : executeResult.result.errors())
  {
    REQUIRE(err.code == k_EmptyArrayPath);
  }
}

TEST_CASE("Core::CopyFeatureArrayToElementArray: Valid filter execution", "[Core][CopyFeatureArrayToElementArray]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CopyFeatureArrayToElementArray filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_SelectedFeatureArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_CreatedArrayName_Key, std::make_any<DataPath>(k_CreatedArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("Core::CopyFeatureArrayToElementArray: InValid filter execution", "[Core][CopyFeatureArrayToElementArray]")
//{
//
//}
