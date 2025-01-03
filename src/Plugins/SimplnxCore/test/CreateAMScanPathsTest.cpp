#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CreateAMScanPathsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Edge Geometry"});
const nx::core::DataPath k_ExemplarScanVectorsPath = DataPath({"Exemplar Scan Vectors"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Exemplar Edge Geometry", "Edge Data", "Region Ids"});
const nx::core::DataPath k_SliceIdsPath = DataPath({"Exemplar Edge Geometry", "Edge Data", "Slice Ids"});

const nx::core::DataPath k_ComputedScanVectorsPath = DataPath({"Output Scan Vectors"});

const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
// const DataObjectNameParameter::ValueType k_VertexData("Vertex Data");
const DataObjectNameParameter::ValueType k_Times("Times");
const DataObjectNameParameter::ValueType k_Powers("Powers");
const DataObjectNameParameter::ValueType k_RegionIdsName("Region Ids");
} // namespace

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Valid Filter Execution", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "scan_path_test_data_v2.tar.gz", "scan_path_test_data");
  // auto baseDataFilePath = fs::path(fmt::format("{}/scan_path_test_data_v2/scan_path_test_data.dream3d", nx::core::unit_test::k_TestFilesDir));

  auto baseDataFilePath = fs::path("/Users/Shared/Data/7_create_am_scan_paths_test/create_am_scan_paths_test.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateAMScanPathsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateAMScanPathsFilter::k_StripeWidth_Key, std::make_any<float32>(7.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, std::make_any<float32>(67.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, std::make_any<DataPath>(k_ExemplarEdgeGeometryPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, std::make_any<DataPath>(k_SliceIdsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, std::make_any<DataPath>(k_RegionIdsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, std::make_any<DataPath>(k_ComputedScanVectorsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_VertexAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_VertexData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_RegionIdsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write out the .dream3d file now
#ifndef SIMPLNX_WRITE_TEST_OUTPUT
  std::cout << "Writing File: " << fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir) << "\n";
  WriteTestDataStructure(dataStructure, fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  //  // Compare the exemplar and the computed outputs
  //  {
  //    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(k_ExemplarScanVectorsPath);
  //    auto computedGeom = dataStructure.getDataAs<IGeometry>(k_ComputedScanVectorsPath);
  //    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  //  }
  //  {
  //    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath(k_VertexData).createChildPath(k_Times);
  //    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_VertexData).createChildPath(k_Times);
  //    UnitTest::CompareArrays<float64>(dataStructure, exemplarDataArray, computedDataArray);
  //  }
  //
  //  {
  //    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
  //    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
  //    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  //  }
  //  {
  //    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_Powers);
  //    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_Powers);
  //    UnitTest::CompareArrays<float32>(dataStructure, exemplarDataArray, computedDataArray);
  //  }
  //
  //  {
  //    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_SliceIdsPath.getTargetName());
  //    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_SliceIdsPath.getTargetName());
  //    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  //  }
}
