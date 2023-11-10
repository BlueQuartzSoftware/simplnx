#include "OrientationAnalysis/Filters/FindTriangleGeomShapesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;

namespace FindTriangleGeomShapesFilterTest
{
const std::string k_TriangleGeometryName = "TriangleDataContainer";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_FaceFeatureName = "FaceFeatureData";
const std::string k_FaceDataName = "FaceData";
const std::string k_CentroidsArrayName = "Centroids";
const std::string k_VolumesArrayName = "Volumes";

const std::string k_Omega3SArrayName = "Omega3s [NX Computed]";
const std::string k_AxisLengthsArrayName = "AxisLengths [NX Computed]";
const std::string k_AxisEulerAnglesArrayName = "AxisEulerAngles [NX Computed]";
const std::string k_AspectRatiosArrayName = "AspectRatios [NX Computed]";

const DataPath k_GeometryPath = DataPath({k_TriangleGeometryName});
const DataPath k_FaceFeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceDataName).createChildPath(k_FaceLabelsName);
const DataPath k_FaceFeatureCentroidsPath = k_FaceFeatureAttributeMatrixPath.createChildPath(k_CentroidsArrayName);
const DataPath k_FaceFeatureVolumesPath = k_FaceFeatureAttributeMatrixPath.createChildPath(k_VolumesArrayName);
} // namespace FindTriangleGeomShapesFilterTest

using namespace FindTriangleGeomShapesFilterTest;

TEST_CASE("OrientationAnalysis::FindTriangleGeomShapes", "[OrientationAnalysis][FindTriangleGeomShapes]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "12_IN625_GBCD.tar.gz", "12_IN625_GBCD");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    FindTriangleGeomShapesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));

    args.insertOrAssign(FindTriangleGeomShapesFilter::k_FeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_FaceFeatureAttributeMatrixPath));
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_FaceFeatureCentroidsPath));
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_VolumesArrayPath_Key, std::make_any<DataPath>(k_FaceFeatureVolumesPath));
    // Output Vars
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_Omega3sArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Omega3SArrayName));
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName));
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName));
    args.insertOrAssign(FindTriangleGeomShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::vector<std::string> outputArrayNames = {k_Omega3SArrayName, k_AxisLengthsArrayName, k_AxisEulerAnglesArrayName, k_AspectRatiosArrayName};
  std::vector<std::string> exemplarArrayNames = {"Omega3s", "AxisLengths", "AxisEulerAngles", "AspectRatios"};
  for(usize i = 0; i < 4; i++)
  {
    const DataPath kExemplarArrayPath = k_FaceFeatureAttributeMatrixPath.createChildPath(exemplarArrayNames[i]);
    const DataPath kNxArrayPath = k_FaceFeatureAttributeMatrixPath.createChildPath(outputArrayNames[i]);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<float32>(kExemplarsArray, kNxArray);
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_triangle_geom_shapes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
