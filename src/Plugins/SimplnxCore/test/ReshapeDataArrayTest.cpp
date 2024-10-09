#include "SimplnxCore/Filters/Algorithms/ReshapeDataArray.hpp"
#include "SimplnxCore/Filters/ReshapeDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
const std::string k_TestArrayName = "TestArray";
const std::string k_TestResultArrayName = "TestResultArray";
const DataPath k_InputArrayPath = DataPath({k_TestArrayName});
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid DataArrays - Same Size", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArrayName, std::vector<usize>{4}, std::vector<usize>{2});
  array1->setComponent(0, 0, 0);
  array1->setComponent(0, 1, 1);
  array1->setComponent(1, 0, 0);
  array1->setComponent(1, 1, 0);
  array1->setComponent(2, 0, 1);
  array1->setComponent(2, 1, 0);
  array1->setComponent(3, 0, 1);
  array1->setComponent(3, 1, 0);

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0, 2.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
  auto reshapedArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

  REQUIRE(reshapedArray.getTupleShape() == std::vector<usize>{2, 2});
  REQUIRE(reshapedArray.getNumberOfComponents() == 2);
  REQUIRE(reshapedArray[0] == 0);
  REQUIRE(reshapedArray[1] == 1);
  REQUIRE(reshapedArray[2] == 0);
  REQUIRE(reshapedArray[3] == 0);
  REQUIRE(reshapedArray[4] == 1);
  REQUIRE(reshapedArray[5] == 0);
  REQUIRE(reshapedArray[6] == 1);
  REQUIRE(reshapedArray[7] == 0);
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid DataArrays - Shrink", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArrayName, std::vector<usize>{3, 2}, std::vector<usize>{2});
  array1->setComponent(0, 0, 0);
  array1->setComponent(0, 1, 1);
  array1->setComponent(1, 0, 0);
  array1->setComponent(1, 1, 0);
  array1->setComponent(2, 0, 1);
  array1->setComponent(2, 1, 0);
  array1->setComponent(3, 0, 1);
  array1->setComponent(3, 1, 0);
  array1->setComponent(4, 0, 1);
  array1->setComponent(4, 1, 0);
  array1->setComponent(5, 0, 1);
  array1->setComponent(5, 1, 0);

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0, 2.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
  auto reshapedArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

  REQUIRE(reshapedArray.getTupleShape() == std::vector<usize>{2, 2});
  REQUIRE(reshapedArray.getNumberOfComponents() == 2);
  REQUIRE(reshapedArray[0] == 0);
  REQUIRE(reshapedArray[1] == 1);
  REQUIRE(reshapedArray[2] == 0);
  REQUIRE(reshapedArray[3] == 0);
  REQUIRE(reshapedArray[4] == 1);
  REQUIRE(reshapedArray[5] == 0);
  REQUIRE(reshapedArray[6] == 1);
  REQUIRE(reshapedArray[7] == 0);
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid DataArrays - Expand", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArrayName, std::vector<usize>{1}, std::vector<usize>{2});
  array1->setComponent(0, 0, 0);
  array1->setComponent(0, 1, 1);

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0, 2.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
  auto reshapedArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

  REQUIRE(reshapedArray.getTupleShape() == std::vector<usize>{2, 2});
  REQUIRE(reshapedArray.getNumberOfComponents() == 2);
  REQUIRE(reshapedArray[0] == 0);
  REQUIRE(reshapedArray[1] == 1);
  REQUIRE(reshapedArray[2] == 0);
  REQUIRE(reshapedArray[3] == 0);
  REQUIRE(reshapedArray[4] == 0);
  REQUIRE(reshapedArray[5] == 0);
  REQUIRE(reshapedArray[6] == 0);
  REQUIRE(reshapedArray[7] == 0);
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid NeighborLists - Shrink", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto inputNeighborList = NeighborList<T>::Create(dataStructure, k_TestArrayName, 2);
  typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>({0, 1, 0}));
  inputNeighborList->setList(0, inputList);
  typename NeighborList<T>::SharedVectorType inputList2(new std::vector<T>({1, 0, 0}));
  inputNeighborList->setList(1, inputList2);

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(k_InputArrayPath));
  auto reshapedNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(k_InputArrayPath);

  REQUIRE(reshapedNeighborList.getTupleShape() == std::vector<usize>{1});
  auto reshapedList1 = reshapedNeighborList.getList(0);

  REQUIRE(*reshapedList1 == *inputList);
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid NeighborLists - Expand", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto inputNeighborList = NeighborList<T>::Create(dataStructure, k_TestArrayName, 1);
  typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>({0, 1, 0}));
  inputNeighborList->setList(0, inputList);

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(k_InputArrayPath));
  auto reshapedNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(k_InputArrayPath);

  REQUIRE(reshapedNeighborList.getTupleShape() == std::vector<usize>{2});
  auto reshapedList1 = reshapedNeighborList.getList(0);
  auto reshapedList2 = reshapedNeighborList.getList(1);

  REQUIRE(*reshapedList1 == *inputList);
  REQUIRE((*reshapedList2).empty());
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid StringArrays - Shrink", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";
  std::string value3 = "Baz";
  std::string value4 = "Fizz";
  std::string value5 = "Buzz";
  std::string value6 = "FizzBuzz";

  StringArray::CreateWithValues(dataStructure, k_TestArrayName, {value1, value2, value3, value4, value5, value6});

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{4.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_InputArrayPath));
  auto reshapedArray = dataStructure.getDataRefAs<StringArray>(k_InputArrayPath);

  REQUIRE(reshapedArray.getTupleShape() == std::vector<usize>{4});
  REQUIRE(reshapedArray[0] == value1);
  REQUIRE(reshapedArray[1] == value2);
  REQUIRE(reshapedArray[2] == value3);
  REQUIRE(reshapedArray[3] == value4);
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Valid StringArrays - Expand", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";

  StringArray::CreateWithValues(dataStructure, k_TestArrayName, {value1, value2});

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{4.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_InputArrayPath));
  auto reshapedArray = dataStructure.getDataRefAs<StringArray>(k_InputArrayPath);

  REQUIRE(reshapedArray.getTupleShape() == std::vector<usize>{4});
  REQUIRE(reshapedArray[0] == value1);
  REQUIRE(reshapedArray[1] == value2);
  REQUIRE(reshapedArray[2].empty());
  REQUIRE(reshapedArray[3].empty());
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Invalid - Same Size", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32,
                   float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("DataArrays")
  {
    auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArrayName, std::vector<usize>{2, 2}, std::vector<usize>{2});
    array1->setComponent(0, 0, 0);
    array1->setComponent(0, 1, 1);
    array1->setComponent(1, 0, 0);
    array1->setComponent(1, 1, 0);
    array1->setComponent(2, 0, 1);
    array1->setComponent(2, 1, 0);
    array1->setComponent(3, 0, 1);
    array1->setComponent(3, 1, 0);

    args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
    args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0, 2.0}}));
  }
  SECTION("NeighborLists")
  {
    auto inputNeighborList = NeighborList<T>::Create(dataStructure, k_TestArrayName, 1);
    typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>({0, 1, 0}));
    inputNeighborList->setList(0, inputList);

    args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
    args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1.0}}));
  }
  SECTION("StringArrays")
  {
    std::string value1 = "Foo";
    std::string value2 = "Bar";

    StringArray::CreateWithValues(dataStructure, k_TestArrayName, {value1, value2});

    args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
    args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0}}));
  }

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == to_underlying(ReshapeDataArray::ErrorCodes::TupleShapesEqual));
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: NeighborList Warning - Expand", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto inputNeighborList = NeighborList<T>::Create(dataStructure, k_TestArrayName, 1);
  typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>({0, 1, 0}));
  inputNeighborList->setList(0, inputList);

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0, 2.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings()[0].code == to_underlying(ReshapeDataArray::WarningCodes::NeighborListMultipleTupleDims));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(k_InputArrayPath));
  auto reshapedNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(k_InputArrayPath);

  REQUIRE(reshapedNeighborList.getTupleShape() == std::vector<usize>{4});
  auto reshapedList1 = reshapedNeighborList.getList(0);
  auto reshapedList2 = reshapedNeighborList.getList(1);
  auto reshapedList3 = reshapedNeighborList.getList(2);
  auto reshapedList4 = reshapedNeighborList.getList(3);

  REQUIRE(*reshapedList1 == *inputList);
  REQUIRE((*reshapedList2).empty());
  REQUIRE((*reshapedList3).empty());
  REQUIRE((*reshapedList4).empty());
}

