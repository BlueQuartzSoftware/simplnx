#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ImageContouringFilter.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;

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

TEST_CASE("ComplexCore::Image Contouring Valid Execution", "[ComplexCore][ImageContouring]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "flying_edges_exemplar.tar.gz", "flying_edges_exemplar.dream3d",
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/flying_edges_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    ImageContouringFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ImageContouringFilter::k_IsoVal_Key, std::make_any<float64>(ContourTest::k_IsoVal));
    // Selected Data Objects
    args.insertOrAssign(ImageContouringFilter::k_SelectedImageGeometry_Key, std::make_any<GeometrySelectionParameter::ValueType>(ContourTest::k_GeometryPath));
    args.insertOrAssign(ImageContouringFilter::k_SelectedDataArray_Key, std::make_any<DataPath>(ContourTest::k_DataPath));
    // Output Path
    args.insertOrAssign(ImageContouringFilter::k_NewTriangleGeometryName_Key, std::make_any<DataObjectNameParameter::ValueType>(ContourTest::k_NewTriangleContourName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
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

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/image_contouring_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
