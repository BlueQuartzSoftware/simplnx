#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CopyDataGroup.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ComplexCore::CopyDataGroup(Instantiate)", "[ComplexCore][CopyDataGroup]")
{
  static const DataPath k_DataPath({Constants::k_SmallIN100, "Phase Data"});
  static const DataPath k_CopyPath({Constants::k_SmallIN100, "Copy Data Bar"});

  CopyDataGroup filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(CopyDataGroup::k_DataPath_Key, std::make_any<DataPath>(k_DataPath));
  args.insert(CopyDataGroup::k_NewPath_Key, std::make_any<DataPath>(k_CopyPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CopyDataGroup(Invalid Parameters)", "[ComplexCore][CopyDataGroup]")
{
  CopyDataGroup filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  SECTION("Section1")
  {
    const DataPath dataPath({Constants::k_SmallIN100, "Bad Data Foo"});
    const DataPath copyPath({Constants::k_SmallIN100, "Copy Data"});

    args.insert(CopyDataGroup::k_DataPath_Key, std::make_any<DataPath>(dataPath));
    args.insert(CopyDataGroup::k_NewPath_Key, std::make_any<DataPath>(copyPath));

    auto result = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("Section2")
  {
    const DataPath dataPath({Constants::k_SmallIN100, "Phase Data"});
    const DataPath copyPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});

    args.insert(CopyDataGroup::k_DataPath_Key, std::make_any<DataPath>(dataPath));
    args.insert(CopyDataGroup::k_NewPath_Key, std::make_any<DataPath>(copyPath));

    auto result = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
}
