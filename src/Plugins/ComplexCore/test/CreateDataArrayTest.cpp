#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CreateDataArray.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ComplexCore::CreateDataArray(Instantiate)", "[ComplexCore][CreateDataArray]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};

  static const DataPath k_DataPath({"foo"});

  CreateDataArray filter;
  DataStructure dataGraph;
  Arguments args;

  args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
  args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
  args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CreateDataArray(Invalid Parameters)", "[ComplexCore][CreateDataArray]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath k_DataPath({"foo"});

  CreateDataArray filter;
  DataStructure dataGraph;
  Arguments args;

  SECTION("Section1")
  {
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("-1"));

    auto result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section2")
  {
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1024"));

    auto result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section3")
  {
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section4")
  {
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(1));

    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(0.0)}};
    args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);
  }
  SECTION("Section5")
  {
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(1));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>(""));

    auto result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);
  }
  SECTION("Section6")
  {
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(1));
    DynamicTableInfo::TableDataType tupleDims = {{static_cast<double>(1.0)}};
    args.insert(CreateDataArray::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(tupleDims));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1000"));

    auto result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint8));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int16));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("70000"));
    result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("-1"));
    result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("4294967297"));
    result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);

    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("-4294967297"));
    result = filter.execute(dataGraph, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);
  }
}
