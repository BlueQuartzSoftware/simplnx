#include <catch2/catch.hpp>

#include "ComplexCore/Filters/SetOriginResolutionImageGeom.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("SetOriginResolutionImageGeom(Instantiate)", "[ComplexCore][SetOriginResolutionImageGeom]")
{
  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = false;
  bool k_ChangeResolution = false;
  std::vector<float64> k_Origin{0, 0, 0};
  std::vector<float64> k_Spacing{1, 1, 1};

  SetOriginResolutionImageGeom filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetOriginResolutionImageGeom::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetOriginResolutionImageGeom::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetOriginResolutionImageGeom::k_ChangeResolution_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetOriginResolutionImageGeom::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
  args.insert(SetOriginResolutionImageGeom::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

  auto result = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("SetOriginResolutionImageGeom(Valid Parameters)", "[ComplexCore][SetOriginResolutionImageGeom]")
{
  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = true;
  bool k_ChangeResolution = true;
  std::vector<float64> k_Origin{7, 6, 5};
  std::vector<float64> k_Spacing{2, 2, 2};

  SetOriginResolutionImageGeom filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetOriginResolutionImageGeom::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetOriginResolutionImageGeom::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetOriginResolutionImageGeom::k_ChangeResolution_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetOriginResolutionImageGeom::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
  args.insert(SetOriginResolutionImageGeom::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = ds.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeom.getOrigin() == FloatVec3{7, 6, 5});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});
}
