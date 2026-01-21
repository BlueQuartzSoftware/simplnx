#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/SliceTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace slice_triangle_geometry_test
{
const nx::core::DataPath k_InputTriangleGeometryPath = DataPath({"Input Triangle Geometry"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Input Triangle Geometry", "FaceData", "Part Number"});
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Slice Geometry"});

const nx::core::DataPath k_ComputedEdgeGeometryPath = DataPath({"Output Edge Geometry"});
const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
const DataObjectNameParameter::ValueType k_SliceData("Slice Feature Data");
const DataObjectNameParameter::ValueType k_SliceIds("Slice Ids");
const DataObjectNameParameter::ValueType k_RegionIdsName("Part Number");
} // namespace slice_triangle_geometry_test

TEST_CASE("SimplnxCore::SliceTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  /// The test data set was reviewed manually by MAJ and found to be correct in output to the
  /// the best of our abilities. This is needed because DREAM3D-NX did not have
  /// this functionality and so we have nothing to compare against.

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files_v2.tar.gz",
                                                              "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  SliceTriangleGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zstart_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zend_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(true));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(slice_triangle_geometry_test::k_InputTriangleGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key, std::make_any<DataPath>(slice_triangle_geometry_test::k_RegionIdsPath));

  args.insertOrAssign(SliceTriangleGeometryFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(slice_triangle_geometry_test::k_ComputedEdgeGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(slice_triangle_geometry_test::k_EdgeData));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceIdArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(slice_triangle_geometry_test::k_SliceIds));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(slice_triangle_geometry_test::k_SliceData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  fs::path testFileOutputPath(fmt::format("{}/slice_triangle_geometry.dream3d", unit_test::k_BinaryTestOutputDir));
  std::cout << "Writing Output file: " << testFileOutputPath << std::endl;
  UnitTest::WriteTestDataStructure(dataStructure, testFileOutputPath);
#endif

  // Compare the exemplar and the computed outputs
  {
    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(slice_triangle_geometry_test::k_ExemplarEdgeGeometryPath);
    auto computedGeom = dataStructure.getDataAs<IGeometry>(slice_triangle_geometry_test::k_ComputedEdgeGeometryPath);
    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  }
  {
    DataPath exemplarDataArray =
        slice_triangle_geometry_test::k_ExemplarEdgeGeometryPath.createChildPath(slice_triangle_geometry_test::k_EdgeData).createChildPath(slice_triangle_geometry_test::k_SliceIds);
    DataPath computedDataArray =
        slice_triangle_geometry_test::k_ComputedEdgeGeometryPath.createChildPath(slice_triangle_geometry_test::k_EdgeData).createChildPath(slice_triangle_geometry_test::k_SliceIds);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  {
    DataPath exemplarDataArray =
        slice_triangle_geometry_test::k_ExemplarEdgeGeometryPath.createChildPath(slice_triangle_geometry_test::k_EdgeData).createChildPath(slice_triangle_geometry_test::k_RegionIdsName);
    DataPath computedDataArray =
        slice_triangle_geometry_test::k_ComputedEdgeGeometryPath.createChildPath(slice_triangle_geometry_test::k_EdgeData).createChildPath(slice_triangle_geometry_test::k_RegionIdsName);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
