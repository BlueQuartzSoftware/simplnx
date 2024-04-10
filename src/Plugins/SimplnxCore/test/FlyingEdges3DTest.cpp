#include "SimplnxCore/Filters/FlyingEdges3DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace ContourTest
{
const float64 k_IsoVal = 328;

const std::string k_ImageGeometryName = "Geometry";
const std::string k_DataName = "Data";
const std::string k_VertexNormals = "VertexNormals";

const DataPath k_GeometryPath = DataPath({k_ImageGeometryName});
const DataPath k_DataPath = k_GeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_DataName);

const std::string k_NewTriangleContourName = "Contouring Test";
const std::string k_ExemplarTriangleContourName = "Contouring Geometry";

const DataPath k_ExemplarContourPath = DataPath({k_ExemplarTriangleContourName});
const DataPath k_NewContourPath = DataPath({k_NewTriangleContourName});

const DataPath k_ExemplarNormals = k_ExemplarContourPath.createChildPath(INodeGeometry0D::k_VertexDataName).createChildPath(k_VertexNormals);
const DataPath k_NewNormals = k_NewContourPath.createChildPath(INodeGeometry0D::k_VertexDataName).createChildPath(k_VertexNormals);
} // namespace ContourTest

TEST_CASE("SimplnxCore::Image Contouring Valid Execution", "[SimplnxCore][FlyingEdges3D]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "flying_edges_exemplar.tar.gz",
                                                              "flying_edges_exemplar.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/flying_edges_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    FlyingEdges3DFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FlyingEdges3DFilter::k_IsoVal_Key, std::make_any<float64>(ContourTest::k_IsoVal));
    // Selected Data Objects
    args.insertOrAssign(FlyingEdges3DFilter::k_SelectedImageGeometryPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(ContourTest::k_GeometryPath));
    args.insertOrAssign(FlyingEdges3DFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(ContourTest::k_DataPath));
    // Output Path
    args.insertOrAssign(FlyingEdges3DFilter::k_NewTriangleGeometryName_Key, std::make_any<DataObjectNameParameter::ValueType>(ContourTest::k_NewTriangleContourName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // This is in here because the exemplar face attribute matrix is not sized correctly. This will
    // correct that value allowing the test to proceed normally.
    auto& exemplarContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(ContourTest::k_ExemplarContourPath);
    exemplarContourTriGeom.getFaceAttributeMatrixRef().resizeTuples({70});

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the results
  {
    auto& newContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(ContourTest::k_NewContourPath);
    auto& exemplarContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(ContourTest::k_ExemplarContourPath);

    const auto& kNxVertArray = newContourTriGeom.getVerticesRef();
    const auto& kExemplarsVertArray = exemplarContourTriGeom.getVerticesRef();

    CompareDataArrays<IGeometry::SharedVertexList::value_type>(kExemplarsVertArray, kNxVertArray);

    const auto& kNxTriArray = newContourTriGeom.getFacesRef();
    const auto& kExemplarsTriArray = exemplarContourTriGeom.getFacesRef();

    CompareDataArrays<IGeometry::MeshIndexType>(kExemplarsTriArray, kNxTriArray);

    const auto& kNxNormalsArray = dataStructure.getDataRefAs<Float32Array>(ContourTest::k_NewNormals);
    const auto& kExemplarsNormalsArray = dataStructure.getDataRefAs<Float32Array>(ContourTest::k_ExemplarNormals);

    CompareDataArrays<float32>(kExemplarsNormalsArray, kNxNormalsArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/image_contouring_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
