#include "SimplnxCore/Filters/RemoveFlaggedVertices.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::RemoveFlaggedVertices: Instantiate", "[SimplnxCore][RemoveFlaggedVertices]")
{
  RemoveFlaggedVertices filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(RemoveFlaggedVertices::k_VertexGeomPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVertices::k_InputMaskPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVertices::k_ReducedVertexGeometryPath_Key, std::make_any<DataPath>());

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("SimplnxCore::RemoveFlaggedVertices: Test Algorithm", "[SimplnxCore][RemoveFlaggedVertices]")
{
  RemoveFlaggedVertices filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);

  std::vector<usize> vertexTupleDims = {100};
  std::vector<usize> vertexCompDims = {3};

  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry, topLevelGroup->getId());
  Float32Array* coords = UnitTest::CreateTestDataArray<float>(dataStructure, "coords", vertexTupleDims, vertexCompDims, vertexGeom->getId());
  vertexGeom->setVertices(*coords); // Add the vertices to the VertexGeom object

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, Constants::k_VertexDataGroupName, vertexTupleDims, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

  Int32Array* slipVector = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_SlipVector, vertexTupleDims, vertexCompDims, vertexAttributeMatrix->getId());
  Int32Array* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, vertexTupleDims, {1}, vertexAttributeMatrix->getId());

  BoolArray* conditionalArray = UnitTest::CreateTestDataArray<bool>(dataStructure, Constants::k_ConditionalArray, vertexTupleDims, {1}, vertexAttributeMatrix->getId());
  conditionalArray->fill(false);
  // initialize the coords just to have something other than 0.0
  // Set the first 25 values of the conditional array to true, thus keeping 75 vertices
  for(size_t i = 0; i < vertexTupleDims[0]; i++)
  {
    coords->initializeTuple(i, static_cast<float>(i));
    slipVector->initializeTuple(i, static_cast<int32>(i));
    featureIds->initializeTuple(i, static_cast<int32>(i));
    if(i < 25)
    {
      conditionalArray->initializeTuple(i, true);
    }
  }

  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_VertexGeometry});
  DataPath vertexAMPath = vertexGeomPath.createChildPath(Constants::k_VertexDataGroupName);
  std::vector<DataPath> arraySelection{vertexAMPath.createChildPath(Constants::k_SlipVector), vertexAMPath.createChildPath(Constants::k_FeatureIds)};
  DataPath maskPath = vertexAMPath.createChildPath(Constants::k_ConditionalArray);
  DataPath reducedVertexPath({Constants::k_SmallIN100, Constants::k_ReducedGeometry});
  DataPath reducedVertexAMPath = reducedVertexPath.createChildPath(Constants::k_VertexDataGroupName);

  args.insertOrAssign(RemoveFlaggedVertices::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(RemoveFlaggedVertices::k_InputMaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(RemoveFlaggedVertices::k_ReducedVertexGeometryPath_Key, std::make_any<DataPath>(reducedVertexPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* reducedVertexGeom = dataStructure.getDataAs<VertexGeom>(reducedVertexPath);

  size_t reducedTupleCount = reducedVertexGeom->getNumberOfVertices();
  REQUIRE(reducedTupleCount == 75);

  Int32Array& reducedFeatureIds = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_FeatureIds));
  REQUIRE((reducedFeatureIds.getNumberOfTuples() == 75));

  Int32Array& reducedSlipVectors = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_SlipVector));
  REQUIRE((reducedSlipVectors.getNumberOfTuples() == 75));

  for(size_t i = 0; i < reducedTupleCount; i++)
  {
    Point3D<float> coord = reducedVertexGeom->getVertexCoordinate(i);
    float compValue = static_cast<float>(i) + 25.0F;
    REQUIRE(((coord[0] == compValue) && (coord[1] == compValue) && (coord[2] == compValue)));

    REQUIRE(reducedFeatureIds[i] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3 + 1] == i + 25);
    REQUIRE(reducedSlipVectors[i * 3 + 2] == i + 25);
  }

  // Write out the DataStructure for later viewing/debugging
  std::string filePath = fmt::format("{}/RemoveFlaggedVertices.dream3d", unit_test::k_BinaryTestOutputDir);
  // std::cout << "Writing file to: " << filePath << std::endl;
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(filePath);
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}
