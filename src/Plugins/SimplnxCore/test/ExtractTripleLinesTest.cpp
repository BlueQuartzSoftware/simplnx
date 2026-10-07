#include "SimplnxCore/Filters/Algorithms/ExtractTripleLines.hpp"
#include "SimplnxCore/Filters/ExtractTripleLinesFilter.hpp"
#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_TriangleGeomPath({"TriangleGeom"});
const DataPath k_TripleLineGeomPath({"Triple Lines"});
const std::string k_VertexDataName = "Vertex Data";
const std::string k_FaceDataName = "Face Data";
const std::string k_NodeTypesName = "NodeTypes";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_NumFeaturesName = "NumFeatures";

const DataPath k_FaceLabelsPath = k_TriangleGeomPath.createChildPath(k_FaceDataName).createChildPath(k_FaceLabelsName);
const DataPath k_NodeTypesPath = k_TriangleGeomPath.createChildPath(k_VertexDataName).createChildPath(k_NodeTypesName);
const DataPath k_OutNumFeaturesPath = k_TripleLineGeomPath.createChildPath(INodeGeometry1D::k_EdgeAttributeMatrixName).createChildPath(k_NumFeaturesName);
const DataPath k_OutNodeTypesPath = k_TripleLineGeomPath.createChildPath(INodeGeometry0D::k_VertexAttributeMatrixName).createChildPath(k_NodeTypesName);

/**
 * @brief Creates four unit cells whose Features meet along one vertical quadruple line.
 * @param dataStructure Receives the Image Geometry and Feature Ids.
 */
void BuildFourGrainBlock(DataStructure& dataStructure)
{
  const std::string imageGeomName = "ImageGeom";
  const std::string cellDataName = "Cell Data";
  const std::string featureIdsName = "FeatureIds";
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, imageGeomName);
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 1});
  imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, cellDataName, ShapeType{1, 2, 2}, imageGeomPtr->getId());
  REQUIRE(cellAmPtr != nullptr);
  imageGeomPtr->setCellData(*cellAmPtr);

  const DataPath featureIdsPath({imageGeomName, cellDataName, featureIdsName});
  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureIdsPath, {1, 2, 2}, {1});
  auto* featureIdsPtr = Int32Array::Create(dataStructure, featureIdsName, featureIdsStore, cellAmPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();
  featureIdsRef[0] = 1;
  featureIdsRef[1] = 2;
  featureIdsRef[2] = 3;
  featureIdsRef[3] = 4;
}

/**
 * @brief Runs QuickSurfaceMesh over the shared 2x2x1 four-grain block, leaving a TriangleGeom at
 * k_TriangleGeomPath with its FaceLabels and NodeTypes.
 * @param dataStructure Receives the mesh and its source image.
 */
