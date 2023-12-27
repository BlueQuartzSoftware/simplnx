#include "SimplnxCore/Filters/MoveData.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <set>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_Group1Name = "Group1";
constexpr StringLiteral k_Group2Name = "Group2";
constexpr StringLiteral k_Group3Name = "Group3";
constexpr StringLiteral k_Group4Name = "Group4";

const int32 k_TupleDimMismatchWarningCode = -69250;

constexpr StringLiteral k_AM1Name = "Matrix1";
constexpr StringLiteral k_DataArray1Name = "Array1";

DataStructure createDataStructure()
{
  DataStructure data;
  auto group1 = DataGroup::Create(data, k_Group1Name);
  auto group2 = DataGroup::Create(data, k_Group2Name);
  auto group3 = DataGroup::Create(data, k_Group3Name, group2->getId());
  auto group4 = DataGroup::Create(data, k_Group4Name, group2->getId());

  auto am1 = AttributeMatrix::Create(data, k_AM1Name, std::vector<usize>{2});

  auto dataStore = std::make_unique<DataStore<uint8>>(std::vector<usize>{20}, std::vector<usize>{3}, 0);
  auto dataArray1 = DataArray<uint8>::Create(data, k_DataArray1Name, std::move(dataStore));

  data.setAdditionalParent(group4->getId(), group3->getId());
  return data;
}
} // namespace

TEST_CASE("SimplnxCore::MoveData Successful", "[Simplnx::Core][MoveData]")
{
  MoveData filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_Group1Path({k_Group1Name});
  const DataPath k_Group3Path({k_Group2Name, k_Group3Name});
  const DataPath k_Group4Path({k_Group2Name, k_Group4Name});

  args.insertOrAssign(MoveData::k_Data_Key, std::make_any<std::vector<DataPath>>({k_Group3Path, k_Group4Path}));
  args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_Group1Path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* group1 = dataStructure.getDataAs<DataGroup>(k_Group1Path);
  REQUIRE(group1->getDataMap().getSize() == 2);

  const auto* group2 = dataStructure.getDataAs<DataGroup>(DataPath({k_Group2Name}));
  REQUIRE(group2->getDataMap().getSize() == 0);

  REQUIRE(dataStructure.getDataAs<DataGroup>(k_Group3Path) == nullptr);

  const DataPath newGroup3Path = k_Group1Path.createChildPath(k_Group3Name);
  REQUIRE(dataStructure.getDataAs<DataGroup>(newGroup3Path) != nullptr);

  REQUIRE(dataStructure.getDataAs<DataGroup>(k_Group4Path) == nullptr);

  const DataPath newGroup4Path = k_Group1Path.createChildPath(k_Group4Name);
  REQUIRE(dataStructure.getDataAs<DataGroup>(newGroup4Path) != nullptr);
}

TEST_CASE("SimplnxCore::MoveData Unsuccessful", "[Simplnx::Core][MoveData]")
{
  MoveData filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_Group2Path({k_Group2Name});
  const DataPath k_Group3Path({k_Group3Name});

  SECTION("Object already exists in new data path")
  {
    args.insertOrAssign(MoveData::k_Data_Key, std::make_any<std::vector<DataPath>>({k_Group3Path}));
    args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_Group2Path));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  SECTION("Cannot reparent object to itself")
  {
    args.insertOrAssign(MoveData::k_Data_Key, std::make_any<std::vector<DataPath>>({k_Group3Path}));
    args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_Group3Path));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  SECTION("Cannot reparent object to a child object")
  {
    args.insertOrAssign(MoveData::k_Data_Key, std::make_any<std::vector<DataPath>>({k_Group2Path}));
    args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_Group3Path));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
}

TEST_CASE("SimplnxCore::MoveData Tuple Size Mismatches Warning and Failure", "[Simplnx::Core][MoveData]")
{
  MoveData filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_DataArrayPath({k_DataArray1Name});
  const DataPath k_AMPath({k_AM1Name});

  args.insertOrAssign(MoveData::k_Data_Key, std::make_any<std::vector<DataPath>>({k_DataArrayPath}));
  args.insertOrAssign(MoveData::k_NewParent_Key, std::make_any<DataPath>(k_AMPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);

  REQUIRE_FALSE(preflightResult.outputActions.warnings().empty());

  bool found = false;
  for(const auto& warning : preflightResult.outputActions.warnings())
  {
    if(warning.code == ::k_TupleDimMismatchWarningCode)
    {
      found = true;
    }
  }

  REQUIRE(found);

  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
}
