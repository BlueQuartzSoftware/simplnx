#include "SimplnxCore/Filters/CreateDataArrayAdvancedFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

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
