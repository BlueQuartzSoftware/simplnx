#include "SimplnxCore/Filters/RenameDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::RenameDataAction(Instantiate)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Bar";
  const DataPath k_DataPath({Constants::k_SmallIN100});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("SimplnxCore::RenameDataAction(Invalid Parameters)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
}

TEST_CASE("SimplnxCore::RenameDataAction(Valid Parameters)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = dataStructure.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);
}

TEST_CASE("SimplnxCore::RenameDataAction(Valid Overwrite)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = Constants::k_GroupHName;
  static const DataPath k_DataPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, Constants::k_ArrayIName});

  RenameDataObject filter;
  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  Arguments args;

  args.insert(RenameDataObject::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // There is a warning clause, but under current implementation it won't be reached
  //  bool warningFound = false;
  //  for(const auto& warning : preflightResult.outputActions.warnings())
  //  {
  //    if(warning.code == -6602)
  //    {
  //      warningFound = true;
  //    }
  //  }
  //  REQUIRE(warningFound);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  // Verify rename was successful
  {
    DataPath newPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, k_NewName});
    auto* dataObject = dataStructure.getData(newPath);
    REQUIRE(dataObject != nullptr);

    REQUIRE(dataObject->getName() == k_NewName);
  }

  // Verify old DataGroup (`H`) was removed
  {
    DataPath oldHPath({Constants::k_GroupAName, Constants::k_GroupHName});
    auto* dataObject = dataStructure.getData(oldHPath);
    REQUIRE(dataObject == nullptr);
  }

  // Verify old DataGroup (`H`) sub-array (`N`) was removed
  {
    DataPath oldHChildPath({Constants::k_GroupAName, Constants::k_GroupHName, Constants::k_ArrayNName});
    auto* dataObject = dataStructure.getData(oldHChildPath);
    REQUIRE(dataObject == nullptr);
  }
}

TEST_CASE("SimplnxCore::RenameDataAction(InValid Overwrite)", "[SimplnxCore][RenameDataAction]")
{
  static constexpr StringLiteral k_NewName = Constants::k_GroupDName;
  static const DataPath k_DataPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, Constants::k_ArrayIName});

  RenameDataObject filter;

  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  Arguments args;

  args.insert(RenameDataObject::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObject::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

  bool errorFound = false;
  for(const auto& error : result.result.errors())
  {
    if(error.code == -6601)
    {
      errorFound = true;
    }
  }
  REQUIRE(errorFound);
}
