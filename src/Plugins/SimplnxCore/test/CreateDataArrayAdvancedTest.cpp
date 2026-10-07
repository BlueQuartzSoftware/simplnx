#include "SimplnxCore/Filters/CreateDataArrayAdvancedFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>

using namespace nx::core;

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter(Instantiate)", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath k_DataPath({"foo"});

  SECTION("Component-specific fill values")
  {
    CreateDataArrayAdvancedFilter filter;
    DataStructure dataStructure;
    Arguments args;
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(k_NComp)}}));
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("5;17;-3"));

    const auto result = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_DataPath));
    const auto& array = dataStructure.getDataRefAs<Int32Array>(k_DataPath);
    REQUIRE(array.getNumberOfTuples() == k_NumTuples);
    REQUIRE(array.getNumberOfComponents() == k_NComp);
    for(usize tupleIdx = 0; tupleIdx < k_NumTuples; tupleIdx++)
    {
      REQUIRE(array[tupleIdx * k_NComp] == 5);
      REQUIRE(array[tupleIdx * k_NComp + 1] == 17);
      REQUIRE(array[tupleIdx * k_NComp + 2] == -3);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Component-specific incremental values")
  {
    CreateDataArrayAdvancedFilter filter;
    DataStructure dataStructure;
    Arguments args;
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(k_NComp)}}));
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(1));
    args.insert(CreateDataArrayAdvancedFilter::k_StartingFillValue_Key, std::make_any<std::string>("4;10;-8"));
    args.insert(CreateDataArrayAdvancedFilter::k_StepOperation_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArrayAdvancedFilter::k_StepValue_Key, std::make_any<std::string>("2;-3;5"));

    const auto result = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_DataPath));
    const auto& array = dataStructure.getDataRefAs<Int32Array>(k_DataPath);
    REQUIRE(array.getNumberOfTuples() == k_NumTuples);
    REQUIRE(array.getNumberOfComponents() == k_NComp);
    REQUIRE(array[0] == 4);
    REQUIRE(array[1] == 10);
    REQUIRE(array[2] == -8);
    REQUIRE(array[3] == 6);
    REQUIRE(array[4] == 7);
    REQUIRE(array[5] == -3);
    REQUIRE(array[6] == 8);
    REQUIRE(array[7] == 4);
    REQUIRE(array[8] == 2);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Fixed-seed component-specific random range")
  {
    CreateDataArrayAdvancedFilter filter;
    DataStructure dataStructure;
    Arguments args;
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(k_NComp)}}));
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(3));
    args.insert(CreateDataArrayAdvancedFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insert(CreateDataArrayAdvancedFilter::k_SeedValue_Key, std::make_any<uint64>(8675309));
    args.insert(CreateDataArrayAdvancedFilter::k_StandardizeSeed_Key, std::make_any<bool>(false));
    args.insert(CreateDataArrayAdvancedFilter::k_InitStartRange_Key, std::make_any<std::string>("4;10;-8"));
    args.insert(CreateDataArrayAdvancedFilter::k_InitEndRange_Key, std::make_any<std::string>("6;12;-6"));

    const auto result = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_DataPath));
    const auto& array = dataStructure.getDataRefAs<Int32Array>(k_DataPath);
    REQUIRE(array.getNumberOfTuples() == k_NumTuples);
    REQUIRE(array.getNumberOfComponents() == k_NComp);
    for(usize tupleIdx = 0; tupleIdx < k_NumTuples; tupleIdx++)
    {
      REQUIRE(array[tupleIdx * k_NComp] >= 4);
      REQUIRE(array[tupleIdx * k_NComp] <= 6);
      REQUIRE(array[tupleIdx * k_NComp + 1] >= 10);
      REQUIRE(array[tupleIdx * k_NComp + 1] <= 12);
      REQUIRE(array[tupleIdx * k_NComp + 2] >= -8);
      REQUIRE(array[tupleIdx * k_NComp + 2] <= -6);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter(Invalid Parameters)", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath k_DataPath({"foo"});

  CreateDataArrayAdvancedFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Section1")
  {
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(k_NComp)}}));
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("-1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section2")
  {
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(k_NComp)}}));
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("1024"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section3")
  {
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{0.0}}));
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section4")
  {
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1.0}}));

    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(0.0)}};
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section5")
  {
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1.0}}));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>(""));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section6")
  {
    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1.0}}));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("1000"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint8));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int16));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("70000"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("4294967297"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("-4294967297"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter: Non-Integer Text Does Not Throw In Preflight", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();

  CreateDataArrayAdvancedFilter filter;
  DataStructure dataStructure;
  const DataPath arrayPath({"Data"});
  Arguments args;
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(1));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_StepOperation_Key, std::make_any<uint64>(0));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_StepValue_Key, std::make_any<std::string>("1"));

  const auto checkValues = [&]<typename T, usize N>(const std::array<T, N>& expectedValues) {
    REQUIRE_NOTHROW(static_cast<void>(filter.preflight(dataStructure, args)));
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(arrayPath));
    const auto& array = dataStructure.getDataRefAs<DataArray<T>>(arrayPath);
    REQUIRE(array.getSize() == N);
    for(usize valueIdx = 0; valueIdx < N; valueIdx++)
    {
      CAPTURE(valueIdx);
      REQUIRE(array[valueIdx] == expectedValues[valueIdx]);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  };

  SECTION("a: Fractional float32 incremental start")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{3}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_StartingFillValue_Key, std::make_any<std::string>(".5"));
    checkValues(std::array<float32, 3>{0.5f, 1.5f, 2.5f});
  }
  SECTION("b: uint64 incremental start above int64 maximum")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint64));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_StartingFillValue_Key, std::make_any<std::string>("10000000000000000000"));
    checkValues(std::array<uint64, 2>{10000000000000000000ULL, 10000000000000000001ULL});
  }
  SECTION("c: Fractional float32 component fill values")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{3}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>(".25;.5;.75"));
    checkValues(std::array<float32, 3>{0.25f, 0.5f, 0.75f});
  }
  SECTION("e: A single step applies to all int32 components")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{2}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{3}}));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_StartingFillValue_Key, std::make_any<std::string>("0;1;2"));
    checkValues(std::array<int32, 6>{0, 1, 2, 1, 2, 3});
  }
}

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter: Standardized Seeded Float Random Is Identical And Reproducible", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();

  CreateDataArrayAdvancedFilter filter;
  const DataPath arrayPath({"Data"});
  Arguments args;
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{50}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{3}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(2));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_StandardizeSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_SeedArrayName_Key, std::make_any<std::string>("SeedArray"));

  const auto checkRandom = [&](auto value) {
    using ValueType = decltype(value);
    DataStructure firstRun;
    DataStructure secondRun;
    auto firstResult = filter.execute(firstRun, args);
    SIMPLNX_RESULT_REQUIRE_VALID(firstResult.result);
    auto secondResult = filter.execute(secondRun, args);
    SIMPLNX_RESULT_REQUIRE_VALID(secondResult.result);

    REQUIRE_NOTHROW(firstRun.getDataRefAs<DataArray<ValueType>>(arrayPath));
    REQUIRE_NOTHROW(secondRun.getDataRefAs<DataArray<ValueType>>(arrayPath));
    const auto& firstArray = firstRun.getDataRefAs<DataArray<ValueType>>(arrayPath);
    const auto& secondArray = secondRun.getDataRefAs<DataArray<ValueType>>(arrayPath);
    REQUIRE(firstArray.getNumberOfTuples() == 50);
    REQUIRE(firstArray.getNumberOfComponents() == 3);
    REQUIRE(secondArray.getNumberOfTuples() == 50);
    REQUIRE(secondArray.getNumberOfComponents() == 3);
    for(usize tupleIdx = 0; tupleIdx < 50; tupleIdx++)
    {
      CAPTURE(tupleIdx);
      REQUIRE(firstArray[3 * tupleIdx] == firstArray[3 * tupleIdx + 1]);
      REQUIRE(firstArray[3 * tupleIdx] == firstArray[3 * tupleIdx + 2]);
      REQUIRE(secondArray[3 * tupleIdx] == secondArray[3 * tupleIdx + 1]);
      REQUIRE(secondArray[3 * tupleIdx] == secondArray[3 * tupleIdx + 2]);
    }
    for(usize valueIdx = 0; valueIdx < 150; valueIdx++)
    {
      CAPTURE(valueIdx);
      REQUIRE(firstArray[valueIdx] == secondArray[valueIdx]);
    }
    UnitTest::CheckArraysInheritTupleDims(firstRun);
    UnitTest::CheckArraysInheritTupleDims(secondRun);
  };

  SECTION("float32")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    checkRandom(float32{});
  }
  SECTION("float64")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float64));
    checkRandom(float64{});
  }
}

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter: Random Range Start Greater Than End Is Rejected", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();

  CreateDataArrayAdvancedFilter filter;
  DataStructure dataStructure;
  const DataPath arrayPath({"Data"});
  Arguments args;
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{3}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(3));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_SeedArrayName_Key, std::make_any<std::string>("SeedArray"));

  SECTION("int32 reversed range")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitStartRange_Key, std::make_any<std::string>("10"));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitEndRange_Key, std::make_any<std::string>("0"));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -11615);
  }
  SECTION("float64 reversed range")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float64));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitStartRange_Key, std::make_any<std::string>("1.5"));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitEndRange_Key, std::make_any<std::string>("0.5"));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -11615);
  }
  SECTION("Equal endpoints are valid and produce the endpoint")
  {
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitStartRange_Key, std::make_any<std::string>("5"));
    args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitEndRange_Key, std::make_any<std::string>("5"));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(arrayPath));
    const auto& array = dataStructure.getDataRefAs<Int32Array>(arrayPath);
    REQUIRE(array.getSize() == 3);
    REQUIRE(array[0] == 5);
    REQUIRE(array[1] == 5);
    REQUIRE(array[2] == 5);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter: Non-Whole Tuple Dimensions Are Rejected", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();

  const float64 tupleDim = GENERATE(0.5, 2.5, -1.0);
  CAPTURE(tupleDim);
  CreateDataArrayAdvancedFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"Data"})));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{tupleDim}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(0));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("1"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -78603);
}

TEST_CASE("SimplnxCore::CreateDataArrayAdvancedFilter: Non-Whole Component Dimensions Are Rejected", "[SimplnxCore][CreateDataArrayAdvancedFilter]")
{
  UnitTest::LoadPlugins();

  const float64 compDim = GENERATE(0.5, 2.5, -1.0);
  CAPTURE(compDim);
  CreateDataArrayAdvancedFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"Data"})));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{1}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{compDim}}));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitType_Key, std::make_any<uint64>(0));
  args.insertOrAssign(CreateDataArrayAdvancedFilter::k_InitValue_Key, std::make_any<std::string>("1"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -78605);
}
