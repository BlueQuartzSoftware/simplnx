#include "SimplnxCore/Filters/Algorithms/CombineAttributeArrays.hpp"
#include "SimplnxCore/Filters/CombineAttributeArraysFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_Array1("Array_1");
const std::string k_Array2("Array_2");
const std::string k_Array3("Array_3");
const std::string k_InvalidArrayType("Invalid_Array_Type");
const DataPath k_OutputArrayPath({"OutputArray"});

template <typename T>
class CombineFailOnLaterWriteStore : public DataStore<T>
{
public:
  CombineFailOnLaterWriteStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> value, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, value)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize offset, nonstd::span<const T> buffer) override
  {
    if(++m_WriteCount == 2)
    {
      return MakeErrorResult(m_ErrorCode, "Injected CombineAttributeArrays later-page write failure");
    }
    return DataStore<T>::copyFromBuffer(offset, buffer);
  }

  usize getWriteCount() const
  {
    return m_WriteCount;
  }

private:
  int32 m_ErrorCode;
  usize m_WriteCount = 0;
};
} // namespace

template <typename T>
static DataStructure CreateTestDataStructure()
{
  using DataArrayType = DataArray<T>;

  DataStructure dataStructure;

  DataArrayType* array1Ptr = nx::core::unit_test::CreateTestDataArray<T>(dataStructure, k_Array1, {10ULL, 10ULL}, {1}, 0);
  array1Ptr->fill(static_cast<T>(1));
  DataArrayType* array2Ptr = nx::core::unit_test::CreateTestDataArray<T>(dataStructure, k_Array2, {10ULL, 10ULL}, {2}, 0);
  array2Ptr->fill(static_cast<T>(2));
  DataArrayType* array3Ptr = nx::core::unit_test::CreateTestDataArray<T>(dataStructure, k_Array3, {10ULL, 10ULL}, {3}, 0);
  array3Ptr->fill(static_cast<T>(3));

  return dataStructure;
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Parameter Check", "[SimplnxCore][CombineAttributeArrays]")
{
  unit_test::LoadPlugins();

  // Configure the filter arguments.

  DataStructure dataStructure = CreateTestDataStructure<uint8_t>();
  Arguments args;
  CombineAttributeArraysFilter filter;

  MultiArraySelectionParameter::ValueType inputArrays = {DataPath({k_Array1}), DataPath({k_Array2}), DataPath({k_Array3})};

  args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
  args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Invalid because no arrays are selected
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);

  // Invalid because no output array is set
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(""));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);

  // Invalid because mismatch array types
  Float32ArrayType* array3Ptr = nx::core::unit_test::CreateTestDataArray<float32>(dataStructure, k_InvalidArrayType, {10ULL, 10ULL}, {3}, 0);
  array3Ptr->fill(static_cast<float32>(3));
  inputArrays.push_back(DataPath({k_InvalidArrayType}));
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);

  // Invalid because mismatch tuple shapes
  dataStructure = CreateTestDataStructure<uint8>();                                                                                      // Reset the DataStructure
  UInt8ArrayType* array4Ptr = nx::core::unit_test::CreateTestDataArray<uint8>(dataStructure, k_InvalidArrayType, {5ULL, 55ULL}, {3}, 0); // Non-matching tuple shape
  array4Ptr->fill(static_cast<uint8>(3));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

