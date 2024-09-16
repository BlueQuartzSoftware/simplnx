#include "SimplnxCore/Filters/ConcatenateDataArraysFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
const std::string k_TestArray1Name = "TestArray1";
const std::string k_TestArray2Name = "TestArray2";
const std::string k_TestResultArrayName = "TestResultArray";
const std::vector<DataPath> k_InputArrayPaths = std::vector<DataPath>{DataPath({k_TestArray1Name}), DataPath({k_TestArray2Name})};
const DataPath k_OutputArrayPath = DataPath({k_TestResultArrayName});
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: DataArrays Valid - 1 Tuple", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArray1Name, std::vector<usize>{1}, std::vector<usize>{3});
  array1->setComponent(0, 0, 0);
  array1->setComponent(0, 1, 1);
  array1->setComponent(0, 2, 0);
  auto array2 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArray2Name, std::vector<usize>{1}, std::vector<usize>{3});
  array2->setComponent(0, 0, 1);
  array2->setComponent(0, 1, 1);
  array2->setComponent(0, 2, 1);

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_OutputArrayPath));
  auto outputArray = dataStructure.getDataRefAs<DataArray<T>>(k_OutputArrayPath);

  REQUIRE(outputArray.getTupleShape() == std::vector<usize>{2});
  REQUIRE(outputArray.getNumberOfComponents() == 3);
  REQUIRE(outputArray[0] == 0);
  REQUIRE(outputArray[1] == 1);
  REQUIRE(outputArray[2] == 0);
  REQUIRE(outputArray[3] == 1);
  REQUIRE(outputArray[4] == 1);
  REQUIRE(outputArray[5] == 1);
}

TEMPLATE_TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: DataArrays Valid - 2 Tuples", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32,
                   uint64, float32, float64)
{
  using T = TestType;

  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArray1Name, std::vector<usize>{2}, std::vector<usize>{2});
  array1->setComponent(0, 0, 0);
  array1->setComponent(0, 1, 1);
  array1->setComponent(1, 0, 0);
  array1->setComponent(1, 1, 0);
  auto array2 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArray2Name, std::vector<usize>{2}, std::vector<usize>{2});
  array2->setComponent(0, 0, 1);
  array2->setComponent(0, 1, 1);
  array2->setComponent(1, 0, 1);
  array2->setComponent(1, 1, 0);

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2, 2}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_OutputArrayPath));
  auto outputArray = dataStructure.getDataRefAs<DataArray<T>>(k_OutputArrayPath);

  REQUIRE(outputArray.getTupleShape() == std::vector<usize>{2, 2});
  REQUIRE(outputArray.getNumberOfComponents() == 2);
  REQUIRE(outputArray[0] == 0);
  REQUIRE(outputArray[1] == 1);
  REQUIRE(outputArray[2] == 0);
  REQUIRE(outputArray[3] == 0);
  REQUIRE(outputArray[4] == 1);
  REQUIRE(outputArray[5] == 1);
  REQUIRE(outputArray[6] == 1);
  REQUIRE(outputArray[7] == 0);
}

TEMPLATE_TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: DataArrays Valid - 3 Tuples", "[SimplnxCore][ConcatenateDataArraysFilter]", bool, int8, int16, int32, int64, uint8, uint16, uint32,
                   uint64, float32, float64)
{
  using T = TestType;

  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto array1 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArray1Name, std::vector<usize>{3}, std::vector<usize>{1});
  array1->setValue(0, 0);
  array1->setValue(1, 1);
  array1->setValue(2, 0);
  auto array2 = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_TestArray2Name, std::vector<usize>{3}, std::vector<usize>{1});
  array2->setValue(0, 1);
  array2->setValue(1, 1);
  array2->setValue(2, 1);

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{3, 2}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_OutputArrayPath));
  auto outputArray = dataStructure.getDataRefAs<DataArray<T>>(k_OutputArrayPath);

  REQUIRE(outputArray.getTupleShape() == std::vector<usize>{3, 2});
  REQUIRE(outputArray.getNumberOfComponents() == 1);
  REQUIRE(outputArray[0] == 0);
  REQUIRE(outputArray[1] == 1);
  REQUIRE(outputArray[2] == 0);
  REQUIRE(outputArray[3] == 1);
  REQUIRE(outputArray[4] == 1);
  REQUIRE(outputArray[5] == 1);
}

