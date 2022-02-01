#include <catch2/catch.hpp>

#include "ComplexCore/Filters/RenameDataGroup.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("RenameDataAction(Instantiate)", "[ComplexCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Bar";
  const DataPath k_DataPath({Constants::k_SmallIN100});

  RenameDataGroup filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataGroup::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataGroup::k_DataGroup_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("RenameDataAction(Invalid Parameters)", "[ComplexCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataGroup filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataGroup::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataGroup::k_DataGroup_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(result.result);
}

TEST_CASE("RenameDataAction(Valid Parameters)", "[ComplexCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataGroup filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataGroup::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataGroup::k_DataGroup_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = ds.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);
}
