#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <set>
#include <utility>

using namespace nx::core;

namespace
{
const DataPath k_ImageGeomPath({"ImageGeom"});
const DataPath k_FeatureIdsPath({"ImageGeom", "CellData", "FeatureIds"});
const DataPath k_TriangleGeomPath({"SurfaceNets"});

// Runs SurfaceNets on the flush cylinder and returns the distinct {comp0, comp1} label pairs.
std::set<std::pair<int32, int32>> RunAndCollectLabelPairs()
{
  DataStructure dataStructure = SurfaceMeshingTest::CreateCylinderInBox(true);

  SurfaceNetsFilter filter;
  Arguments args;
  args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
  args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
  args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
  args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
  args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
  args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath faceLabelsPath = k_TriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(faceLabelsPath));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(faceLabelsPath).getDataStoreRef();

  std::set<std::pair<int32, int32>> labelPairs;
  for(usize i = 0; i < faceLabelsRef.getNumberOfTuples(); i++)
  {
    labelPairs.insert({faceLabelsRef[i * 2], faceLabelsRef[i * 2 + 1]});
  }
  return labelPairs;
}
} // namespace

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Face Label conventions", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  const std::set<std::pair<int32, int32>> labelPairs = RunAndCollectLabelPairs();

  // The cylinder wall separates background (0) from the cylinder (1).
  REQUIRE(labelPairs.count({0, 1}) == 1);
  // The box wall backed by background must be distinguishable from a feature cap.
  REQUIRE(labelPairs.count({-1, 0}) == 1);
  // The cylinder's bottom cap: box wall backed by the cylinder.
  REQUIRE(labelPairs.count({-1, 1}) == 1);
  // The buggy {-1,-1} pair must not appear.
  REQUIRE(labelPairs.count({-1, -1}) == 0);
}