TEMPLATE_TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: NeighborLists Valid - 1 List", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto inputNeighborList1 = NeighborList<T>::Create(dataStructure, k_TestArray1Name, 1);
  typename NeighborList<T>::SharedVectorType inputList1(new std::vector<T>({0, 1, 0}));
  inputNeighborList1->setList(0, inputList1);
  auto inputNeighborList2 = NeighborList<T>::Create(dataStructure, k_TestArray2Name, 1);
  typename NeighborList<T>::SharedVectorType inputList2(new std::vector<T>({1, 1, 1}));
  inputNeighborList2->setList(0, inputList2);

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(k_OutputArrayPath));
  auto outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(k_OutputArrayPath);

  REQUIRE(outputNeighborList.getTupleShape() == std::vector<usize>{2});
  auto outputList1 = outputNeighborList.getList(0);
  auto outputList2 = outputNeighborList.getList(1);

  REQUIRE(outputList1 == inputList1);
  REQUIRE(outputList2 == inputList2);
}

TEMPLATE_TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: NeighborLists Valid - 2 Lists", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto inputNeighborList1 = NeighborList<T>::Create(dataStructure, k_TestArray1Name, 2);
  typename NeighborList<T>::SharedVectorType inputList1(new std::vector<T>({0, 1, 0}));
  inputNeighborList1->setList(0, inputList1);
  typename NeighborList<T>::SharedVectorType inputList2(new std::vector<T>({1, 0, 0}));
  inputNeighborList1->setList(1, inputList2);
  auto inputNeighborList2 = NeighborList<T>::Create(dataStructure, k_TestArray2Name, 2);
  typename NeighborList<T>::SharedVectorType inputList3(new std::vector<T>({1, 1, 1}));
  inputNeighborList2->setList(0, inputList3);
  typename NeighborList<T>::SharedVectorType inputList4(new std::vector<T>({0, 0, 1}));
  inputNeighborList2->setList(1, inputList4);

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{4}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(k_OutputArrayPath));
  auto outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(k_OutputArrayPath);

  REQUIRE(outputNeighborList.getTupleShape() == std::vector<usize>{4});
  auto outputList1 = outputNeighborList.getList(0);
  auto outputList2 = outputNeighborList.getList(1);
  auto outputList3 = outputNeighborList.getList(2);
  auto outputList4 = outputNeighborList.getList(3);

  REQUIRE(outputList1 == inputList1);
  REQUIRE(outputList2 == inputList2);
  REQUIRE(outputList3 == inputList3);
  REQUIRE(outputList4 == inputList4);
}

TEMPLATE_TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: NeighborLists Valid - 3 Lists", "[SimplnxCore][ConcatenateDataArraysFilter]", int8, int16, int32, int64, uint8, uint16, uint32, uint64,
                   float32, float64)
{
  using T = TestType;

  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  auto inputNeighborList1 = NeighborList<T>::Create(dataStructure, k_TestArray1Name, 3);
  typename NeighborList<T>::SharedVectorType inputList1(new std::vector<T>({0, 1, 0}));
  inputNeighborList1->setList(0, inputList1);
  typename NeighborList<T>::SharedVectorType inputList2(new std::vector<T>({1, 0, 0}));
  inputNeighborList1->setList(1, inputList2);
  typename NeighborList<T>::SharedVectorType inputList3(new std::vector<T>({2, 2, 1}));
  inputNeighborList1->setList(2, inputList3);
  auto inputNeighborList2 = NeighborList<T>::Create(dataStructure, k_TestArray2Name, 3);
  typename NeighborList<T>::SharedVectorType inputList4(new std::vector<T>({1, 1, 1}));
  inputNeighborList2->setList(0, inputList4);
  typename NeighborList<T>::SharedVectorType inputList5(new std::vector<T>({0, 0, 1}));
  inputNeighborList2->setList(1, inputList5);
  typename NeighborList<T>::SharedVectorType inputList6(new std::vector<T>({4, 5, 6}));
  inputNeighborList2->setList(2, inputList6);

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{6}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(k_OutputArrayPath));
  auto outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(k_OutputArrayPath);

  REQUIRE(outputNeighborList.getTupleShape() == std::vector<usize>{6});
  auto outputList1 = outputNeighborList.getList(0);
  auto outputList2 = outputNeighborList.getList(1);
  auto outputList3 = outputNeighborList.getList(2);
  auto outputList4 = outputNeighborList.getList(3);
  auto outputList5 = outputNeighborList.getList(4);
  auto outputList6 = outputNeighborList.getList(5);

  REQUIRE(outputList1 == inputList1);
  REQUIRE(outputList2 == inputList2);
  REQUIRE(outputList3 == inputList3);
  REQUIRE(outputList4 == inputList4);
  REQUIRE(outputList5 == inputList5);
  REQUIRE(outputList6 == inputList6);
}

TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: StringArray Valid - 1 Tuple", "[SimplnxCore][ConcatenateDataArraysFilter]")
{
  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";

  StringArray::CreateWithValues(dataStructure, k_TestArray1Name, {value1});
  StringArray::CreateWithValues(dataStructure, k_TestArray2Name, {value2});

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath));
  auto outputArray = dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath);

  REQUIRE(outputArray.getTupleShape() == std::vector<usize>{2});
  REQUIRE(outputArray[0] == value1);
  REQUIRE(outputArray[1] == value2);
}

TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: StringArray Valid - 2 Tuples", "[SimplnxCore][ConcatenateDataArraysFilter]")
{
  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";
  std::string value3 = "Baz";
  std::string value4 = "Fizzle";

  StringArray::CreateWithValues(dataStructure, k_TestArray1Name, {value1, value2});
  StringArray::CreateWithValues(dataStructure, k_TestArray2Name, {value3, value4});

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{4}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath));
  auto outputArray = dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath);

  REQUIRE(outputArray.getTupleShape() == std::vector<usize>{4});
  REQUIRE(outputArray[0] == value1);
  REQUIRE(outputArray[1] == value2);
  REQUIRE(outputArray[2] == value3);
  REQUIRE(outputArray[3] == value4);
}

TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: StringArray Valid - 3 Tuples", "[SimplnxCore][ConcatenateDataArraysFilter]")
{
  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::string value1 = "Foo";
  std::string value2 = "Bar";
  std::string value3 = "Baz";
  std::string value4 = "Fizzle";
  std::string value5 = "Sizzle";
  std::string value6 = "Twizzler";

  StringArray::CreateWithValues(dataStructure, k_TestArray1Name, {value1, value2, value3});
  StringArray::CreateWithValues(dataStructure, k_TestArray2Name, {value4, value5, value6});

  args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
  args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
  args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{6}}));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath));
  auto outputArray = dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath);

  REQUIRE(outputArray.getTupleShape() == std::vector<usize>{6});
  REQUIRE(outputArray[0] == value1);
  REQUIRE(outputArray[1] == value2);
  REQUIRE(outputArray[2] == value3);
  REQUIRE(outputArray[3] == value4);
  REQUIRE(outputArray[4] == value5);
  REQUIRE(outputArray[5] == value6);
}

