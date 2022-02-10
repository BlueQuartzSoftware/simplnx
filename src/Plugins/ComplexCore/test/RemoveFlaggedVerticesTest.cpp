#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"


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

  auto* maskArray = dataGraph.getDataAs<BoolArray>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConditionalArray}));
  // Fill the mask array with every other value being true
  size_t totalElements = vertexGeom->getNumberOfVertices();
  bool value = true;
  for(size_t i = 0; i < totalElements; i++)
  {
    (*maskArray)[i] = value;
    value = !value;
  }

  DataPath ipfColorsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "IPF Colors"});
  DataPath eulersPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Euler"});

  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  std::vector<DataPath> arraySelection{ipfColorsPath, eulersPath};
  DataPath maskPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConditionalArray});
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

  auto* reducedVertexGeom = dataGraph.getDataAs<VertexGeom>(reducedVertexPath);

  size_t reducedTupleCount = reducedVertexGeom->getNumberOfVertices();
  REQUIRE(reducedTupleCount == 96000);

#if 1
  // Write out the DataStructure for later viewing/debugging
  std::string filePath = fmt::format("{}/RemoveFlaggedVertices.dream3d", unit_test::k_BinaryDir);
  // std::cout << "Writing file to: " << filePath << std::endl;
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
#endif
}
