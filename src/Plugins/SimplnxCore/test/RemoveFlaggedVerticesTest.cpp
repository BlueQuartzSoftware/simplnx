#include "SimplnxCore/Filters/RemoveFlaggedVerticesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_elements_data/remove_flagged_vertices_data.dream3d", nx::core::unit_test::k_TestFilesDir));

constexpr StringLiteral k_CopyTestName = "copy_test";
constexpr StringLiteral k_DataName = "data";
constexpr StringLiteral k_VertexListName = "SharedVertexList";

const DataPath k_VertexGeomPath({"VertexGeometry"});
const DataPath k_MaskPath = k_VertexGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(Constants::k_Mask);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});
const DataPath k_ExemplarReducedGeomPath({"ExemplarReducedGeometry"});

const DataPath k_VertexListPath = k_ReducedGeomPath.createChildPath("SharedVertexList");
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Instantiate", "[SimplnxCore][RemoveFlaggedVerticesFilter]")
{
  RemoveFlaggedVerticesFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>());

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: From Scratch", "[SimplnxCore][RemoveFlaggedVerticesFilter]")
{
  RemoveFlaggedVerticesFilter filter;
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

  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(reducedVertexPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* reducedVertexGeom = dataStructure.getDataAs<VertexGeom>(reducedVertexPath);

  size_t reducedTupleCount = reducedVertexGeom->getNumberOfVertices();
  REQUIRE(reducedTupleCount == 75);

  auto& reducedFeatureIds = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_FeatureIds));
  REQUIRE((reducedFeatureIds.getNumberOfTuples() == 75));

  auto& reducedSlipVectors = dataStructure.getDataRefAs<Int32Array>(reducedVertexAMPath.createChildPath(Constants::k_SlipVector));
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
  std::string filePath = fmt::format("{}/RemoveFlaggedVerticesFilter.dream3d", unit_test::k_BinaryTestOutputDir);
  // std::cout << "Writing file to: " << filePath << std::endl;
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(filePath);
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("SimplnxCore::RemoveFlaggedVerticesFilter: Test Algorithm", "[SimplnxCore][RemoveFlaggedVerticesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "remove_flagged_elements_data.tar.gz", "remove_flagged_elements_data");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedVerticesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(::k_VertexGeomPath));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_InputMaskPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedVerticesFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(::k_DataName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(::k_DataName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(::k_CopyTestName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath(::k_CopyTestName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_VertexListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_VertexListName);

    UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }
}
