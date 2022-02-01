#include <catch2/catch.hpp>

#include "ComplexCore/Filters/IdentifySample.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("IdentifySample(Instantiate)", "[ComplexCore][IdentifySample]")
{
  static constexpr bool k_FillHoles = true;
  static const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  static const DataPath k_MaskArrayPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "IPF Colors"});

  IdentifySample filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(IdentifySample::k_FillHoles_Key, std::make_any<bool>(k_FillHoles));
  args.insert(IdentifySample::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(IdentifySample::k_GoodVoxels_Key, std::make_any<DataPath>(k_MaskArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}