void RunQuickSurfaceMesh(DataStructure& dataStructure)
{
  BuildFourGrainBlock(dataStructure);

  QuickSurfaceMeshFilter filter;
  Arguments args;
  args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Cell Data", "FeatureIds"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_BoundingBoxSkinMode_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

enum class NodeTypeCopy : uint8
{
  Enabled,
  Disabled
};

/**
 * @brief Runs extraction on the fixture mesh and validates preflight.
 * @param dataStructure Contains the source mesh and receives the output geometry.
 * @param includeExterior Whether negative Feature Ids participate in edge selection.
 * @param nodeTypeCopy Whether source NodeTypes are copied to output vertices.
 * @return Filter execution result.
 */
IFilter::ExecuteResult RunExtractTripleLines(DataStructure& dataStructure, bool includeExterior = false, NodeTypeCopy nodeTypeCopy = NodeTypeCopy::Enabled)
{
  ExtractTripleLinesFilter filter;
  Arguments args;
  args.insertOrAssign(ExtractTripleLinesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(ExtractTripleLinesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  if(nodeTypeCopy == NodeTypeCopy::Enabled)
  {
    args.insertOrAssign(ExtractTripleLinesFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  }
  args.insertOrAssign(ExtractTripleLinesFilter::k_CopyNodeTypes_Key, std::make_any<bool>(nodeTypeCopy == NodeTypeCopy::Enabled));
  args.insertOrAssign(ExtractTripleLinesFilter::k_IncludeExteriorTripleLines_Key, std::make_any<bool>(includeExterior));
  args.insertOrAssign(ExtractTripleLinesFilter::k_CreatedTripleLineGeometryPath_Key, std::make_any<DataPath>(k_TripleLineGeomPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  return filter.execute(dataStructure, args);
}
/**
 * @brief Runs SurfaceNets over the same four-grain block, emitting into the same names as
 * RunQuickSurfaceMesh so one extraction helper serves both. Smoothing is off so the geometry
 * stays predictable.
 * @param dataStructure Receives the mesh and its source image.
 */
void RunSurfaceNets(DataStructure& dataStructure)
{
  BuildFourGrainBlock(dataStructure);

  SurfaceNetsFilter filter;
  Arguments args;
  args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
  args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
  args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
  args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Cell Data", "FeatureIds"})));
  args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataName));
  args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypesName));
  args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataName));
  args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_FaceLabelsName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

/**
 * @brief Runs M3CSurfaceMeshing over the same four-grain block, emitting into the same names.
 * @param dataStructure Receives the mesh and its source image.
 */
void RunM3CSurfaceMeshing(DataStructure& dataStructure)
{
  BuildFourGrainBlock(dataStructure);

  M3CSurfaceMeshingFilter filter;
  Arguments args;
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Cell Data", "FeatureIds"})));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypesName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataName));
  args.insertOrAssign(M3CSurfaceMeshingFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_FaceLabelsName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Quadruple point line from a four-grain block", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  RunQuickSurfaceMesh(dataStructure);

  auto executeResult = RunExtractTripleLines(dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
  const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath);

  // Four grains meet along the single interior grid edge, so exactly one segment bordering four
  // unique Feature Ids - a quadruple point line.
  REQUIRE(tripleLineGeom.getNumberOfEdges() == 1);
  REQUIRE(tripleLineGeom.getNumberOfVertices() == 2);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath));
  const auto& numFeatures = dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath);
  REQUIRE(numFeatures.getNumberOfTuples() == 1);
  REQUIRE(numFeatures[0] == 4);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Include Exterior Triple Lines", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  usize interiorOnlyEdges = 0;
  {
    DataStructure dataStructure;
    RunQuickSurfaceMesh(dataStructure);
    auto executeResult = RunExtractTripleLines(dataStructure, false);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
    interiorOnlyEdges = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath).getNumberOfEdges();
  }

  DataStructure dataStructure;
  RunQuickSurfaceMesh(dataStructure);
  auto executeResult = RunExtractTripleLines(dataStructure, true);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
  const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath);

  // At x=1, z=0 and z=1 each contribute two boundary segments; y=0 and y=2 each contribute one vertical segment.
  // The plane y=1 contributes six more. The interior quadruple line adds one: 13 total, with 12 triples and one quadruple.
  REQUIRE(interiorOnlyEdges == 1);
  REQUIRE(tripleLineGeom.getNumberOfEdges() == 13);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath));
  const auto& numFeatures = dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath);
  usize tripleCount = 0;
  usize quadrupleCount = 0;
  for(usize edgeIdx = 0; edgeIdx < numFeatures.getNumberOfTuples(); edgeIdx++)
  {
    tripleCount += numFeatures[edgeIdx] == 3 ? 1 : 0;
    quadrupleCount += numFeatures[edgeIdx] == 4 ? 1 : 0;
  }
  REQUIRE(tripleCount == 12);
  REQUIRE(quadrupleCount == 1);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: NodeTypes are carried onto the created vertices", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  RunQuickSurfaceMesh(dataStructure);
  auto executeResult = RunExtractTripleLines(dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
  const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNodeTypesPath));
  const auto& outNodeTypes = dataStructure.getDataRefAs<Int8Array>(k_OutNodeTypesPath);

  // Downstream filters require one NodeTypes tuple per output vertex.
  REQUIRE(outNodeTypes.getNumberOfTuples() == tripleLineGeom.getNumberOfVertices());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_NodeTypesPath));
  const auto& sourceGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
  const auto& sourceNodeTypes = dataStructure.getDataRefAs<Int8Array>(k_NodeTypesPath);
  const auto& sourceVertices = sourceGeom.getVertices()->getDataStoreRef();
  const auto& outputVertices = tripleLineGeom.getVertices()->getDataStoreRef();
  for(usize vertexIdx = 0; vertexIdx < outNodeTypes.getNumberOfTuples(); vertexIdx++)
  {
    bool foundMatch = false;
    for(usize sourceIdx = 0; sourceIdx < sourceGeom.getNumberOfVertices(); sourceIdx++)
    {
      if(outputVertices[3 * vertexIdx] == sourceVertices[3 * sourceIdx] && outputVertices[(3 * vertexIdx) + 1] == sourceVertices[(3 * sourceIdx) + 1] &&
         outputVertices[(3 * vertexIdx) + 2] == sourceVertices[(3 * sourceIdx) + 2])
      {
        REQUIRE(outNodeTypes[vertexIdx] == sourceNodeTypes[sourceIdx]);
        foundMatch = true;
        break;
      }
    }
    REQUIRE(foundMatch);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Preflight rejects mismatched input arrays", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  RunQuickSurfaceMesh(dataStructure);

  ExtractTripleLinesFilter filter;
  Arguments args;
  args.insertOrAssign(ExtractTripleLinesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(ExtractTripleLinesFilter::k_IncludeExteriorTripleLines_Key, std::make_any<bool>(false));
  args.insertOrAssign(ExtractTripleLinesFilter::k_CreatedTripleLineGeometryPath_Key, std::make_any<DataPath>(k_TripleLineGeomPath));

  args.insertOrAssign(ExtractTripleLinesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(ExtractTripleLinesFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(ExtractTripleLinesFilter::k_CopyNodeTypes_Key, std::make_any<bool>(true));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);

  SECTION("Face Labels tuple count mismatch")
  {
    auto* wrongLabelsPtr = Int32Array::Create(dataStructure, "WrongLabels", std::make_unique<DataStore<int32>>(std::vector<usize>{triangleGeom.getNumberOfFaces() + 1}, std::vector<usize>{2}, 0));
    REQUIRE(wrongLabelsPtr != nullptr);
    args.insertOrAssign(ExtractTripleLinesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"WrongLabels"})));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -57402);
  }
  SECTION("Node Types tuple count mismatch")
  {
    auto* wrongNodeTypesPtr =
        Int8Array::Create(dataStructure, "WrongNodeTypes", std::make_unique<DataStore<int8>>(std::vector<usize>{triangleGeom.getNumberOfVertices() + 1}, std::vector<usize>{1}, 0));
    REQUIRE(wrongNodeTypesPtr != nullptr);
    args.insertOrAssign(ExtractTripleLinesFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(DataPath({"WrongNodeTypes"})));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -57403);

    args.insertOrAssign(ExtractTripleLinesFilter::k_CopyNodeTypes_Key, std::make_any<bool>(false));
    auto disabledPreflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(disabledPreflightResult.outputActions);
  }
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Copy Node Types off creates only NumFeatures", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  RunQuickSurfaceMesh(dataStructure);
  auto executeResult = RunExtractTripleLines(dataStructure, false, NodeTypeCopy::Disabled);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath));
  REQUIRE(dataStructure.getDataAs<Int8Array>(k_OutNumFeaturesPath)->getNumberOfTuples() == 1);
  REQUIRE(dataStructure.getData(k_OutNodeTypesPath) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Consistent across surface meshers", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  // Segment counts depend on node placement: QuickSurfaceMesh uses voxel corners; SurfaceNets uses cell centers; M3C uses edge, face and body centers.
  // Check only properties that all three meshers share.
  struct MesherCase
  {
    const char* Name;
    void (*Run)(DataStructure&);
  };
  const std::array<MesherCase, 3> mesherCases = {MesherCase{"QuickSurfaceMesh", &RunQuickSurfaceMesh}, MesherCase{"SurfaceNets", &RunSurfaceNets},
                                                 MesherCase{"M3CSurfaceMeshing", &RunM3CSurfaceMeshing}};

  for(const auto& mesherCase : mesherCases)
  {
    INFO(mesherCase.Name);
    DataStructure dataStructure;
    mesherCase.Run(dataStructure);

    auto executeResult = RunExtractTripleLines(dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
    const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath));
    const auto& numFeatures = dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath);

    // 1. A junction exists in this input, so every mesher must find at least one segment.
    INFO(mesherCase.Name << " produced " << tripleLineGeom.getNumberOfEdges() << " triple line segments");
    REQUIRE(tripleLineGeom.getNumberOfEdges() > 0);

    // Counts saturate at 4, which means four or more Features.
    bool sawQuadruplePoint = false;
    for(usize i = 0; i < numFeatures.getNumberOfTuples(); i++)
    {
      INFO(mesherCase.Name << " segment " << i << " reported NumFeatures " << static_cast<int32>(numFeatures[i]));
      REQUIRE((numFeatures[i] == 3 || numFeatures[i] == 4));
      if(numFeatures[i] == 4)
      {
        sawQuadruplePoint = true;
      }
    }

    // 3. Four grains meet in this input, so each mesher must find the quadruple point line.
    REQUIRE(sawQuadruplePoint);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

