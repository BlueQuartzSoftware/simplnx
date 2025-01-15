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
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Slice Geometry"});
const nx::core::DataPath k_ExemplarScanVectorsPath = DataPath({"Exemplar Scan Paths Geometry"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Exemplar Slice Geometry", "Edge Data", "Part Number"});
const nx::core::DataPath k_SliceIdsPath = DataPath({"Exemplar Slice Geometry", "Edge Data", "Slice Ids"});

const nx::core::DataPath k_ComputedScanVectorsPath = DataPath({"Output Scan Vectors"});

const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
// const DataObjectNameParameter::ValueType k_VertexData("VertexData");
const DataObjectNameParameter::ValueType k_RegionIdsName("RegionIds");
} // namespace

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Valid Filter Execution", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files.tar.gz",
                                                              "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateAMScanPathsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, std::make_any<float32>(0.14f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_StripeWidth_Key, std::make_any<float32>(7.0f));
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
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  std::cout << "Writing File: " << fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir) << "\n";
  WriteTestDataStructure(dataStructure, fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare the exemplar and the computed outputs
  {
    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(k_ExemplarScanVectorsPath);
    auto computedGeom = dataStructure.getDataAs<IGeometry>(k_ComputedScanVectorsPath);
    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  }

  {
    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath("EdgeData").createChildPath(k_SliceIdsPath.getTargetName());
    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_SliceIdsPath.getTargetName());
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  {
    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath("EdgeData").createChildPath("RegionIds");
    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }
}
