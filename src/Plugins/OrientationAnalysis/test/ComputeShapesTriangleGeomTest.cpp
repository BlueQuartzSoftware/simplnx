#include "OrientationAnalysis/Filters/ComputeShapesTriangleGeomFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>
#include <catch2/catch.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

#define SIMPLNX_WRITE_TEST_OUTPUT

namespace ComputeShapesTriangleGeomFilterTest
{
const std::string k_FaceLabelsName = "Face Labels";
const std::string k_FaceFeatureName = "Face Feature Data";
const std::string k_FaceDataName = "Face Data";
const std::string k_CentroidsArrayName = "Centroids";

const std::string k_Omega3SArrayName = "Omega3s [NX Computed]";
const std::string k_AxisLengthsArrayName = "AxisLengths [NX Computed]";
const std::string k_AxisEulerAnglesArrayName = "AxisEulerAngles [NX Computed]";
const std::string k_AspectRatiosArrayName = "AspectRatios [NX Computed]";

const std::string k_ExemplarOmega3SArrayName = "Exemplar Omega3s";
const std::string k_ExemplarAxisLengthsArrayName = "Exemplar AxisLengths";
const std::string k_ExemplarAxisEulerAnglesArrayName = "Exemplar AxisEulerAngles";
const std::string k_ExemplarAspectRatiosArrayName = "Exemplar AspectRatios";

constexpr StringLiteral k_GeomName = "InputGeometry";

const DataPath k_GeometryPath({k_GeomName});

const DataPath k_FaceFeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceDataPath = k_GeometryPath.createChildPath(k_FaceDataName);
const DataPath k_FaceLabelsPath = k_FaceDataPath.createChildPath(k_FaceLabelsName);
const DataPath k_FaceFeatureCentroidsPath = k_FaceFeatureAttributeMatrixPath.createChildPath(k_CentroidsArrayName);
} // namespace ComputeShapesTriangleGeomFilterTest

using namespace ComputeShapesTriangleGeomFilterTest;

// !!! See filter documentation for information on included data and how it was generated and visually validated !!!
TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom", "[OrientationAnalysis][ComputeShapesTriangleGeom]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_compute_triangle_shapes_test.tar.gz", "7_compute_triangle_shapes_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/7_compute_triangle_shapes_test/test/7_exemplar_triangle_shapes.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter and an Arguments Object
  ComputeShapesTriangleGeomFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));

  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FaceFeatureAttributeMatrixPath));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_FaceFeatureCentroidsPath));

  // Output Vars
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_Omega3sArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Omega3SArrayName));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_AxisLengthsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_AspectRatiosArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(exemplarDataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(exemplarDataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(exemplarDataStructure, fs::path(fmt::format("{}/{}.dream3d", unit_test::k_BinaryTestOutputDir, "ComputeShapesTriangleGeomTestOutput")));
#endif

  UnitTest::CompareArrays<float32>(exemplarDataStructure, k_FaceFeatureAttributeMatrixPath.createChildPath(k_Omega3SArrayName),
                                   k_FaceFeatureAttributeMatrixPath.createChildPath(k_ExemplarOmega3SArrayName));
  UnitTest::CompareArrays<float32>(exemplarDataStructure, k_FaceFeatureAttributeMatrixPath.createChildPath(k_AxisLengthsArrayName),
                                   k_FaceFeatureAttributeMatrixPath.createChildPath(k_ExemplarAxisLengthsArrayName));
  UnitTest::CompareArrays<float32>(exemplarDataStructure, k_FaceFeatureAttributeMatrixPath.createChildPath(k_AxisEulerAnglesArrayName),
                                   k_FaceFeatureAttributeMatrixPath.createChildPath(k_ExemplarAxisEulerAnglesArrayName));
  UnitTest::CompareArrays<float32>(exemplarDataStructure, k_FaceFeatureAttributeMatrixPath.createChildPath(k_AspectRatiosArrayName),
                                   k_FaceFeatureAttributeMatrixPath.createChildPath(k_ExemplarAspectRatiosArrayName));

  UnitTest::CheckArraysInheritTupleDims(exemplarDataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Face Feature Centroids requires 3 components", "[OrientationAnalysis][ComputeShapesTriangleGeom][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* triangleGeom = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(triangleGeom != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {3}, {3}, triangleGeom->getId());
  triangleGeom->setVertices(*vertices);
  triangleGeom->setVertexCoordinate(0, {0.0F, 0.0F, 0.0F});
  triangleGeom->setVertexCoordinate(1, {1.0F, 0.0F, 0.0F});
  triangleGeom->setVertexCoordinate(2, {0.0F, 1.0F, 0.0F});
  auto* faces = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Faces", {1}, {3}, triangleGeom->getId());
  (*faces)[0] = 0;
  (*faces)[1] = 1;
  (*faces)[2] = 2;
  triangleGeom->setFaceList(*faces);
  auto* faceData = AttributeMatrix::Create(dataStructure, "Face Data", {1}, triangleGeom->getId());
  REQUIRE(faceData != nullptr);
  triangleGeom->setFaceAttributeMatrix(*faceData);
  auto* labels = UnitTest::CreateTestDataArray<int32>(dataStructure, "Face Labels", {1}, {2}, faceData->getId());
  (*labels)[0] = 1;
  (*labels)[1] = 0;
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", {2}, triangleGeom->getId());
  REQUIRE(featureData != nullptr);
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Centroids", {2}, {1}, featureData->getId());

  ComputeShapesTriangleGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Face Data", "Face Labels"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Feature Data"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Feature Data", "Centroids"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(std::any_of(preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end(), [](const Error& error) { return error.code == -208; }));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