// Regression guard for #1718: M3C must not offset the mesh by its ghost shell.
// That defect placed a vertex at z=-0.5 below the input domain [0,1].
TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: M3C triple line vertices stay within the domain bounds", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  RunM3CSurfaceMeshing(dataStructure);
  auto executeResult = RunExtractTripleLines(dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
  const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath);
  constexpr float32 k_Epsilon = 0.0001f;
  const auto& vertices = tripleLineGeom.getVertices()->getDataStoreRef();
  for(usize vertexIdx = 0; vertexIdx < tripleLineGeom.getNumberOfVertices(); vertexIdx++)
  {
    const float32 zCoord = vertices[(vertexIdx * 3) + 2];
    CAPTURE(vertexIdx, zCoord);
    REQUIRE(zCoord >= -k_Epsilon);
    REQUIRE(zCoord <= 1.0f + k_Epsilon);
  }
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: M3C splits the quadruple line at the mid-plane", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  // M3C places nodes at cell edge, face and body centers. The vertical quadruple line through
  // the shared corner therefore has two collinear segments, with a split at z=0.5.
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  RunM3CSurfaceMeshing(dataStructure);
  auto executeResult = RunExtractTripleLines(dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
  REQUIRE(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath).getNumberOfEdges() == 2);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath));
  const auto& numFeatures = dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath);
  REQUIRE(numFeatures.getNumberOfTuples() == 2);
  REQUIRE(numFeatures[0] == 4);
  REQUIRE(numFeatures[1] == 4);
}

