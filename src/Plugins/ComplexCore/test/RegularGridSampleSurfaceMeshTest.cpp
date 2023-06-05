#include <catch2/catch.hpp>

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp"

using namespace complex;

namespace
{
const std::string k_TriGeomName = "STL-Cylinder";
const DataPath k_TriGeomPath = DataPath({k_TriGeomName});
const DataPath k_FaceLabelsPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath(Constants::k_FaceLabels);

const std::string k_ExemplarImageGeomName = "Sampled_Cylinder";
const DataPath k_ExemplarImageGeomPath = DataPath({k_ExemplarImageGeomName});
const DataPath k_ExemplarFeatureIdsPath = k_ExemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("Inside");

const DataPath k_GeneratedImageGeomPath = DataPath({Constants::k_ImageGeometry});
const DataPath k_GeneratedFeatureIdsPath = k_GeneratedImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
} // namespace

TEST_CASE("ComplexCore::RegularGridSampleSurfaceMeshFilter: Valid Filter Execution", "[ComplexCore][RegularGridSampleSurfaceMeshFilter]")
{
  // Read Exemplar DREAM3D File Filter
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_sample_surface_mesh/6_6_regular_grid_sample_surface_mesh.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{200, 200, 300}));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.1f, 0.1f, 0.1f}));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.0f, 0.0f, 0.0f}));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

// Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarImageGeomPath, ::k_GeneratedImageGeomPath);

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarFeatureIdsPath, ::k_GeneratedFeatureIdsPath);
}
