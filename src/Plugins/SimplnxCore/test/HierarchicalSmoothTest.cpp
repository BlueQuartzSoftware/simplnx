#include "SimplnxCore/Filters/HierarchicalSmoothFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_TriangleGeometryName = "Geometry";
const std::string k_FaceDataName = INodeGeometry2D::k_FaceAttributeMatrixName;
const std::string k_VertexDataName = INodeGeometry0D::k_VertexAttributeMatrixName;
const std::string k_NodeTypeName = "NodeType";
const std::string k_FaceLabelsName = "FaceLabels";

const DataPath triangleGeomPath = DataPath({k_TriangleGeometryName});
const DataPath nodeTypePath = triangleGeomPath.createChildPath(INodeGeometry0D::k_VertexAttributeMatrixName).createChildPath(k_NodeTypeName);
const DataPath faceLabelsPath = triangleGeomPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName).createChildPath(k_FaceLabelsName);
const DataPath sharedVertexList = triangleGeomPath.createChildPath("SharedVertexList");

} // namespace

TEST_CASE("SimplnxCore::HierarchicalSmoothFilter: Valid filter execution", "[SurfaceMeshing][HierarchicalSmoothFilter]")
{
  // Load the Simplnx Application instance and load the plugins
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "hierarchical_smoothing.tar.gz", "hierarchical_smoothing");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/hierarchical_smoothing/ex1/ex1.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    HierarchicalSmoothFilter filter;
    Arguments args;
    args.insertOrAssign(HierarchicalSmoothFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(triangleGeomPath));
    args.insertOrAssign(HierarchicalSmoothFilter::k_SurfaceMeshNodeTypeArrayPath_Key, std::make_any<DataPath>(nodeTypePath));
    args.insertOrAssign(HierarchicalSmoothFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
    args.insertOrAssign(HierarchicalSmoothFilter::k_MaxIterations_Key, std::make_any<int32>(53));
    args.insertOrAssign(HierarchicalSmoothFilter::k_ErrorThreshold_Key, std::make_any<float64>(2.0));

    // Preflight
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute
    auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  DataPath exemplarSharedVertexListPath({"Smoothed Geometry", "SharedVertexList"});
  UnitTest::CompareArrays<float32>(dataStructure, sharedVertexList, exemplarSharedVertexListPath);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/hierarchical_smooth_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
