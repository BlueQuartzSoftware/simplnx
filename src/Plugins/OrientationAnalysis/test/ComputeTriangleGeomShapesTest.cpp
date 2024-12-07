#include "OrientationAnalysis/Filters/ComputeTriangleGeomShapesFilter.hpp"
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
using namespace nx::core::UnitTest;

#define SIMPLNX_WRITE_TEST_OUTPUT

namespace ComputeTriangleGeomShapesFilterTest
{
const std::string k_FaceLabelsName = "Face Labels";
const std::string k_FaceFeatureName = "Face Feature Data";
const std::string k_FaceDataName = "Face Data";
const std::string k_CentroidsArrayName = "Centroids";
const std::string k_VolumesArrayName = "Volumes";

const std::string k_Omega3SArrayName = "Omega3s [NX Computed]";
const std::string k_AxisLengthsArrayName = "AxisLengths [NX Computed]";
const std::string k_AxisEulerAnglesArrayName = "AxisEulerAngles [NX Computed]";
const std::string k_AspectRatiosArrayName = "AspectRatios [NX Computed]";

// const DataPath k_FaceFeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
// const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceDataName).createChildPath(k_FaceLabelsName);
// const DataPath k_FaceFeatureCentroidsPath = k_FaceFeatureAttributeMatrixPath.createChildPath(k_CentroidsArrayName);
// const DataPath k_FaceFeatureVolumesPath = k_FaceFeatureAttributeMatrixPath.createChildPath(k_VolumesArrayName);

constexpr float32 k_Sphere_Omega_3 = 2193.245f; // 1.00000

constexpr float32 k_Tetrahedron_Omega_3 = 888.889f / k_Sphere_Omega_3;
constexpr float32 k_Cube_Omega_3 = 1728.0f / k_Sphere_Omega_3;                // 0.787873675763538
constexpr float32 k_Octahedron_Omega_3 = 1777.778f / k_Sphere_Omega_3;        // 0.810569726592332
constexpr float32 k_Dodecahedron_Omega_3 = 2096.873f / k_Sphere_Omega_3;      // 0.956059628541271
constexpr float32 k_Icosahedron_Omega_3 = 2122.033f / k_Sphere_Omega_3;       // 0.967531215162921
constexpr float32 k_TriangularPyramid_Omega_3 = 888.889f / k_Sphere_Omega_3;  // 0.405284863296166
constexpr float32 k_Rectangular_Prism_Omega_3 = 1728.0f / k_Sphere_Omega_3;   // 0.787873675763538
constexpr float32 k_Circular_Cylinder_Omega_3 = 1894.964f / k_Sphere_Omega_3; // 0.864000145902533
constexpr float32 k_Ellipsoid_Omega_3 = 2193.245f / k_Sphere_Omega_3;         // 1.000

const std::string k_TestFileDirName = "7_Triangle_Shapes_Files/stl_exemplar";
std::vector<std::string> k_TestFileNames = {"10_octahedron", "11_pyramid_elongated", "20_sphere",   "21_ellipsoid",    "40_rounded_cube", "41_rounded_cube_elongated",
                                            "50_cube",       "51_rectangular_prism", "60_Cylinder", "70_Dodecahedron", "80_Icosahedron",  "90_tetrahedron"};

std::vector<float> k_Theoretical_Omega_3_Values = {
    k_Octahedron_Omega_3,  k_Octahedron_Omega_3, 1.0f, k_Ellipsoid_Omega_3, 0.907957, 0.907945, k_Cube_Omega_3, k_Rectangular_Prism_Omega_3, k_Circular_Cylinder_Omega_3, k_Dodecahedron_Omega_3,
    k_Icosahedron_Omega_3, k_Tetrahedron_Omega_3};

} // namespace ComputeTriangleGeomShapesFilterTest

using namespace ComputeTriangleGeomShapesFilterTest;

TEST_CASE("OrientationAnalysis::ComputeTriangleGeomShapes", "[OrientationAnalysis][ComputeTriangleGeomShapes]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_Triangle_Shapes_Files.tar.gz", "7_Triangle_Shapes_Files");
  usize index = 0;
  for(const auto& fileName : k_TestFileNames)
  {
    // Read Exemplar DREAM3D File Filter
    auto exemplarFilePath = fs::path(fmt::format("{}/{}/{}.dream3d", nx::core::unit_test::k_TestFilesDir, k_TestFileDirName, fileName));
    // std::cout << "Loading File: " << exemplarFilePath.string() << "\n";
    DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

    DataPath geometryPath({fileName});
    DataPath faceDataPath = geometryPath.createChildPath(k_FaceDataName);
    DataPath faceFeatureDataPath = geometryPath.createChildPath(k_FaceFeatureName);

    {
      // Instantiate the filter and an Arguments Object
      ComputeTriangleGeomShapesFilter filter;
      Arguments args;

      // Create default Parameters for the filter.
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(geometryPath));
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(k_FaceLabelsName)));

      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(faceFeatureDataPath));
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(faceFeatureDataPath.createChildPath(k_CentroidsArrayName)));
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_VolumesArrayPath_Key, std::make_any<DataPath>(faceFeatureDataPath.createChildPath(k_VolumesArrayName)));
      // Output Vars
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_Omega3sArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Omega3SArrayName));
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName));
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName));
      args.insertOrAssign(ComputeTriangleGeomShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/{}.dream3d", unit_test::k_BinaryTestOutputDir, fileName)));
#endif

    const auto& computedOmega3 = dataStructure.getDataRefAs<Float32Array>(faceFeatureDataPath.createChildPath(k_Omega3SArrayName));
    float32 diff = std::abs(computedOmega3[1] - k_Theoretical_Omega_3_Values[index]);
    // std::cout << fileName << ": Omega-3 Computed: " << computedOmega3[1] << "  Theoretical Value: " << k_Theoretical_Omega_3_Values[index] << "   Diff: " << diff << "\n";
    REQUIRE(diff < 1.0E-4);
    index++;
  }
}
