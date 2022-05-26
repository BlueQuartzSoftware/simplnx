#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RemoveFlaggedVertices.hpp"

using namespace complex;

TEST_CASE("ComplexCore::RemoveFlaggedVertices: Instantiate", "[ComplexCore][RemoveFlaggedVertices]")
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

TEST_CASE("ComplexCore::RemoveFlaggedVertices: Test Algorithm", "[ComplexCore][RemoveFlaggedVertices]")
{
  RemoveFlaggedVertices filter;
  DataStructure dataGraph;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataGraph, Constants::k_SmallIN100);

  std::vector<usize> vertexTupleDims = {100};
  std::vector<usize> vertexCompDims = {3};

  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeom = VertexGeom::Create(dataGraph, Constants::k_VertexGeometry, topLevelGroup->getId());
  Float32Array* coords = UnitTest::CreateTestDataArray<float>(dataGraph, "coords", vertexTupleDims, vertexCompDims, vertexGeom->getId());
  vertexGeom->setVertices(*coords); // Add the vertices to the VertexGeom object

  Int32Array* slipVector = UnitTest::CreateTestDataArray<int32>(dataGraph, Constants::k_SlipVector, vertexTupleDims, vertexCompDims, vertexGeom->getId());
  Int32Array* featureIds = UnitTest::CreateTestDataArray<int32>(dataGraph, Constants::k_FeatureIds, vertexTupleDims, {1}, vertexGeom->getId());

  BoolArray* conditionalArray = UnitTest::CreateTestDataArray<bool>(dataGraph, Constants::k_ConditionalArray, vertexTupleDims, {1}, vertexGeom->getId());
  conditionalArray->fill(true);
  // initialize the coords just to have something other than 0.0
  // Set the first 25 values of the conditional array to false, thus keeping 75 vertices
  for(size_t i = 0; i < vertexTupleDims[0]; i++)
  {
    coords->initializeTuple(i, static_cast<float>(i));
    slipVector->initializeTuple(i, static_cast<int32>(i));
    featureIds->initializeTuple(i, static_cast<int32>(i));
    if(i < 25)
    {
      conditionalArray->initializeTuple(i, false);
    }
  }

  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_VertexGeometry});
  std::vector<DataPath> arraySelection{vertexGeomPath.createChildPath(Constants::k_SlipVector), vertexGeomPath.createChildPath(Constants::k_FeatureIds)};
  DataPath maskPath({Constants::k_SmallIN100, Constants::k_VertexGeometry, Constants::k_ConditionalArray});
  DataPath reducedVertexPath({Constants::k_SmallIN100, Constants::k_ReducedGeometry});

  args.insertOrAssign(RemoveFlaggedVertices::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(RemoveFlaggedVertices::k_ArraySelection_Key, std::make_any<std::vector<DataPath>>(arraySelection));
  args.insertOrAssign(RemoveFlaggedVertices::k_MaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(RemoveFlaggedVertices::k_ReducedVertexPath_Key, std::make_any<DataPath>(reducedVertexPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* reducedVertexGeom = dataGraph.getDataAs<VertexGeom>(reducedVertexPath);

  size_t reducedTupleCount = reducedVertexGeom->getNumberOfVertices();
  REQUIRE(reducedTupleCount == 75);

  Int32Array& reducedFeatureIds = dataGraph.getDataRefAs<Int32Array>(reducedVertexPath.createChildPath(Constants::k_FeatureIds));
  REQUIRE((reducedFeatureIds.getNumberOfTuples() == 75));

  Int32Array& reducedSlipVectors = dataGraph.getDataRefAs<Int32Array>(reducedVertexPath.createChildPath(Constants::k_SlipVector));
  REQUIRE((reducedSlipVectors.getNumberOfTuples() == 75));

  for(size_t i = 0; i < reducedTupleCount; i++)
  {
    Point3D<float> coord = reducedVertexGeom->getCoords(i);
    float compValue = static_cast<float>(i) + 25.0F;
    REQUIRE(((coord[0] == compValue) && (coord[1] == compValue) && (coord[2] == compValue)));

    REQUIRE(reducedFeatureIds[i] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3 + 1] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3 + 2] == i + 25);
  }

  // Write out the DataStructure for later viewing/debugging
  std::string filePath = fmt::format("{}/RemoveFlaggedVertices.dream3d", unit_test::k_BinaryDir);
  // std::cout << "Writing file to: " << filePath << std::endl;
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
  H5::FileWriter fileWriter = std::move(result.value());
  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}
