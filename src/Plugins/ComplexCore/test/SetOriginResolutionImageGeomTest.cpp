#include <catch2/catch.hpp>

#include "ComplexCore/Filters/SetOriginResolutionImageGeom.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("SetOriginResolutionImageGeom(Instantiate)", "[ComplexCore][SetOriginResolutionImageGeom]")
{
  //static constexpr StringLiteral k_ImageGeom = "Bar";
  //static constexpr StringLiteral k_ChangeOrigin = false;
  //const DataPath k_DataPath({Constants::k_SmallIN100});

  SetOriginResolutionImageGeom filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetOriginResolutionImageGeom::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeom));
  args.insert(SetOriginResolutionImageGeom::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetOriginResolutionImageGeom::k_ChangeResolution_Key, std::make_any<bool>(k_DataPath));
  args.insert(SetOriginResolutionImageGeom::k_Origin_Key, std::make_any<std::vector<float64>>(k_DataPath));
  args.insert(SetOriginResolutionImageGeom::k_Spacing_Key, std::make_any<std::vector<float64>>(k_DataPath));

  auto result = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("SetOriginResolutionImageGeom(Invalid Parameters)", "[ComplexCore][SetOriginResolutionImageGeom]")
{
  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  SetOriginResolutionImageGeom filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetOriginResolutionImageGeom::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(SetOriginResolutionImageGeom::k_DataGroup_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(result.result);
}

TEST_CASE("SetOriginResolutionImageGeom(Valid Parameters)", "[ComplexCore][SetOriginResolutionImageGeom]")
{
  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  SetOriginResolutionImageGeom filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetOriginResolutionImageGeom::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(SetOriginResolutionImageGeom::k_DataGroup_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = ds.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);
}
