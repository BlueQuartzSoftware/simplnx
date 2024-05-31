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

namespace
{
const nx::core::DataPath k_InputTriangleGeometryPath = DataPath({"Exemplar Triangle Geometry"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Exemplar Triangle Geometry", "Face Data", "RegionIds"});
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Edge Geometry"});

const nx::core::DataPath k_ComputedEdgeGeometryPath = DataPath({"Output Edge Geometry"});
const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
const DataObjectNameParameter::ValueType k_SliceData("Slice Data");
const DataObjectNameParameter::ValueType k_SliceIds("Slice Ids");
const DataObjectNameParameter::ValueType k_RegionIdsName("RegionIds");
} // namespace

TEST_CASE("SimplnxCore::SliceTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_scan_path_test_data.tar.gz", "6_5_test_data_1");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_scan_path_test_data/6_6_scan_path_test_data.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  SliceTriangleGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zstart_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zend_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.33f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(true));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(k_InputTriangleGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key, std::make_any<DataPath>(k_RegionIdsPath));

  args.insertOrAssign(SliceTriangleGeometryFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(k_ComputedEdgeGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceIdArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_SliceIds));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_SliceData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/slice_triangle_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  // Compare the exemplar and the computed outputs
  {
    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(k_ExemplarEdgeGeometryPath);
    auto computedGeom = dataStructure.getDataAs<IGeometry>(k_ComputedEdgeGeometryPath);
    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  }
  {
    DataPath exemplarDataArray = k_ExemplarEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds);
    DataPath computedDataArray = k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  {
    DataPath exemplarDataArray = k_ExemplarEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    DataPath computedDataArray = k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }
}
