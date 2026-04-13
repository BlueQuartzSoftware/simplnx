#include "SimplnxCore/Filters/MapPointCloudToRegularGridFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_Existing = "[Existing]";
const std::string k_Masked = "[Mask]";
const std::string k_Computed = "[Computed]";

const DataPath k_ExistingImageGeomPath({k_ImageGeometry.str() + " " + k_Existing});
const DataPath k_VertexGeometryPath({k_PointCloudContainerName});
const DataPath k_VertexDataPath = k_VertexGeometryPath.createChildPath(k_VertexData);
const DataPath k_MaskPath = k_VertexDataPath.createChildPath(k_Mask);

const DataPath k_ManualImageGeomPathExemplar({k_ImageGeometry});
const DataPath k_ManualMaskImageGeomPathExemplar({k_ImageGeometry.str() + " " + k_Masked});

const DataPath k_ManualImageGeomPathComputed({k_ImageGeometry.str() + " " + k_Computed});
const DataPath k_ManualMaskImageGeomPathComputed({k_ImageGeometry.str() + " " + k_Masked + " " + k_Computed});

const DataPath k_VoxelIndicesManualExemplar = k_VertexDataPath.createChildPath(k_VoxelIndices);
const DataPath k_VoxelIndicesManualMaskExemplar = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Masked);
const DataPath k_VoxelIndicesExistingMaskExemplar = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Existing);

const DataPath k_VoxelIndicesManualComputed = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Computed);
const DataPath k_VoxelIndicesManualMaskComputed = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Masked + " " + k_Computed);
const DataPath k_VoxelIndicesExistingMaskComputed = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Existing + " " + k_Computed);
} // namespace

TEST_CASE("SimplnxCore::MapPointCloudToRegularGridFilter: Valid Filter Execution - Manual Geometry", "[MapPointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz", "6_6_map_point_cloud_to_regular_grid");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions = {5, 5, 10};

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_ManualImageGeomPathComputed));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndicesName_Key, std::make_any<std::string>(k_VoxelIndicesManualComputed.getTargetName()));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CellDataName_Key, std::make_any<std::string>(k_CellData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareImageGeometry(dataStructure, k_ManualImageGeomPathExemplar, k_ManualImageGeomPathComputed);
  UnitTest::CompareArrays<uint64>(dataStructure, k_VoxelIndicesManualExemplar, k_VoxelIndicesManualComputed);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MapPointCloudToRegularGridFilter: Valid Filter Execution - Manual Geometry with Mask", "[MapPointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz", "6_6_map_point_cloud_to_regular_grid");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions{11, 11, 26};

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_ManualMaskImageGeomPathComputed));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_InputMaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndicesName_Key, std::make_any<std::string>(k_VoxelIndicesManualMaskComputed.getTargetName()));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CellDataName_Key, std::make_any<std::string>(k_CellData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareImageGeometry(dataStructure, k_ManualMaskImageGeomPathExemplar, k_ManualMaskImageGeomPathComputed);
  UnitTest::CompareArrays<uint64>(dataStructure, k_VoxelIndicesManualMaskExemplar, k_VoxelIndicesManualMaskComputed);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MapPointCloudToRegularGridFilter: Valid Filter Execution - Existing Geometry with Mask", "[MapPointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz", "6_6_map_point_cloud_to_regular_grid");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 1;

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ExistingImageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_InputMaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndicesName_Key, std::make_any<std::string>(k_VoxelIndicesExistingMaskComputed.getTargetName()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<uint64>(dataStructure, k_VoxelIndicesExistingMaskExemplar, k_VoxelIndicesExistingMaskComputed);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MapPointCloudToRegularGridFilter: Invalid Filter Execution", "[MapPointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz", "6_6_map_point_cloud_to_regular_grid");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions{11, 11, 26};

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_ManualMaskImageGeomPathComputed));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndicesName_Key, std::make_any<std::string>(k_VoxelIndicesManualMaskComputed.getTargetName()));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CellDataName_Key, std::make_any<std::string>(k_CellData));

  SECTION("Invalid Grid Dimensions")
  {
    gridDimensions[2] = 0;
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_InputMaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  }
  SECTION("Mismatching mask & voxel indices array tuples")
  {
    const std::string invalidMask = "Invalid Mask Array";
    UnitTest::CreateTestDataArray<bool>(dataStructure, invalidMask, std::vector<usize>{100}, std::vector<usize>{1});
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_InputMaskPath_Key, std::make_any<DataPath>(DataPath({invalidMask})));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MapPointCloudToRegularGridFilter: SIMPL Backwards Compatibility", "[SimplnxCore][MapPointCloudToRegularGridFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "MapPointCloudToRegularGridFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<MapPointCloudToRegularGridFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key) == 0);
      CHECK(args.value<DataPath>(MapPointCloudToRegularGridFilter::k_SelectedImageGeometryPath_Key) == DataPath({"ImageDataContainer"}));
      CHECK(args.value<DataPath>(MapPointCloudToRegularGridFilter::k_SelectedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<bool>(MapPointCloudToRegularGridFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(MapPointCloudToRegularGridFilter::k_InputMaskPath_Key) == DataPath({"DataContainer", "VertexData", "Mask"}));
      CHECK(args.value<std::string>(MapPointCloudToRegularGridFilter::k_VoxelIndicesName_Key) == "VoxelIndices");
      CHECK(args.value<DataPath>(MapPointCloudToRegularGridFilter::k_CreatedImageGeometryPath_Key) == DataPath({"CreatedImageDataContainer"}));
    }
  }
}
