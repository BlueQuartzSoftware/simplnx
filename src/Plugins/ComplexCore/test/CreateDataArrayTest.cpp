#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CreateDataArray.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;

TEST_CASE("CreateDataArray(Instantiate)", "[ComplexCore][CreateDataArray]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;

  CreateDataArray filter;
  DataStructure ds;
  Arguments args;
  DataPath dataPath({"foo"});

  args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
  args.insert(CreateDataArray::k_NumTuples_Key, std::make_any<uint64>(k_NumTuples));
  args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(dataPath));

  auto result = filter.execute(ds, args);
  REQUIRE(result.result.valid());
}

TEST_CASE("CreateDataArray(Invalid Parameters)", "[ComplexCore][CreateDataArray]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;

  DataPath dataPath({"foo"});
  {
    CreateDataArray filter;
    DataStructure ds;
    Arguments args;
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::uint16));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArray::k_NumTuples_Key, std::make_any<uint64>(k_NumTuples));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(dataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("-1"));

    auto result = filter.execute(ds, args);
    REQUIRE(result.result.invalid());
  }
  {
    CreateDataArray filter;
    DataStructure ds;
    Arguments args;
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int8));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
    args.insert(CreateDataArray::k_NumTuples_Key, std::make_any<uint64>(k_NumTuples));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(dataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1024"));

    auto result = filter.execute(ds, args);
    REQUIRE(result.result.invalid());
  }
  {
    CreateDataArray filter;
    DataStructure ds;
    Arguments args;
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArray::k_NumTuples_Key, std::make_any<uint64>(k_NumTuples));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(dataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(ds, args);
    REQUIRE(result.result.invalid());
  }
  {
    CreateDataArray filter;
    DataStructure ds;
    Arguments args;
    args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(1));
    args.insert(CreateDataArray::k_NumTuples_Key, std::make_any<uint64>(0));
    args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(dataPath));
    args.insert(CreateDataArray::k_InitilizationValue_Key, std::make_any<std::string>("1"));

    auto result = filter.execute(ds, args);
    REQUIRE(result.result.invalid());
  }
}