TEMPLATE_TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: StringArray Warning - Expand", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";

  StringArray::CreateWithValues(dataStructure, k_TestArrayName, {value1, value2});

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2.0, 3.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings()[0].code == to_underlying(ReshapeDataArray::WarningCodes::StringArrayMultipleTupleDims));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_InputArrayPath));
  auto reshapedArray = dataStructure.getDataRefAs<StringArray>(k_InputArrayPath);

  REQUIRE(reshapedArray.getTupleShape() == std::vector<usize>{6});
  REQUIRE(reshapedArray[0] == value1);
  REQUIRE(reshapedArray[1] == value2);
  REQUIRE(reshapedArray[2].empty());
  REQUIRE(reshapedArray[3].empty());
  REQUIRE(reshapedArray[4].empty());
  REQUIRE(reshapedArray[5].empty());
}

TEST_CASE("SimplnxCore::ReshapeDataArraysFilter: Invalid Tuple Dimensions", "[SimplnxCore][ConcatenateDataArraysFilter]")
{
  ReshapeDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";

  StringArray::CreateWithValues(dataStructure, k_TestArrayName, {value1, value2});

  args.insert(ReshapeDataArrayFilter::k_Input_Array_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insert(ReshapeDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{0.0, 3.0}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == to_underlying(ReshapeDataArray::ErrorCodes::NonPositiveTupleDimValue));
}