template <typename T>
static Result<> ValidateFilterOutput(const DataStructure& dataStructure)
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
  unit_test::LoadPlugins();

  MultiArraySelectionParameter::ValueType inputArrays = {DataPath({k_Array1}), DataPath({k_Array2}), DataPath({k_Array3})};

  SECTION("UINT8")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint8_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint8_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("INT8")
  {
    DataStructure dataStructure = CreateTestDataStructure<int8_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int8_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("UINT16")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint16_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint16_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("INT16")
  {
    DataStructure dataStructure = CreateTestDataStructure<int16_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int16_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("UINT32")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint32_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint32_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("INT32")
  {
    DataStructure dataStructure = CreateTestDataStructure<int32_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int32_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("UINT64")
  {
    DataStructure dataStructure = CreateTestDataStructure<uint64_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint64_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("INT64")
  {
    DataStructure dataStructure = CreateTestDataStructure<int64_t>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int64_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("FLOAT32")
  {
    DataStructure dataStructure = CreateTestDataStructure<float32>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<float32>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("DOUBLE")
  {
    DataStructure dataStructure = CreateTestDataStructure<double>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<float64>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("BOOL")
  {
    DataStructure dataStructure = CreateTestDataStructure<bool>();
    Arguments args;
    CombineAttributeArraysFilter filter;

    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputArrays));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<bool>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Normalization", "[SimplnxCore][CombineAttributeArrays]")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  auto* array1Ptr = unit_test::CreateTestDataArray<float32>(dataStructure, k_Array1, {3}, {1}, 0.0F);
  auto* array2Ptr = unit_test::CreateTestDataArray<float32>(dataStructure, k_Array2, {3}, {2}, 0.0F);

  (*array1Ptr)[0] = -2.0F;
  (*array1Ptr)[1] = 0.0F;
  (*array1Ptr)[2] = 2.0F;
  (*array2Ptr)[0] = 5.0F;
  (*array2Ptr)[1] = 10.0F;
  (*array2Ptr)[2] = 7.0F;
  (*array2Ptr)[3] = 10.0F;
  (*array2Ptr)[4] = 9.0F;
  (*array2Ptr)[5] = 10.0F;

  Arguments args;
  args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(true));
  args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath({k_Array1}), DataPath({k_Array2})}));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

  CombineAttributeArraysFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  Float32ArrayType* outputPtr = nullptr;
  REQUIRE_NOTHROW(outputPtr = &dataStructure.getDataRefAs<Float32ArrayType>(k_OutputArrayPath));
  const std::array<float32, 9> expected = {0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 0.0F, 1.0F, 1.0F, 0.0F};
  REQUIRE(outputPtr->getSize() == expected.size());
  for(usize index = 0; index < expected.size(); index++)
  {
    REQUIRE((*outputPtr)[index] == expected[index]);
  }
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Bulk boundary and write failure propagation", "[SimplnxCore][CombineAttributeArrays]")
{
  unit_test::LoadPlugins();
  constexpr usize k_TupleCount = 21847;
  const DataPath firstPath({"First"});
  const DataPath secondPath({"Second"});
  const DataPath outputPath({"Combined"});

  SECTION("global extrema, constant component, and final partial page")
  {
    DataStructure dataStructure;
    auto* firstPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, firstPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{1});
    auto* secondPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, secondPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{2});
    REQUIRE(firstPtr != nullptr);
    REQUIRE(secondPtr != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_TupleCount; tupleIdx++)
    {
      (*firstPtr)[tupleIdx] = static_cast<float32>(tupleIdx) - 100.0F;
      (*secondPtr)[tupleIdx * 2] = static_cast<float32>(tupleIdx * 2) + 11.0F;
      (*secondPtr)[tupleIdx * 2 + 1] = 17.0F;
    }

    CombineAttributeArraysFilter filter;
    Arguments args;
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{firstPath, secondPath}));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputPath.getTargetName()));

    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(outputPath));
    const auto& output = dataStructure.getDataRefAs<Float32ArrayType>(outputPath);
    REQUIRE(output.getNumberOfTuples() == k_TupleCount);
    REQUIRE(output.getNumberOfComponents() == 3);
    for(usize tupleIdx : {usize{0}, usize{21844}, usize{21845}, k_TupleCount - 1})
    {
      REQUIRE(output[tupleIdx * 3] == Approx(static_cast<float32>(tupleIdx) / static_cast<float32>(k_TupleCount - 1)));
      REQUIRE(output[tupleIdx * 3 + 1] == Approx(static_cast<float32>(tupleIdx) / static_cast<float32>(k_TupleCount - 1)));
      REQUIRE(output[tupleIdx * 3 + 2] == 0.0F);
    }
    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("bool values keep their component order without normalization")
  {
    DataStructure dataStructure;
    auto* firstPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, firstPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{1});
    auto* secondPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, secondPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{2});
    REQUIRE(firstPtr != nullptr);
    REQUIRE(secondPtr != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_TupleCount; tupleIdx++)
    {
      (*firstPtr)[tupleIdx] = tupleIdx % 2 == 0;
      (*secondPtr)[tupleIdx * 2] = tupleIdx % 3 == 0;
      (*secondPtr)[tupleIdx * 2 + 1] = tupleIdx % 5 == 0;
    }

    CombineAttributeArraysFilter filter;
    Arguments args;
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{firstPath, secondPath}));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputPath.getTargetName()));

    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    const auto& output = dataStructure.getDataRefAs<BoolArrayType>(outputPath);
    for(usize tupleIdx : {usize{0}, usize{21844}, usize{21845}, k_TupleCount - 1})
    {
      REQUIRE(output[tupleIdx * 3] == (tupleIdx % 2 == 0));
      REQUIRE(output[tupleIdx * 3 + 1] == (tupleIdx % 3 == 0));
      REQUIRE(output[tupleIdx * 3 + 2] == (tupleIdx % 5 == 0));
    }
  }

  SECTION("second output page failure is returned and no later page is attempted")
  {
    constexpr int32 k_WriteError = -91921;
    constexpr usize k_FailureTupleCount = 43691;
    constexpr usize k_PageTuples = 65536 / 3;
    constexpr int32 k_UntouchedValue = -77;
    DataStructure dataStructure;
    auto* firstPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, firstPath.getTargetName(), ShapeType{k_FailureTupleCount}, ShapeType{1});
    auto* secondPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, secondPath.getTargetName(), ShapeType{k_FailureTupleCount}, ShapeType{2});
    auto outputStore = std::make_shared<CombineFailOnLaterWriteStore<int32>>(ShapeType{k_FailureTupleCount}, ShapeType{3}, k_UntouchedValue, k_WriteError);
    auto* outputPtr = Int32ArrayType::Create(dataStructure, outputPath.getTargetName(), outputStore);
    REQUIRE(firstPtr != nullptr);
    REQUIRE(secondPtr != nullptr);
    REQUIRE(outputPtr != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_FailureTupleCount; ++tupleIdx)
    {
      (*firstPtr)[tupleIdx] = 1;
      (*secondPtr)[tupleIdx * 2] = 2;
      (*secondPtr)[tupleIdx * 2 + 1] = 3;
    }

    CombineAttributeArraysInputValues inputValues;
    inputValues.normalize_data = false;
    inputValues.selected_data_array_paths = {firstPath, secondPath};
    inputValues.stacked_data_array_path = outputPath;
    const std::atomic_bool shouldCancel = false;
    const auto executeResult = CombineAttributeArrays(dataStructure, {}, shouldCancel, &inputValues)();
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult)
    REQUIRE(executeResult.errors().front().code == k_WriteError);
    REQUIRE(outputStore->getWriteCount() == 2);
    REQUIRE((*outputPtr)[0] == 1);
    REQUIRE((*outputPtr)[1] == 2);
    REQUIRE((*outputPtr)[2] == 3);
    for(usize valueIdx = k_PageTuples * 3; valueIdx < outputPtr->getSize(); ++valueIdx)
    {
      REQUIRE((*outputPtr)[valueIdx] == k_UntouchedValue);
    }
  }
}

TEST_CASE("SimplnxCore::CombineAttributeArraysFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CombineAttributeArraysFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CombineAttributeArraysFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CombineAttributeArraysFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<CombineAttributeArraysFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(CombineAttributeArraysFilter::k_MoveValues_Key) == true);
      }
      CHECK(args.value<bool>(CombineAttributeArraysFilter::k_NormalizeData_Key) == true);
      // Successful pipeline loading verifies the MultiDataArraySelectionFilterParameterConverter value.
      CHECK(args.value<std::string>(CombineAttributeArraysFilter::k_StackedDataArrayName_Key) == "TestName");
    }
  }
}
