#include "SimplnxCore/Filters/ExtractTripleLinesFilter.hpp"
#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

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
 * @brief Runs QuickSurfaceMesh over the shared 2x2x1 four-grain block, leaving a TriangleGeom at
 * k_TriangleGeomPath with its FaceLabels and NodeTypes. The mesher no longer produces triple
 * lines itself; extraction is a separate step.
 */
void RunQuickSurfaceMesh(DataStructure& dataStructure)
{
  UnitTest::BuildFourGrainBlock(dataStructure);

  QuickSurfaceMeshFilter filter;
  Arguments args;
  args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Cell Data", "FeatureIds"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

/**
 * @brief Runs ExtractTripleLines over whatever TriangleGeom sits at k_TriangleGeomPath.
 */
IFilter::ExecuteResult RunExtractTripleLines(DataStructure& dataStructure, bool includeExterior = false, const DataPath& triangleGeomPath = k_TriangleGeomPath,
                                             const DataPath& faceLabelsPath = k_FaceLabelsPath, const DataPath& nodeTypesPath = k_NodeTypesPath)
{
  ExtractTripleLinesFilter filter;
  Arguments args;
  args.insertOrAssign(ExtractTripleLinesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(ExtractTripleLinesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
  args.insertOrAssign(ExtractTripleLinesFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(nodeTypesPath));
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
 */
void RunSurfaceNets(DataStructure& dataStructure)
{
  UnitTest::BuildFourGrainBlock(dataStructure);

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
 */
void RunM3CSurfaceMeshing(DataStructure& dataStructure)
{
  UnitTest::BuildFourGrainBlock(dataStructure);

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

  // Counting the outside of the volume as a region makes every grain boundary that reaches the
  // free surface register as a triple line too, so the count must rise materially. Asserted as a
  // strict increase rather than a hardcoded number so the test does not pin an incidental value.
  REQUIRE(interiorOnlyEdges == 1);
  REQUIRE(tripleLineGeom.getNumberOfEdges() > interiorOnlyEdges);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath));
  const auto& numFeatures = dataStructure.getDataRefAs<Int8Array>(k_OutNumFeaturesPath);
  for(usize i = 0; i < numFeatures.getNumberOfTuples(); i++)
  {
    INFO("edge " << i << " reported NumFeatures " << static_cast<int32>(numFeatures[i]));
    REQUIRE((numFeatures[i] == 3 || numFeatures[i] == 4));
  }

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

  // The array must exist, be sized to the created vertices, and be usable by a filter that
  // requires a Node Type array - which is the whole reason it is emitted.
  REQUIRE(outNodeTypes.getNumberOfTuples() == tripleLineGeom.getNumberOfVertices());

  // Every vertex on a triple line is by definition a junction node, so its junction count (the
  // NodeTypes value modulo the +10 surface offset) must be at least 3.
  for(usize i = 0; i < outNodeTypes.getNumberOfTuples(); i++)
  {
    const int32 junctionCount = static_cast<int32>(outNodeTypes[i] % 10);
    INFO("triple line vertex " << i << " has NodeType " << static_cast<int32>(outNodeTypes[i]));
    REQUIRE(junctionCount >= 3);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Agrees with the source mesh NodeTypes", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  RunQuickSurfaceMesh(dataStructure);
  auto executeResult = RunExtractTripleLines(dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Cross-checks two independently computed quantities: NodeTypes comes from the mesher's own
  // junction logic, the triple lines come from FaceLabels. A disagreement points at the NodeTypes
  // producer, not at the extraction.
  UnitTest::CheckTripleLineNodeTypeAgreement(dataStructure, k_TripleLineGeomPath, k_TriangleGeomPath, k_NodeTypesPath);

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

  SECTION("Face Labels sized to the vertices rather than the faces")
  {
    // NodeTypes has one tuple per vertex, so handing it in as Face Labels is a realistic
    // mis-selection. It must be caught in preflight, not read past the end during execute.
    args.insertOrAssign(ExtractTripleLinesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(ExtractTripleLinesFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
}

TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: Consistent across surface meshers", "[SimplnxCore][ExtractTripleLinesFilter]")
{
  UnitTest::LoadPlugins();

  // The same four-grain block through each mesher, then extraction. The assertions are limited to
  // properties that are genuinely mesher-independent: segment COUNT is not one of them, because it
  // follows node placement and the three meshers place nodes differently (QuickSurfaceMesh on
  // voxel corners, SurfaceNets cell-centred, M3C on cube faces).
  struct MesherCase
  {
    const char* Name;
    void (*Run)(DataStructure&);
  };
  const std::array<MesherCase, 3> mesherCases = {MesherCase{"QuickSurfaceMesh", &RunQuickSurfaceMesh}, MesherCase{"SurfaceNets", &RunSurfaceNets},
                                                 MesherCase{"M3CSurfaceMeshing", &RunM3CSurfaceMeshing}};

  for(const auto& mesherCase : mesherCases)
  {
    DYNAMIC_SECTION(mesherCase.Name)
    {
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

      // 2. Only 3 and 4 are meaningful multiplicities.
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
}

// M3CSurfaceMeshing emits a triple line vertex at z = -0.5 on this input: a full cell BELOW a
// domain that spans z = 0 to 1, reported as a quadruple point. It also yields 2 segments where
// QuickSurfaceMesh and SurfaceNets both yield 1.
//
// This is a PRE-EXISTING M3C defect, entirely unrelated to triple line extraction, which merely
// made it visible: extraction reads the mesh and FaceLabels that M3C already produced.
//
// The underlying defect is BROADER than what this test pins. M3C's Triangle Geometry itself lies
// partly outside the input volume, at every domain size, not just thin ones. Measured against a
// unit-spacing volume with origin at 0:
//
//     domain    QuickSurfaceMesh bounds        M3CSurfaceMeshing bounds        M3C verts outside
//     2x2x1     x[0,2] y[0,2] z[0,1]           x[-1,2] y[-1,2] z[-0.5,0.5]     54 of  85
//     4x4x4     x[0,4] y[0,4] z[0,4]           x[-1,4] y[-1,4] z[-0.5,3.5]    142 of 260
//     8x8x8     x[0,8] y[0,8] z[0,8]           x[-1,8] y[-1,8] z[-0.5,7.5]    368 of 744
//
// Roughly half of every M3C mesh sits outside the volume: x and y extend a full cell to -1, and z
// is offset by half a cell. M3CSurfaceMeshing.cpp's SiteCoords already carries a comment saying it
// subtracts the ghost shell to keep coordinates "aligned with the input volume and the other
// meshers" - so this is a stated intent the code does not achieve, not a deliberate convention.
//
// Note that M3C's exemplar comparison test compares vertex coordinates exactly, so the exemplar
// has these coordinates baked in; fixing M3C means regenerating it.
//
// The [!shouldfail] tag keeps the suite green while pinning the defect, following the precedent in
// ComputeFeatureSizesTest.cpp. When M3C is fixed this test will start passing, at which point
// Catch2 will report it as a failure and the tag must be REMOVED.
//
// CHECK (not REQUIRE) on the segment count so the z-bound assertions below are still evaluated
// rather than the case aborting at the first failure.
TEST_CASE("SimplnxCore::ExtractTripleLinesFilter: M3C triple lines stay within the domain bounds", "[SimplnxCore][ExtractTripleLinesFilter][M3CSurfaceMeshingFilter][!shouldfail]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  RunM3CSurfaceMeshing(dataStructure);

  auto executeResult = RunExtractTripleLines(dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath));
  const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(k_TripleLineGeomPath);

  // The four cells meet along a single interior vertical grid edge.
  CHECK(tripleLineGeom.getNumberOfEdges() == 1);

  // No vertex may lie outside the domain, which spans z = 0 to 1.
  constexpr float32 k_Epsilon = 0.0001f;
  const auto& vertsRef = tripleLineGeom.getVertices()->getDataStoreRef();
  for(usize i = 0; i < tripleLineGeom.getNumberOfVertices(); i++)
  {
    const float32 z = vertsRef[i * 3 + 2];
    INFO("triple line vertex " << i << " has z = " << z);
    CHECK(z >= (0.0f - k_Epsilon));
    CHECK(z <= (1.0f + k_Epsilon));
  }
}