namespace
{
const std::string k_AlgorithmTriangleGeomName = "TriangleGeom";
const std::string k_AlgorithmFaceLabelsName = "FaceLabels";
const std::string k_AlgorithmTripleLineGeomName = "TripleLines";
const std::string k_AlgorithmNumFeaturesName = "NumFeatures";
const std::string k_AlgorithmNodeTypesName = "NodeTypes";

/**
 * @brief Builds a TriangleGeom plus its FaceLabels array from explicit vertex, triangle and
 * label lists. The FaceLabels array is a child of the geometry.
 * @param dataStructure Receives the geometry and its arrays.
 * @param vertices Source vertex coordinates.
 * @param triangles Source vertex indices for each triangle.
 * @param faceLabels Feature Id pair for each triangle.
 * @return Created source geometry.
 */
TriangleGeom* CreateTriangleMesh(DataStructure& dataStructure, const std::vector<std::array<float32, 3>>& vertices, const std::vector<std::array<usize, 3>>& triangles,
                                 const std::vector<std::array<int32, 2>>& faceLabels)
{
  REQUIRE(triangles.size() == faceLabels.size());

  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, k_AlgorithmTriangleGeomName);
  REQUIRE(triangleGeomPtr != nullptr);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{vertices.size()}, std::vector<usize>{3}, 0.0f);
  auto* vertexArrayPtr = IGeometry::SharedVertexList::Create(dataStructure, "SharedVertexList", std::move(vertexStore), triangleGeomPtr->getId());
  REQUIRE(vertexArrayPtr != nullptr);
  auto& verticesRef = vertexArrayPtr->getDataStoreRef();
  for(usize i = 0; i < vertices.size(); i++)
  {
    verticesRef[(i * 3) + 0] = vertices[i][0];
    verticesRef[(i * 3) + 1] = vertices[i][1];
    verticesRef[(i * 3) + 2] = vertices[i][2];
  }
  triangleGeomPtr->setVertices(*vertexArrayPtr);

  auto faceStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{triangles.size()}, std::vector<usize>{3}, 0);
  auto* faceArrayPtr = IGeometry::SharedFaceList::Create(dataStructure, "SharedTriList", std::move(faceStore), triangleGeomPtr->getId());
  REQUIRE(faceArrayPtr != nullptr);
  auto& facesRef = faceArrayPtr->getDataStoreRef();
  for(usize i = 0; i < triangles.size(); i++)
  {
    facesRef[(i * 3) + 0] = triangles[i][0];
    facesRef[(i * 3) + 1] = triangles[i][1];
    facesRef[(i * 3) + 2] = triangles[i][2];
  }
  triangleGeomPtr->setFaceList(*faceArrayPtr);

  // Every source vertex gets a NodeTypes value. ExtractTripleLinesAlgorithm copies these through to the
  // output vertices; it never reads them to decide which edges are triple lines.
  auto nodeTypeStore = std::make_unique<DataStore<int8>>(std::vector<usize>{vertices.size()}, std::vector<usize>{1}, 0);
  auto* nodeTypeArrayPtr = Int8Array::Create(dataStructure, k_AlgorithmNodeTypesName, std::move(nodeTypeStore), triangleGeomPtr->getId());
  REQUIRE(nodeTypeArrayPtr != nullptr);
  auto& nodeTypesRef = nodeTypeArrayPtr->getDataStoreRef();
  for(usize i = 0; i < vertices.size(); i++)
  {
    // Distinct, recognisable values so a copy-through bug is visible rather than masked by zeros.
    nodeTypesRef[i] = static_cast<int8>(2 + (i % 3));
  }

  auto labelStore = std::make_unique<DataStore<int32>>(std::vector<usize>{faceLabels.size()}, std::vector<usize>{2}, 0);
  auto* labelArrayPtr = Int32Array::Create(dataStructure, k_AlgorithmFaceLabelsName, std::move(labelStore), triangleGeomPtr->getId());
  REQUIRE(labelArrayPtr != nullptr);
  auto& labelsRef = labelArrayPtr->getDataStoreRef();
  for(usize i = 0; i < faceLabels.size(); i++)
  {
    labelsRef[(i * 2) + 0] = faceLabels[i][0];
    labelsRef[(i * 2) + 1] = faceLabels[i][1];
  }

  return triangleGeomPtr;
}

