#include "SimplnxCore/Filters/CreateDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::CreateDataArrayFilter(Instantiate)", "[SimplnxCore][CreateDataArrayFilter]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};

  static const DataPath k_DataPath({"foo"});

  CreateDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(k_NComp));
  args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
  args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("SimplnxCore::CreateDataArrayFilter(Invalid Parameters)", "[SimplnxCore][CreateDataArrayFilter]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath k_DataPath({"foo"});

  CreateDataArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Section1")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section2")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1024"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section3")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section4")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));

    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(0.0)}};
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section5")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>(""));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section6")
  {
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArrayFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("1000"));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint8));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int16));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("70000"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("4294967297"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("-4294967297"));
    result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }
}
