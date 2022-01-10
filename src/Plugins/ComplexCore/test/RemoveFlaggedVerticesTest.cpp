#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/Filters/RemoveFlaggedVertices.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;

TEST_CASE("RemoveFlaggedVertices: Instantiate", "[ComplexCore][RemoveFlaggedVertices]")
{
  RemoveFlaggedVertices filter;
  DataStructure dataGraph;
  Arguments args;

  args.insertOrAssign(RemoveFlaggedVertices::k_VertexGeomPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVertices::k_ArraySelection_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));
  args.insertOrAssign(RemoveFlaggedVertices::k_MaskPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVertices::k_ReducedVertexPath_Key, std::make_any<DataPath>());

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("RemoveFlaggedVertices: Test Algorithm", "[ComplexCore][RemoveFlaggedVertices]")
{
  RemoveFlaggedVertices filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  auto* vertexArray = dataGraph.getDataAs<Float32Array>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConfidenceIndex}));
  auto* vertexGeom = dataGraph.getDataAs<VertexGeom>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry}));
  vertexGeom->setVertices(vertexArray);

  DataPath ipfColorsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "IPF Colors"});
  DataPath eulersPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Euler"});

  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  std::vector<DataPath> arraySelection{ipfColorsPath, eulersPath};
  DataPath maskPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "ConditionalArray"});
  DataPath reducedVertexPath({"Reduced Vertex Geom"});

  args.insertOrAssign(RemoveFlaggedVertices::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(RemoveFlaggedVertices::k_ArraySelection_Key, std::make_any<std::vector<DataPath>>(arraySelection));
  args.insertOrAssign(RemoveFlaggedVertices::k_MaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(RemoveFlaggedVertices::k_ReducedVertexPath_Key, std::make_any<DataPath>(reducedVertexPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
}
