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
  UnitTest::LoadPlugins();

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  UnitTest::LoadPlugins();

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint8_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int8_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint16_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int16_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint32_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int32_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<uint64_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<int64_t>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<float32>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<float64>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    Result<> validationResult = ValidateFilterOutput<bool>(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(validationResult)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Normalization", "[SimplnxCore][CombineAttributeArrays]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* array1 = UnitTest::CreateTestDataArray<float32>(dataStructure, k_Array1, {3}, {1}, 0.0F);
  auto* array2 = UnitTest::CreateTestDataArray<float32>(dataStructure, k_Array2, {3}, {2}, 0.0F);

  (*array1)[0] = -2.0F;
  (*array1)[1] = 0.0F;
  (*array1)[2] = 2.0F;
  (*array2)[0] = 5.0F;
  (*array2)[1] = 10.0F;
  (*array2)[2] = 7.0F;
  (*array2)[3] = 10.0F;
  (*array2)[4] = 9.0F;
  (*array2)[5] = 10.0F;

  Arguments args;
  args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(true));
  args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
  args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath({k_Array1}), DataPath({k_Array2})}));
  args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_OutputArrayPath.getTargetName()));

  CombineAttributeArraysFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  Float32Array* output = nullptr;
  REQUIRE_NOTHROW(output = &dataStructure.getDataRefAs<Float32Array>(k_OutputArrayPath));
  const std::array<float32, 9> expected = {0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 0.0F, 1.0F, 1.0F, 0.0F};
  REQUIRE(output->getSize() == expected.size());
  for(usize index = 0; index < expected.size(); index++)
  {
    REQUIRE((*output)[index] == expected[index]);
  }
}

TEST_CASE("SimplnxCore::CombineAttributeArrays: Bulk boundary and write failure propagation", "[SimplnxCore][CombineAttributeArrays]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_TupleCount = 21847;
  const DataPath firstPath({"First"});
  const DataPath secondPath({"Second"});
  const DataPath outputPath({"Combined"});

  SECTION("global extrema, constant component, and final partial page")
  {
    DataStructure ds;
    auto* first = Float32Array::CreateWithStore<DataStore<float32>>(ds, firstPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{1});
    auto* second = Float32Array::CreateWithStore<DataStore<float32>>(ds, secondPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{2});
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_TupleCount; tupleIdx++)
    {
      (*first)[tupleIdx] = static_cast<float32>(tupleIdx) - 100.0F;
      (*second)[tupleIdx * 2] = static_cast<float32>(tupleIdx * 2) + 11.0F;
      (*second)[tupleIdx * 2 + 1] = 17.0F;
    }

    CombineAttributeArraysFilter filter;
    Arguments args;
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{firstPath, secondPath}));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputPath.getTargetName()));

    const auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(outputPath));
    const auto& output = ds.getDataRefAs<Float32Array>(outputPath);
    REQUIRE(output.getNumberOfTuples() == k_TupleCount);
    REQUIRE(output.getNumberOfComponents() == 3);
    for(usize tupleIdx : {usize{0}, usize{21844}, usize{21845}, k_TupleCount - 1})
    {
      REQUIRE(output[tupleIdx * 3] == Approx(static_cast<float32>(tupleIdx) / static_cast<float32>(k_TupleCount - 1)));
      REQUIRE(output[tupleIdx * 3 + 1] == Approx(static_cast<float32>(tupleIdx) / static_cast<float32>(k_TupleCount - 1)));
      REQUIRE(output[tupleIdx * 3 + 2] == 0.0F);
    }
    UnitTest::CheckArraysInheritTupleDims(ds);
  }

  SECTION("bool values keep their component order without normalization")
  {
    DataStructure ds;
    auto* first = BoolArray::CreateWithStore<DataStore<bool>>(ds, firstPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{1});
    auto* second = BoolArray::CreateWithStore<DataStore<bool>>(ds, secondPath.getTargetName(), ShapeType{k_TupleCount}, ShapeType{2});
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_TupleCount; tupleIdx++)
    {
      (*first)[tupleIdx] = tupleIdx % 2 == 0;
      (*second)[tupleIdx * 2] = tupleIdx % 3 == 0;
      (*second)[tupleIdx * 2 + 1] = tupleIdx % 5 == 0;
    }

    CombineAttributeArraysFilter filter;
    Arguments args;
    args.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
    args.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{firstPath, secondPath}));
    args.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputPath.getTargetName()));

    const auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    const auto& output = ds.getDataRefAs<BoolArray>(outputPath);
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
    DataStructure ds;
    auto* first = Int32Array::CreateWithStore<DataStore<int32>>(ds, firstPath.getTargetName(), ShapeType{k_FailureTupleCount}, ShapeType{1});
    auto* second = Int32Array::CreateWithStore<DataStore<int32>>(ds, secondPath.getTargetName(), ShapeType{k_FailureTupleCount}, ShapeType{2});
    auto outputStore = std::make_shared<CombineFailOnLaterWriteStore<int32>>(ShapeType{k_FailureTupleCount}, ShapeType{3}, k_UntouchedValue, k_WriteError);
    auto* output = Int32Array::Create(ds, outputPath.getTargetName(), outputStore);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(output != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_FailureTupleCount; ++tupleIdx)
    {
      (*first)[tupleIdx] = 1;
      (*second)[tupleIdx * 2] = 2;
      (*second)[tupleIdx * 2 + 1] = 3;
    }

    CombineAttributeArraysInputValues inputValues;
    inputValues.NormalizeData = false;
    inputValues.SelectedDataArrayPaths = {firstPath, secondPath};
    inputValues.StackedDataArrayPath = outputPath;
    const std::atomic_bool shouldCancel = false;
    const auto executeResult = CombineAttributeArrays(ds, {}, shouldCancel, &inputValues)();
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult)
    REQUIRE(executeResult.errors().front().code == k_WriteError);
    REQUIRE(outputStore->getWriteCount() == 2);
    REQUIRE((*output)[0] == 1);
    REQUIRE((*output)[1] == 2);
    REQUIRE((*output)[2] == 3);
    for(usize valueIdx = k_PageTuples * 3; valueIdx < output->getSize(); ++valueIdx)
    {
      REQUIRE((*output)[valueIdx] == k_UntouchedValue);
    }
  }
}

TEST_CASE("SimplnxCore::CombineAttributeArraysFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CombineAttributeArraysFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CombineAttributeArraysFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CombineAttributeArraysFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<CombineAttributeArraysFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
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