/**
 * @brief Creates an empty EdgeGeom with its attribute matrices, plus a NumFeatures array.
 * ExtractTripleLinesAlgorithm resizes all of them.
 * @param dataStructure Receives the geometry and output arrays.
 * @return Geometry, NumFeatures array, and NodeTypes array.
 */
std::tuple<EdgeGeom*, Int8Array*, Int8Array*> CreateEmptyTripleLineGeom(DataStructure& dataStructure)
{
  auto* edgeGeomPtr = EdgeGeom::Create(dataStructure, k_AlgorithmTripleLineGeomName);
  REQUIRE(edgeGeomPtr != nullptr);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto* vertexArrayPtr = IGeometry::SharedVertexList::Create(dataStructure, "SharedVertexList", std::move(vertexStore), edgeGeomPtr->getId());
  REQUIRE(vertexArrayPtr != nullptr);
  edgeGeomPtr->setVertices(*vertexArrayPtr);

  auto edgeStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* edgeArrayPtr = IGeometry::SharedEdgeList::Create(dataStructure, "SharedEdgeList", std::move(edgeStore), edgeGeomPtr->getId());
  REQUIRE(edgeArrayPtr != nullptr);
  edgeGeomPtr->setEdgeList(*edgeArrayPtr);

  auto* vertexAmPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{0}, edgeGeomPtr->getId());
  REQUIRE(vertexAmPtr != nullptr);
  edgeGeomPtr->setVertexAttributeMatrix(*vertexAmPtr);

  auto* edgeAmPtr = AttributeMatrix::Create(dataStructure, "Edge Data", ShapeType{0}, edgeGeomPtr->getId());
  REQUIRE(edgeAmPtr != nullptr);
  edgeGeomPtr->setEdgeAttributeMatrix(*edgeAmPtr);

  auto numFeaturesStore = std::make_unique<DataStore<int8>>(std::vector<usize>{0}, std::vector<usize>{1}, 0);
  auto* numFeaturesArrayPtr = Int8Array::Create(dataStructure, k_AlgorithmNumFeaturesName, std::move(numFeaturesStore), edgeAmPtr->getId());
  REQUIRE(numFeaturesArrayPtr != nullptr);

  auto outNodeTypeStore = std::make_unique<DataStore<int8>>(std::vector<usize>{0}, std::vector<usize>{1}, 0);
  auto* outNodeTypeArrayPtr = Int8Array::Create(dataStructure, k_AlgorithmNodeTypesName, std::move(outNodeTypeStore), vertexAmPtr->getId());
  REQUIRE(outNodeTypeArrayPtr != nullptr);

  return {edgeGeomPtr, numFeaturesArrayPtr, outNodeTypeArrayPtr};
}

std::tuple<EdgeGeom*, Int8Array*, Int8Array*> GetOutputs(DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(DataPath({k_AlgorithmTripleLineGeomName})));
  auto* edgeGeomPtr = &dataStructure.getDataRefAs<EdgeGeom>(DataPath({k_AlgorithmTripleLineGeomName}));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_AlgorithmTripleLineGeomName, "Edge Data", k_AlgorithmNumFeaturesName})));
  auto* numFeaturesPtr = &dataStructure.getDataRefAs<Int8Array>(DataPath({k_AlgorithmTripleLineGeomName, "Edge Data", k_AlgorithmNumFeaturesName}));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_AlgorithmTripleLineGeomName, "Vertex Data", k_AlgorithmNodeTypesName})));
  auto* nodeTypesPtr = &dataStructure.getDataRefAs<Int8Array>(DataPath({k_AlgorithmTripleLineGeomName, "Vertex Data", k_AlgorithmNodeTypesName}));
  REQUIRE(edgeGeomPtr != nullptr);
  REQUIRE(numFeaturesPtr != nullptr);
  REQUIRE(nodeTypesPtr != nullptr);
  return {edgeGeomPtr, numFeaturesPtr, nodeTypesPtr};
}

