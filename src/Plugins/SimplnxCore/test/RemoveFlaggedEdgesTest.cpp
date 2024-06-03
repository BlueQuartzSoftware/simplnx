#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/MultiThresholdObjectsFilter.hpp"
#include "SimplnxCore/Filters/RemoveFlaggedEdgesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
namespace
{
// fs::path k_ExemplarDataFilePath = fs::path(fmt::format("{}/6_6_scan_path_test_data/6_6_scan_path_test_data.dream3d", nx::core::unit_test::k_TestFilesDir));
// fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_scan_path_test_data/masked_triangle_geometry.dream3d", nx::core::unit_test::k_TestFilesDir));

// static constexpr StringLiteral k_CreatedAMName = "Cell Feature AM";
// static constexpr StringLiteral k_NumTrianglesName = "NumTriangles";
// static constexpr StringLiteral k_RegionIdsName = "Region Ids";

const DataPath k_TriangleGeomPath({"Exemplar Edge Geometry"});
const DataPath k_MaskPath = k_TriangleGeomPath.createChildPath("Edge Data").createChildPath(Constants::k_Mask);
const DataPath k_ReducedGeomPath({"Reduced Geometry"});

const DataPath k_VertexListPath = k_ReducedGeomPath.createChildPath("SharedVertexList");
const DataPath k_TriangleListPath = k_ReducedGeomPath.createChildPath("SharedTriList");
} // namespace
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedEdgesFilter: Valid Filter Execution", "[SimplnxCore][RemoveFlaggedEdgesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "6_6_scan_path_test_data.tar.gz", "6_6_scan_path_test_data.dream3d");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_scan_path_test_data/6_6_scan_path_test_data.dream3d", nx::core::unit_test::k_TestFilesDir));
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
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
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
