#include <catch2/catch.hpp>

#include "ComplexCore/Filters/RenameDataObject.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ComplexCore::RenameDataAction(Instantiate)", "[ComplexCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Bar";
  const DataPath k_DataPath({Constants::k_SmallIN100});

  RenameDataObject filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("ComplexCore::RenameDataAction(Invalid Parameters)", "[ComplexCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObject filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(result.result);
}

TEST_CASE("ComplexCore::RenameDataAction(Valid Parameters)", "[ComplexCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObject filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = ds.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);
}