enum class NodeTypePointers : uint8
{
  Both,
  Neither,
  InputOnly,
  OutputOnly
};

Result<> RunExtractTripleLinesAlgorithm(DataStructure& dataStructure, bool includeExterior = false, NodeTypePointers nodeTypePointers = NodeTypePointers::Both,
                                        const std::atomic_bool& shouldCancel = std::atomic_bool{false})
{
  auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = CreateEmptyTripleLineGeom(dataStructure);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_AlgorithmTriangleGeomName, k_AlgorithmFaceLabelsName})));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_AlgorithmTriangleGeomName, k_AlgorithmNodeTypesName})));
  const auto& sourceNodeTypes = dataStructure.getDataRefAs<Int8Array>(DataPath({k_AlgorithmTriangleGeomName, k_AlgorithmNodeTypesName})).getDataStoreRef();
  const auto* inputNodeTypesPtr = nodeTypePointers == NodeTypePointers::Both || nodeTypePointers == NodeTypePointers::InputOnly ? &sourceNodeTypes : nullptr;
  auto* outputNodeTypesPtr = nodeTypePointers == NodeTypePointers::Both || nodeTypePointers == NodeTypePointers::OutputOnly ? &nodeTypesPtr->getDataStoreRef() : nullptr;
  ExtractTripleLinesInputValues inputValues;
  inputValues.TriangleGeometryPath = DataPath({k_AlgorithmTriangleGeomName});
  inputValues.FaceLabelsPath = DataPath({k_AlgorithmTriangleGeomName, k_AlgorithmFaceLabelsName});
  inputValues.TripleLineGeometryPath = DataPath({k_AlgorithmTripleLineGeomName});
  inputValues.NumFeaturesPath = DataPath({k_AlgorithmTripleLineGeomName, "Edge Data", k_AlgorithmNumFeaturesName});
  inputValues.SourceNodeTypes = inputNodeTypesPtr;
  inputValues.DestinationNodeTypes = outputNodeTypesPtr;
  inputValues.IncludeExteriorLines = includeExterior;
  return ExtractTripleLines(dataStructure, {}, shouldCancel, &inputValues)();
}

