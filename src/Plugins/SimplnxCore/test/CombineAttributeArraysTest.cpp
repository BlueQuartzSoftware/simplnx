#include "SimplnxCore/Filters/CombineAttributeArraysFilter.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
const std::string k_Array1("Array_1");
const std::string k_Array2("Array_2");
const std::string k_Array3("Array_3");
const std::string k_InvalidArrayType("Invalid_Array_Type");
const DataPath k_OutputArrayPath({"OutputArray"});
} // namespace

template <typename T>
DataStructure CreateTestDataStructure()
{
  using DataArrayType = DataArray<T>;

  DataStructure dataStructure;

  DataArrayType* array1 = nx::core::UnitTest::CreateTestDataArray<T>(dataStructure, k_Array1, {10ULL, 10ULL}, {1}, 0);
  array1->fill(static_cast<T>(1));
  DataArrayType* array2 = nx::core::UnitTest::CreateTestDataArray<T>(dataStructure, k_Array2, {10ULL, 10ULL}, {2}, 0);
  array2->fill(static_cast<T>(2));
  DataArrayType* array3 = nx::core::UnitTest::CreateTestDataArray<T>(dataStructure, k_Array3, {10ULL, 10ULL}, {3}, 0);
  array3->fill(static_cast<T>(3));

  return dataStructure;
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Parameter Check", "[SimplnxCore][CombineAttributeArrays]")
{

  // Instantiate the filter, a DataStructure object and an Arguments Object

  DataStructure dataStructure = CreateTestDataStructure<uint8_t>();
  Arguments args;
  CombineAttributeArraysFilter filter;

  MultiArraySelectionParameter::ValueType inputArrays = {DataPath({k_Array1}), DataPath({k_Array2}), DataPath({k_Array3})};

  // Create default Parameters for the filter.
  args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
  args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Invalid because no arrays are selected
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Invalid because no output array is set
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(""));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Invalid because mismatch array types
  Float32Array* array3 = nx::core::UnitTest::CreateTestDataArray<float32>(dataStructure, k_InvalidArrayType, {10ULL, 10ULL}, {3}, 0);
  array3->fill(static_cast<float32>(3));
  inputArrays.push_back(DataPath({k_InvalidArrayType}));
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Invalid because mismatch tuple shapes
  dataStructure = CreateTestDataStructure<uint8>();                                                                              // Reset the DataStructure
  UInt8Array* array4 = nx::core::UnitTest::CreateTestDataArray<uint8>(dataStructure, k_InvalidArrayType, {5ULL, 55ULL}, {3}, 0); // Non-matching tuple shape
  array4->fill(static_cast<uint8>(3));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

template <typename T>
Result<> ValidateFilterOutput(const DataStructure& dataStructure)
{
  using DataArrayType = DataArray<T>;

  auto& output = dataStructure.template getDataRefAs<DataArrayType>(DataPath({k_OutputArrayPath}));
  size_t numTuples = output.getNumberOfTuples();
  REQUIRE(numTuples == 100);

  size_t numOutputComps = output.getNumberOfComponents();
  REQUIRE(numOutputComps == 6);

  MultiArraySelectionParameter::ValueType inputDataPaths = {DataPath({k_Array1}), DataPath({k_Array2}), DataPath({k_Array3})};
  size_t compOffset = 0;
  for(const auto& inputDataPath : inputDataPaths)
  {
    auto& inputArray = dataStructure.template getDataRefAs<DataArrayType>(inputDataPath);
    size_t numInputComps = inputArray.getNumberOfComponents();
    for(size_t tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
    {
      for(size_t compIndex = 0; compIndex < numInputComps; compIndex++)
      {
        REQUIRE(output[tupleIndex * numOutputComps + compIndex + compOffset] == inputArray[tupleIndex * numInputComps + compIndex]);
      }
    }
    compOffset += numInputComps;
  }

  return {};
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Algorithm Validation", "[SimplnxCore][CombineAttributeArrays]")
{
  MultiArraySelectionParameter::ValueType inputArrays = {DataPath({k_Array1}), DataPath({k_Array2}), DataPath({k_Array3})};

  SECTION("UINT8")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint8_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint8_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("INT8")
  {
    DataStructure dataStructure = CreateTestDataStructure<int8_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int8_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("UINT16")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint16_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint16_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("INT16")
  {
    DataStructure dataStructure = CreateTestDataStructure<int16_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int16_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("UINT32")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint32_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint32_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("INT32")
  {
    DataStructure dataStructure = CreateTestDataStructure<int32_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int32_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("UINT64")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint64_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint64_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("INT64")
  {
    DataStructure dataStructure = CreateTestDataStructure<int64_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int64_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("FLOAT32")
  {
    DataStructure dataStructure = CreateTestDataStructure<float32>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<float32>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("DOUBLE")
  {
    DataStructure dataStructure = CreateTestDataStructure<double>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<float64>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }

  SECTION("BOOL")
  {
    DataStructure dataStructure = CreateTestDataStructure<bool>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<bool>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)
  }
}