TEST_CASE("SimplnxCore::ConcatenateDataArraysFilter: Invalid Parameters", "[SimplnxCore][ConcatenateDataArraysFilter]")
{
  ConcatenateDataArraysFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Empty Input Arrays")
  {
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray1Name, std::vector<usize>{1}, std::vector<usize>{3});
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray2Name, std::vector<usize>{1}, std::vector<usize>{3});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors()[0].code == to_underlying(ConcatenateDataArraysFilter::ErrorCodes::EmptyInputArrays));
  }
  SECTION("One Input Array")
  {
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray1Name, std::vector<usize>{1}, std::vector<usize>{3});
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray2Name, std::vector<usize>{1}, std::vector<usize>{3});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_TestArray1Name})}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors()[0].code == to_underlying(ConcatenateDataArraysFilter::ErrorCodes::OneInputArray));
  }
  SECTION("Mismatching Type Names 1")
  {
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray1Name, std::vector<usize>{1}, std::vector<usize>{3});
    Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_TestArray2Name, std::vector<usize>{1}, std::vector<usize>{3});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_InputArrayPaths}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors()[0].code == to_underlying(ConcatenateDataArraysFilter::ErrorCodes::TypeNameMismatch));
  }

  SECTION("Mismatching Type Names 2")
  {
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray1Name, std::vector<usize>{1}, std::vector<usize>{3});
    StringArray::CreateWithValues(dataStructure, k_TestArray2Name, {"Foo"});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_InputArrayPaths}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors()[0].code == to_underlying(ConcatenateDataArraysFilter::ErrorCodes::TypeNameMismatch));
  }
  SECTION("Mismatching Component Dimensions")
  {
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray1Name, std::vector<usize>{1}, std::vector<usize>{3, 4});
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray2Name, std::vector<usize>{1}, std::vector<usize>{4, 3});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_InputArrayPaths}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors()[0].code == to_underlying(ConcatenateDataArraysFilter::ErrorCodes::ComponentShapeMismatch));
  }
  SECTION("Mismatching Tuple Counts")
  {
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray1Name, std::vector<usize>{2, 3}, std::vector<usize>{3});
    Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_TestArray2Name, std::vector<usize>{4, 5}, std::vector<usize>{3});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_InputArrayPaths}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors()[0].code == to_underlying(ConcatenateDataArraysFilter::ErrorCodes::TotalTuplesMismatch));
  }
  SECTION("NeighborList Multiple Tuple Dims")
  {
    auto inputNeighborList1 = NeighborList<int8>::Create(dataStructure, k_TestArray1Name, 2);
    typename NeighborList<int8>::SharedVectorType inputList1(new std::vector<int8>({0, 1, 0}));
    inputNeighborList1->setList(0, inputList1);
    typename NeighborList<int8>::SharedVectorType inputList2(new std::vector<int8>({0, 1, 0}));
    inputNeighborList1->setList(1, inputList2);
    auto inputNeighborList2 = NeighborList<int8>::Create(dataStructure, k_TestArray2Name, 2);
    typename NeighborList<int8>::SharedVectorType inputList3(new std::vector<int8>({1, 1, 1}));
    inputNeighborList2->setList(0, inputList3);
    typename NeighborList<int8>::SharedVectorType inputList4(new std::vector<int8>({1, 1, 1}));
    inputNeighborList2->setList(1, inputList4);

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_InputArrayPaths}));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2, 2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().size() == 1);
    REQUIRE(result.result.warnings()[0].code == to_underlying(ConcatenateDataArraysFilter::WarningCodes::MultipleTupleDimsNotSupported));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int8>>(k_OutputArrayPath));
    auto outputNeighborList = dataStructure.getDataRefAs<NeighborList<int8>>(k_OutputArrayPath);
    REQUIRE(outputNeighborList.getTupleShape() == std::vector<usize>{4});
  }
  SECTION("StringArray Multiple Tuple Dims")
  {
    StringArray::CreateWithValues(dataStructure, k_TestArray1Name, {"Foo", "Bar"});
    StringArray::CreateWithValues(dataStructure, k_TestArray2Name, {"Baz", "Fizzle"});

    args.insert(ConcatenateDataArraysFilter::k_InputArrays_Key, std::make_any<std::vector<DataPath>>(k_InputArrayPaths));
    args.insert(ConcatenateDataArraysFilter::k_OutputArray_Key, std::make_any<DataPath>(k_OutputArrayPath));
    args.insert(ConcatenateDataArraysFilter::k_OutputTupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2, 2}}));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().size() == 1);
    REQUIRE(result.result.warnings()[0].code == to_underlying(ConcatenateDataArraysFilter::WarningCodes::MultipleTupleDimsNotSupported));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath));
    auto outputDataArray = dataStructure.getDataRefAs<StringArray>(k_OutputArrayPath);
    REQUIRE(outputDataArray.getTupleShape() == std::vector<usize>{4});
  }
}
