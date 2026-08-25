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

#include <any>
#include <array>
#include <memory>
#include <string>
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
