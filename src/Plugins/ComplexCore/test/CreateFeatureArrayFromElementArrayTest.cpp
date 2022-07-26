/**
 * This file is auto generated from the original Core/CreateFeatureArrayFromElementArray
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
 * When you start working on this unit test remove "[CreateFeatureArrayFromElementArray][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/Common/TypesUtility.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateFeatureArrayFromElementArray.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const DataPath k_CellArrayPath({"Cell Array"});
const DataPath k_FeatureIDsPath({"Feature IDs"});
const DataPath k_CreatedFeatureArrayPath({"Created Array"});
const DataPath k_FeatureArrayExemplaryPath({"Exemplary Array"});
const std::string k_FeatureIdsFileName = "FeatureIds.raw";

template <typename T>
void testElementArray(const std::string& elementArrayFileName, uint64 compCount, const std::string& exemplaryFileName)
{
  DataStructure ds;

  // Load binary files
  {
    Arguments args;

    NumericType elementArrayType = GetNumericType<T>();

    RawBinaryReaderFilter rbrFilter;
    args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(elementArrayFileName));
    args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(compCount));
    args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, elementArrayType);
    args.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
    args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_CellArrayPath));

    auto result = rbrFilter.execute(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);

    args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
    args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
    args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
    args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

    result = rbrFilter.execute(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);

    args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(exemplaryFileName));
    args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(compCount));
    args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, elementArrayType);
    args.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{796}}, {}, {}}));
    args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureArrayExemplaryPath));

    result = rbrFilter.execute(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);
  }

  CreateFeatureArrayFromElementArray filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateFeatureArrayFromElementArray::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(k_CellArrayPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArray::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArray::k_CreatedArrayName_Key, std::make_any<DataPath>(k_CreatedFeatureArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_FeatureArrayExemplaryPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_CreatedFeatureArrayPath));

  const DataArray<T>& featureArrayExemplary = ds.getDataRefAs<DataArray<T>>(k_FeatureArrayExemplaryPath);
  const DataArray<T>& createdFeatureArray = ds.getDataRefAs<DataArray<T>>(k_CreatedFeatureArrayPath);
  REQUIRE(createdFeatureArray.getSize() == featureArrayExemplary.getSize());

  for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
  {
    REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
  }
}
} // namespace

TEST_CASE("ComplexCore::CreateFeatureArrayFromElementArray: Instantiation and Parameter Check", "[Core][CreateFeatureArrayFromElementArray]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateFeatureArrayFromElementArray filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateFeatureArrayFromElementArray::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateFeatureArrayFromElementArray::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateFeatureArrayFromElementArray::k_CreatedArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
}

TEST_CASE("ComplexCore::CreateFeatureArrayFromElementArray: Valid filter execution - 1 Component")
{
  testElementArray<float32>("ConfidenceIndex.raw", 1, "ConfidenceIndex_FeatureArray.raw");
}

TEST_CASE("ComplexCore::CreateFeatureArrayFromElementArray: Valid filter execution - 3 Component")
{
  testElementArray<uint8>("IPFColors.raw", 3, "IPFColors_FeatureArray.raw");
}
