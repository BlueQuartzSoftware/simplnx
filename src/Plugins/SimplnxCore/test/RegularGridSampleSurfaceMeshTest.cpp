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

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter:CreateImageGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "sample_surface_mesh_regular_grid_v1.tar.gz", "sample_surface_mesh_regular_grid_v1");
  auto baseDataFilePath = fs::path(fmt::format("{}/sample_surface_mesh_regular_grid_v1/sample_surface_mesh_regular_grid_v1.dream3d", unit_test::k_TestFilesDir));

  std::string sectionTitle = "Exemplar Image Geometry Face Labels Create";
  SECTION(sectionTitle)
  {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

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

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers int32 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

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
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers uint8 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (uint8)");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

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
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<uint8>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers float32 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (float32)");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

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
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter:ExistingImageGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "sample_surface_mesh_regular_grid_v1.tar.gz", "sample_surface_mesh_regular_grid_v1");
  auto baseDataFilePath = fs::path(fmt::format("{}/sample_surface_mesh_regular_grid_v1/sample_surface_mesh_regular_grid_v1.dream3d", unit_test::k_TestFilesDir));

  DataPath exemplarImageGeometryPath({"Exemplar Image Geometry Existing"});
  std::string sectionTitle = "FeatureIds (FaceLabels)";
  SECTION(sectionTitle)
  {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<int32>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "FeatureIds (PartNumber)";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber");
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<int32>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "FeatureIds (PartNumber uint8)";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (uint8)");
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<uint8>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
