#include "OrientationAnalysis/Filters/ComputeShapesTriangleGeomFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

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
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_compute_triangle_shapes_test.tar.gz",
                                                              "7_compute_triangle_shapes_test");

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
}
