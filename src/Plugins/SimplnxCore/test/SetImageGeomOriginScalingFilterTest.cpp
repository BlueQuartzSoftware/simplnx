#include "SimplnxCore/Filters/SetImageGeomOriginScalingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter(Instantiate)", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = false;
  bool k_ChangeResolution = false;
  std::vector<float64> k_Origin{0, 0, 0};
  std::vector<float64> k_Spacing{1, 1, 1};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeResolution_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter(Valid Parameters)", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = true;
  bool k_ChangeResolution = true;
  std::vector<float64> k_Origin{7, 6, 5};
  std::vector<float64> k_Spacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeResolution_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float64>>(k_Origin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeom.getOrigin() == FloatVec3{7, 6, 5});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});
}