const std::vector<std::array<float32, 3>> k_TripleJunctionVertices = {
    {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {-0.5f, 0.87f, 0.0f}, {-0.5f, 0.87f, 1.0f}, {-0.5f, -0.87f, 0.0f}, {-0.5f, -0.87f, 1.0f},
};
const std::vector<std::array<usize, 3>> k_TripleJunctionTriangles = {
    {0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6},
};
const std::vector<std::array<int32, 2>> k_TripleJunctionLabels = {
    {1, 2}, {1, 2}, {2, 3}, {2, 3}, {1, 3}, {1, 3},
};

} // namespace

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Flat boundary produces no triple lines", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;

  // A single quad between grains 1 and 2, split into two triangles. Every edge is
  // shared by at most two triangles, and both carry the same labels, so no edge
  // borders 3 or more unique Feature Ids.
  const std::vector<std::array<float32, 3>> vertices = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 2}, {0, 2, 3}};
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}};

  CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
  REQUIRE(numFeaturesArrayPtr->getNumberOfTuples() == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Interior triple junction", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;

  // Three quad sheets meeting along the shared edge v0->v1. Each sheet is split so that
  // exactly one of its two triangles contains both v0 and v1, so the shared edge is
  // touched by exactly 3 triangles carrying labels {1,2}, {2,3} and {1,3} => 3 unique.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const auto& faceLabels = k_TripleJunctionLabels;

  CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 2);
  REQUIRE(numFeaturesArrayPtr->getNumberOfTuples() == 1);
  REQUIRE((*numFeaturesArrayPtr)[0] == 3);

  // The one emitted edge must join the two ends of the shared edge, which sit at
  // z = 0 and z = 1 with x = y = 0. Vertices follow ascending source index order.
  const auto& edgesRef = edgeGeomPtr->getEdges()->getDataStoreRef();
  const auto& vertsRef = edgeGeomPtr->getVertices()->getDataStoreRef();
  std::set<float32> zCoords;
  for(usize i = 0; i < 2; i++)
  {
    const usize vertIndex = edgesRef[i];
    REQUIRE(vertsRef[(vertIndex * 3) + 0] == Approx(0.0f));
    REQUIRE(vertsRef[(vertIndex * 3) + 1] == Approx(0.0f));
    zCoords.insert(vertsRef[(vertIndex * 3) + 2]);
  }
  REQUIRE(zCoords == std::set<float32>{0.0f, 1.0f});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Quadruple point line", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;

  // Four sheets around the shared edge, labels {1,2}, {2,3}, {3,4} and {1,4} => 4 unique.
  const std::vector<std::array<float32, 3>> vertices = {
      {0.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 1.0f},  // v0, v1 : the shared edge
      {1.0f, 0.0f, 0.0f},  {1.0f, 0.0f, 1.0f},  // sheet A rim
      {0.0f, 1.0f, 0.0f},  {0.0f, 1.0f, 1.0f},  // sheet B rim
      {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 1.0f}, // sheet C rim
      {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 1.0f}, // sheet D rim
  };
  const std::vector<std::array<usize, 3>> triangles = {
      {0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6}, {0, 1, 9}, {0, 9, 8},
  };
  const std::vector<std::array<int32, 2>> faceLabels = {
      {1, 2}, {1, 2}, {2, 3}, {2, 3}, {3, 4}, {3, 4}, {1, 4}, {1, 4},
  };

  CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE(numFeaturesArrayPtr->getNumberOfTuples() == 1);
  REQUIRE((*numFeaturesArrayPtr)[0] == 4);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Vertex list is compacted", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;

  // Same fixture as Interior triple junction, which has 8 source vertices but only 2 lie on a
  // triple line. The output must carry exactly those 2, and every edge index must be
  // in range - i.e. the remap really happened rather than passing indices through.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const auto& faceLabels = k_TripleJunctionLabels;

  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(triangleGeomPtr->getNumberOfVertices() == 8);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 2);

  const auto& edgesRef = edgeGeomPtr->getEdges()->getDataStoreRef();
  for(usize i = 0; i < edgeGeomPtr->getNumberOfEdges() * 2; i++)
  {
    REQUIRE(edgesRef[i] < edgeGeomPtr->getNumberOfVertices());
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): IncludeExteriorLines toggles surface lines", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  // A grain boundary between grains 1 and 2 reaching the free surface of the volume.
  // Three sheets meet along the shared edge: the interior 1|2 boundary and two exposed outer faces.
  // The outer faces carry the -1 "outside" label.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}, {-1, 2}, {-1, 2}, {-1, 1}, {-1, 1}};

  SECTION("Interior only (the default) rejects it")
  {
    DataStructure dataStructure;
    auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

    Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, false);

    // Discounting -1, the shared edge borders only grains 1 and 2.
    REQUIRE(result.valid());
    const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("IncludeExteriorLines accepts it")
  {
    DataStructure dataStructure;
    auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

    Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, true);

    // Counting -1 as a region, the shared edge borders {1, 2, -1} => 3 unique.
    REQUIRE(result.valid());
    const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
    REQUIRE((*numFeaturesArrayPtr)[0] == 3);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): NodeTypes are copied through to the output vertices", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;

  // The Interior triple junction fixture: 8 source vertices, of which only v0 and v1 lie on the triple line.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const auto& faceLabels = k_TripleJunctionLabels;

  CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunExtractTripleLinesAlgorithm(dataStructure, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 2);
  REQUIRE(tripleLineNodeTypesPtr->getNumberOfTuples() == 2);

  // The Interior triple junction fixture retains source indices 0 and 1, in that order.
  REQUIRE((*tripleLineNodeTypesPtr)[0] == 2);
  REQUIRE((*tripleLineNodeTypesPtr)[1] == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Five sheets saturate at four Features", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;
  const std::vector<std::array<float32, 3>> vertices = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 1, 0}, {-1, -1, 0}, {1, -1, 0}};
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 1, 5}, {0, 1, 6}};
  const std::vector<std::array<int32, 2>> labels = {{1, 2}, {2, 3}, {3, 4}, {4, 5}, {1, 5}};
  CreateTriangleMesh(dataStructure, vertices, triangles, labels);
  auto result = RunExtractTripleLinesAlgorithm(dataStructure);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE((*numFeaturesPtr)[0] == 4); // A value of 4 means four or more unique Feature Ids.
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Feature zero is an ordinary Feature", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;
  const std::vector<std::array<int32, 2>> labels = {{0, 1}, {0, 1}, {1, 2}, {1, 2}, {0, 2}, {0, 2}};
  CreateTriangleMesh(dataStructure, k_TripleJunctionVertices, k_TripleJunctionTriangles, labels);
  auto result = RunExtractTripleLinesAlgorithm(dataStructure, false);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE((*numFeaturesPtr)[0] == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Checkerboard counts Features instead of triangles", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  // This fixture separates unique Feature Id count from triangle count per edge.
  // Four triangles share edge (0,1), but only two distinct Features border that edge.
  DataStructure dataStructure;
  const std::vector<std::array<float32, 3>> vertices = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}};
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 1, 5}};
  const std::vector<std::array<int32, 2>> labels(4, {1, 2});
  CreateTriangleMesh(dataStructure, vertices, triangles, labels);
  auto result = RunExtractTripleLinesAlgorithm(dataStructure);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Cancellation leaves empty output", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  DataStructure dataStructure;
  CreateTriangleMesh(dataStructure, k_TripleJunctionVertices, k_TripleJunctionTriangles, k_TripleJunctionLabels);
  const std::atomic_bool shouldCancel{true};
  auto result = RunExtractTripleLinesAlgorithm(dataStructure, false, NodeTypePointers::Both, shouldCancel);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): Output follows sorted source indices", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  // Unused source indices force compaction. Triangles for the larger edge key come first,
  // and each shared edge has its endpoints reversed to check canonical key ordering.
  const std::vector<std::array<float32, 3>> vertices = {{9, 9, 9}, {2, 0, 0}, {0, 0, 0}, {9, 9, 8}, {2, 0, 1}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {3, 0, 0}, {2, 1, 0}, {1, 1, 0}};
  const std::vector<std::array<usize, 3>> triangles = {{5, 2, 6}, {5, 2, 7}, {5, 2, 8}, {4, 1, 9}, {4, 1, 10}, {4, 1, 11}};
  const std::vector<std::array<int32, 2>> labels = {{1, 2}, {2, 3}, {1, 3}, {1, 2}, {2, 3}, {1, 3}};
  for(const bool reverseTriangles : {false, true})
  {
    CAPTURE(reverseTriangles);
    auto orderedTriangles = triangles;
    auto orderedLabels = labels;
    if(reverseTriangles)
    {
      std::reverse(orderedTriangles.begin(), orderedTriangles.end());
      std::reverse(orderedLabels.begin(), orderedLabels.end());
    }
    DataStructure dataStructure;
    CreateTriangleMesh(dataStructure, vertices, orderedTriangles, orderedLabels);
    auto result = RunExtractTripleLinesAlgorithm(dataStructure);
    REQUIRE(result.valid());
    const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 2);
    REQUIRE(edgeGeomPtr->getNumberOfVertices() == 4);
    const auto& outputVertices = edgeGeomPtr->getVertices()->getDataStoreRef();
    const std::array<usize, 4> sourceIndices = {1, 2, 4, 5};
    for(usize vertexIdx = 0; vertexIdx < sourceIndices.size(); vertexIdx++)
    {
      for(usize compIdx = 0; compIdx < 3; compIdx++)
      {
        REQUIRE(outputVertices[(3 * vertexIdx) + compIdx] == vertices[sourceIndices[vertexIdx]][compIdx]);
      }
    }
    const auto& edges = edgeGeomPtr->getEdges()->getDataStoreRef();
    REQUIRE(edges[0] == 0);
    REQUIRE(edges[1] == 2);
    REQUIRE(edges[2] == 1);
    REQUIRE(edges[3] == 3);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ExtractTripleLines (Algorithm): NodeTypes pointers must be paired", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  const auto pointers = GENERATE(NodeTypePointers::Neither, NodeTypePointers::InputOnly, NodeTypePointers::OutputOnly);
  DataStructure dataStructure;
  CreateTriangleMesh(dataStructure, k_TripleJunctionVertices, k_TripleJunctionTriangles, k_TripleJunctionLabels);
  auto result = RunExtractTripleLinesAlgorithm(dataStructure, false, pointers);
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  if(pointers == NodeTypePointers::Neither)
  {
    REQUIRE(result.valid());
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
    REQUIRE((*numFeaturesPtr)[0] == 3);
    // The AM resizes this optional test array, but extraction must not copy into it.
    REQUIRE((*nodeTypesPtr)[0] == 0);
    REQUIRE((*nodeTypesPtr)[1] == 0);
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -57401);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
    REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
