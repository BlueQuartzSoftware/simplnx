

#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CreateDataArray.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;

TEST_CASE("Instantiate CreateDataArray Filter")
{
  //  static constexpr uint64 k_NComp = 3;
  //  static constexpr uint64 k_NLines = 25;

  CreateDataArray filter;
  DataStructure ds;
  Arguments args;
  //  DataPath dataPath({"foo"});

  //  args.insert(CreateDataArray::k_NumericType_Key, std::make_any<NumericType>(NumericType::int32));
  //  args.insert(CreateDataArray::k_NumComps_Key, std::make_any<uint64>(k_NComp));
  //  args.insert(CreateDataArray::k_NumTuples_Key, std::make_any<uint64>(k_NLines));
  //  args.insert(CreateDataArray::k_DataPath_Key, std::make_any<DataPath>(dataPath));

  auto result = filter.execute(ds, args);
  REQUIRE(result.result.valid());
}
