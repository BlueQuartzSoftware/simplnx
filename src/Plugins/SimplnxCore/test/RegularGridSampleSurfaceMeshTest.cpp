#include <catch2/catch.hpp>

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
const std::string k_TriGeomName = "Input Triangle Geometry";
const DataPath k_TriGeomPath = DataPath({k_TriGeomName});
const DataPath k_FaceLabelsPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath(Constants::k_FaceLabels);

const std::string k_ExemplarImageGeomName = "Exemplar Sample Triangle Geometry on Regular Grid";
const DataPath k_ExemplarImageGeomPath = DataPath({k_ExemplarImageGeomName});
const DataPath k_ExemplarFeatureIdsPath = k_ExemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);

const DataPath k_GeneratedImageGeomPath = DataPath({Constants::k_ImageGeometry});
const DataPath k_GeneratedCellDataPath = k_GeneratedImageGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_GeneratedFeatureIdsPath = k_GeneratedCellDataPath.createChildPath(Constants::k_FeatureIds);

const std::vector<uint64> k_Dims{171, 200, 150};
const std::vector<float32> k_Origin{1.0f, 1.99f, 0.0f};
const std::vector<float32> k_Spacing{0.1f, 0.1f, 0.02f};
} // namespace

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Valid Filter Execution", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  /**
   * THe generated test case file for this was generated on a temporary modification of the 6.6 DREAM3D-SIMPL fork where the
   * random generation was modified to use the same generator, engine, and distribution used in standard SIMPLNX random generation
   * utilizing the standard library. It was seeded with the std::mt19937::default_seed.
   */

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files.tar.gz",
                                                              "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarImageGeomPath, ::k_GeneratedImageGeomPath);

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarFeatureIdsPath, ::k_GeneratedFeatureIdsPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter::ExistingGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  /**
   * THe generated test case file for this was generated on a temporary modification of the 6.6 DREAM3D-SIMPL fork where the
   * random generation was modified to use the same generator, engine, and distribution used in standard SIMPLNX random generation
   * utilizing the standard library. It was seeded with the std::mt19937::default_seed.
   */

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files.tar.gz",
                                                              "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    CreateImageGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(CreateImageGeometryFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(k_GeneratedImageGeomPath));
    args.insertOrAssign(CreateImageGeometryFilter::k_Dimensions_Key, std::make_any<std::vector<uint64>>(k_Dims));
    args.insertOrAssign(CreateImageGeometryFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
    args.insertOrAssign(CreateImageGeometryFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
    args.insertOrAssign(CreateImageGeometryFilter::k_CellDataName_Key, std::make_any<std::string>(k_GeneratedCellDataPath.getTargetName()));

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarImageGeomPath, ::k_GeneratedImageGeomPath);

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarFeatureIdsPath, ::k_GeneratedFeatureIdsPath);
}
