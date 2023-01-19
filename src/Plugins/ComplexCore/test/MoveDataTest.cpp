#include "ComplexCore/Filters/MoveData.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <set>

using namespace complex;

namespace
{
constexpr StringLiteral k_Group1Name = "Group1";
constexpr StringLiteral k_Group2Name = "Group2";
constexpr StringLiteral k_Group3Name = "Group3";
constexpr StringLiteral k_Group4Name = "Group4";

DataStructure createDataStructure()
{
  DataStructure data;
  auto group1 = DataGroup::Create(data, k_Group1Name);
  auto group2 = DataGroup::Create(data, k_Group2Name);
  auto group3 = DataGroup::Create(data, k_Group3Name, group2->getId());
  auto group4 = DataGroup::Create(data, k_Group4Name, group2->getId());

  data.setAdditionalParent(group4->getId(), group3->getId());
  return data;
}
} // namespace

TEST_CASE("ComplexCore::MoveData Successful", "[Complex::Core][MoveData]")
{
  MoveData filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_Group1Path({k_Group1Name});
  const DataPath k_Group3Path({k_Group2Name, k_Group3Name});

  args.insertOrAssign(MoveData::k_Data_Key, std::make_any<DataPath>(k_Group3Path));
  args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_Group1Path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto* group1 = dataStructure.getDataAs<DataGroup>(k_Group1Path);
  REQUIRE(group1->getDataMap().getSize() == 1);

  const auto* group2 = dataStructure.getDataAs<DataGroup>(DataPath({k_Group2Name}));
  REQUIRE(group2->getDataMap().getSize() == 1);

  REQUIRE(dataStructure.getDataAs<DataGroup>(k_Group3Path) == nullptr);

  const DataPath newGroup3Path = k_Group1Path.createChildPath(k_Group3Name);
  REQUIRE(dataStructure.getDataAs<DataGroup>(newGroup3Path) != nullptr);
}

TEST_CASE("ComplexCore::MoveData Unsuccessful", "[Complex::Core][MoveData]")
{
  MoveData filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_Group2Path({k_Group2Name});
  const DataPath k_Group3Path({k_Group3Name});

  args.insertOrAssign(MoveData::k_Data_Key, std::make_any<DataPath>(k_Group3Path));
  args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_Group2Path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}
