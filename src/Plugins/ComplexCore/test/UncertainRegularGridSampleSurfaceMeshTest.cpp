#include <catch2/catch.hpp>

#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/UncertainRegularGridSampleSurfaceMeshFilter.hpp"

#include <random>

using namespace complex;

namespace
{
const std::string k_TriGeomName = "STL-Cylinder";
const DataPath k_TriGeomPath = DataPath({k_TriGeomName});
const DataPath k_FaceLabelsPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath(Constants::k_FaceLabels);

const std::string k_ExemplarImageGeomName = "UncertainRegularGrid";
const DataPath k_ExemplarImageGeomPath = DataPath({k_ExemplarImageGeomName});
const DataPath k_ExemplarFeatureIdsPath = k_ExemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);

const DataPath k_GeneratedImageGeomPath = DataPath({Constants::k_ImageGeometry});
const DataPath k_GeneratedFeatureIdsPath = k_GeneratedImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
} // namespace

TEST_CASE("ComplexCore::UncertainRegularGridSampleSurfaceMeshFilter: Valid Filter Execution", "[ComplexCore][UncertainRegularGridSampleSurfaceMeshFilter]")
{
  /**
   * THe generated test case file for this was generated on a temporary modification of the 6.6 DREAM3D-SIMPL fork where the
   * random generation was modified to use the same generator, engine, and distribution used in standard SIMPLNX random generation
   * utilizing the standard library. It was seeded with the std::mt19937::default_seed.
   */
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_sample_surface_mesh.tar.gz", "6_6_sample_surface_mesh",
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_sample_surface_mesh/6_6_grid_sample_surface_mesh.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    UncertainRegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SeedValue_Key, std::make_any<uint64>(std::mt19937::default_seed));

    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{179, 18, 2}));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0f, 1.0f, 1.0f}));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.25f, 0.25f, 0.25f}));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.1f, 0.1f, 0.1f}));

    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

// Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_uncertain_regular_grid_sample_surface_mesh.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarImageGeomPath, ::k_GeneratedImageGeomPath);

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarFeatureIdsPath, ::k_GeneratedFeatureIdsPath);
}
