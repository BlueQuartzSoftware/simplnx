#include <catch2/catch.hpp>

#include "ComplexCore/Filters/IdentifySample.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ComplexCore::IdentifySample(Instantiate)", "[ComplexCore][IdentifySample]")
{
  static constexpr bool k_FillHoles = true;
  static const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  static const std::string k_MaskName = "Mask";
  static const DataPath k_MaskArrayPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_MaskName});

  IdentifySample filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  auto scanDataId = dataGraph.getId(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData})).value();
  UInt8Array* maskArray = UnitTest::CreateTestDataArray<uint8>(dataGraph, k_MaskName, {80, 60, 40}, {1}, scanDataId);
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
