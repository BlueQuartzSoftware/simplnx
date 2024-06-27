#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/MultiThresholdObjectsFilter.hpp"
#include "SimplnxCore/Filters/RemoveFlaggedEdgesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
namespace
{

const std::string k_EdgeGeomName("Exemplar Edge Geometry");
const DataPath k_EdgeGeomPath({k_EdgeGeomName});
const DataPath k_MaskPath = k_EdgeGeomPath.createChildPath("Edge Data").createChildPath(Constants::k_Mask);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});

const std::string k_VertexAttrMatName("Vertex Data");
const std::string k_EdgeAttrMatName("Edge Data");

const std::string k_VertArray1Name("vert data 1");
const std::string k_EdgeMaskArray1Name("mask data");
const std::string k_EdgeDataArray1Name("edge data 1");

// const DataPath k_VertexListPath = k_ReducedGeomPath.createChildPath("SharedVertexList");
// const DataPath k_TriangleListPath = k_ReducedGeomPath.createChildPath("SharedTriList");
} // namespace
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedEdgesFilter: Valid Filter Execution", "[SimplnxCore][RemoveFlaggedEdgesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "remove_flagged_triangles_test_1.tar.gz", "remove_flagged_triangles_test_1");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  auto baseDataFilePath = fs::path(fmt::format("{}/remove_flagged_triangles_test_1/remove_flagged_edges_1.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Run a Multi-Threshold Filter
  {
    MultiThresholdObjectsFilter filter;

    Arguments args;

    ArrayThresholdSet arrayThresholdset;
    ArrayThresholdSet::CollectionType thresholds;

    std::shared_ptr<ArrayThreshold> ciThreshold = std::make_shared<ArrayThreshold>();
    ciThreshold->setArrayPath(DataPath({"Exemplar Edge Geometry", "Edge Data", "RegionIds"}));
    ciThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    ciThreshold->setComparisonValue(4);
    thresholds.push_back(ciThreshold);

    arrayThresholdset.setArrayThresholds(thresholds);

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdsParameter::ValueType>(arrayThresholdset));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(Constants::k_Mask));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedEdgesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_EdgeGeomPath));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // This filter did not exist in DREAM.3D Version 6 so there is nothing to compare against.
  // Right now we are going to just check some basic attributes of the Edge Geometry and see
  // if they are the expected value.
  auto& reducedEdgeGeom = dataStructure.getDataRefAs<EdgeGeom>(k_ReducedGeomPath);
  REQUIRE(reducedEdgeGeom.getEdgeAttributeMatrixRef().getSize() == 0);
  REQUIRE(reducedEdgeGeom.getVertexAttributeMatrixRef().getSize() == 0);
  REQUIRE(reducedEdgeGeom.getNumberOfVertices() == 30800);
  REQUIRE(reducedEdgeGeom.getNumberOfEdges() == 15400);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/remove_flagged_edges.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::RemoveFlaggedEdgesFilter:1", "[SimplnxCore][RemoveFlaggedEdgesFilter]")
{

  DataStructure dataStructure;
  size_t numEdges = 25;
  size_t numVertices = 26;
  CreateGeometry1DAction<EdgeGeom> creatEdgeGeomAction(k_EdgeGeomPath, numEdges, numVertices, k_VertexAttrMatName, k_EdgeAttrMatName, CreateGeometry1DAction<EdgeGeom>::k_DefaultVerticesName,
                                                       CreateGeometry1DAction<EdgeGeom>::k_DefaultEdgesName);
  creatEdgeGeomAction.apply(dataStructure, IDataAction::Mode::Execute);
  auto& edgeGeom = dataStructure.getDataRefAs<EdgeGeom>(k_EdgeGeomPath);

  DataPath vertAMPath = k_EdgeGeomPath.createChildPath(k_VertexAttrMatName);
  auto& vertexAM = edgeGeom.getVertexAttributeMatrixRef();

  DataPath edgeAMPath = k_EdgeGeomPath.createChildPath(k_EdgeAttrMatName);
  auto& edgeAM = edgeGeom.getEdgeAttributeMatrixRef();

  CreateArrayAction vertArray1Action(DataType::int32, vertexAM.getShape(), {1ULL}, vertAMPath.createChildPath(k_VertArray1Name));
  vertArray1Action.apply(dataStructure, IDataAction::Mode::Execute);

  CreateArrayAction edgeMaskArrayAction(DataType::uint8, edgeAM.getShape(), {1ULL}, edgeAMPath.createChildPath(k_EdgeMaskArray1Name));
  edgeMaskArrayAction.apply(dataStructure, IDataAction::Mode::Execute);

  CreateArrayAction edgeDataArrayAction(DataType::uint32, edgeAM.getShape(), {1ULL}, edgeAMPath.createChildPath(k_EdgeDataArray1Name));
  edgeDataArrayAction.apply(dataStructure, IDataAction::Mode::Execute);

  // Assign Coordinates to the Vertices
  //
  auto& vertDataArray = dataStructure.getDataRefAs<Int32Array>(vertAMPath.createChildPath(k_VertArray1Name));
  auto& sharedVertexList = edgeGeom.getVerticesRef();
  float32 radius = 10.0f;
  for(usize i = 0; i < numVertices; i++)
  {
    double angle = 2.0 * M_PI * i / numVertices; // Angle in radians

    sharedVertexList[i * 3] = radius * cos(angle);
    sharedVertexList[i * 3 + 1] = radius * sin(angle);
    sharedVertexList[i * 3 + 2] = 0.0f; // radius * cos(phi);

    // We are just creating some data
    vertDataArray[i] = static_cast<int32>(sin(angle) * 10.0f);
  }

  // Assign values to the shared edge list
  auto& edgeDataArray = dataStructure.getDataRefAs<UInt32Array>(edgeAMPath.createChildPath(k_EdgeDataArray1Name));
  auto& edgeMaskDataArray = dataStructure.getDataRefAs<UInt8Array>(edgeAMPath.createChildPath(k_EdgeMaskArray1Name));
  auto& sharedEdgeList = edgeGeom.getEdgesRef();
  for(usize i = 1; i < numEdges; i++)
  {
    sharedEdgeList[i * 2] = i - 1;
    sharedEdgeList[i * 2 + 1] = i;

    edgeMaskDataArray[i] = (i % 2 == 0 ? 0 : 1);
    edgeDataArray[i] = i * 100;
  }

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedEdgesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_EdgeGeomPath));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(edgeAMPath.createChildPath(k_EdgeMaskArray1Name)));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_VertexDataHandling_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_VertexDataSelectedAttributeMatrix_Key, std::make_any<AttributeMatrixSelectionParameter::ValueType>(vertAMPath));

    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_EdgeDataHandling_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_EdgeDataSelectedAttributeMatrix_Key, std::make_any<AttributeMatrixSelectionParameter::ValueType>(edgeAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/remove_flagged_edges_1.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif
}
