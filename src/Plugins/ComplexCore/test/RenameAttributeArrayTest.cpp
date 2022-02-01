#include <catch2/catch.hpp>

#include "ComplexCore/Filters/RenameAttributeArray.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("RenameAttributeArray(Instantiate)", "[ComplexCore][RenameAttributeArray]")
{
  static constexpr StringLiteral k_NewName = "Bar";
  const DataPath k_DataPath({Constants::k_SmallIN100});

  RenameAttributeArray filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameAttributeArray::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameAttributeArray::k_ArrayPath_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(result.outputActions);
}

TEST_CASE("RenameAttributeArray(Invalid Parameters)", "[ComplexCore][RenameAttributeArray]")
{
  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  RenameAttributeArray filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameAttributeArray::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameAttributeArray::k_ArrayPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(result.result);
}

TEST_CASE("RenameAttributeArray(Valid Parameters)", "[ComplexCore][RenameAttributeArray]")
{
  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  RenameAttributeArray filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameAttributeArray::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameAttributeArray::k_ArrayPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = ds.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);
}
