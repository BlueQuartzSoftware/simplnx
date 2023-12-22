#include "SimplnxCore/Filters/RenameDataObject.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::RenameDataAction(Instantiate)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Bar";
  const DataPath k_DataPath({Constants::k_SmallIN100});

  RenameDataObject filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("SimplnxCore::RenameDataAction(Invalid Parameters)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObject filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
}

TEST_CASE("SimplnxCore::RenameDataAction(Valid Parameters)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObject filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = dataStructure.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);
}
