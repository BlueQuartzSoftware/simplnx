#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_SurfaceAreaVolumeRationArrayName("SurfaceAreaVolumeRatio");
const std::string k_SphericityArrayName("Sphericity");
const std::string k_SurfaceAreaVolumeRationArrayNameNX("SurfaceAreaVolumeRatioNX");
const std::string k_SphericityArrayNameNX("SphericityNX");
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolume", "[SimplnxCore][ComputeSurfaceAreaToVolume]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 25600, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_NumElementsArrayPath({k_DataContainer, k_CellFeatureData, k_NumElements});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_NumElementsArrayPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_SurfaceAreaVolumeRationArrayNameNX));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_SphericityArrayNameNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayName});
    DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayNameNX});
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    exemplarPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayName});
    calculatedPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayNameNX});
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_surface_area_volume_ratio.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolume: Benchmark 200x200x200", "[SimplnxCore][ComputeSurfaceAreaToVolume][Benchmark]")
{
  UnitTest::LoadPlugins();

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 160000, true);

  const auto benchmarkFile = fs::path(fmt::format("{}/compute_surface_area_to_volume_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  const std::string k_BenchGeomName = "ImageGeom";
  const DataPath k_BenchGeomPath({k_BenchGeomName});
  const DataPath k_BenchFeatureIdsPath = k_BenchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
  const DataPath k_BenchCellFeatureAMPath = k_BenchGeomPath.createChildPath(Constants::k_CellFeatureData);
  const DataPath k_BenchNumCellsPath = k_BenchCellFeatureAMPath.createChildPath(Constants::k_NumElements);

  constexpr int32 kNumFeatures = 8;

  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, k_BenchGeomName);
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, Constants::k_CellData, cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    auto* featureIds = UnitTest::CreateTestDataArray<int32>(buildDS, Constants::k_FeatureIds, cellTupleShape, {1}, cellAM->getId());
    auto& fidsStore = featureIds->getDataStoreRef();

    // Count voxels per feature to build NumCells
    std::vector<int32> featureCounts(kNumFeatures + 1, 0);
    for(usize i = 0; i < kTotalVoxels; i++)
    {
      const usize ix = i % kDimX;
      const usize iy = (i / kDimX) % kDimY;
      const usize iz = i / (kDimX * kDimY);
      const int32 octant = static_cast<int32>((iz >= kDimZ / 2 ? 4 : 0) + (iy >= kDimY / 2 ? 2 : 0) + (ix >= kDimX / 2 ? 1 : 0)) + 1;
      const int32 fid = ((i * 7 + 13) % 100) < 5 ? 0 : octant;
      fidsStore.setValue(i, fid);
      if(fid > 0)
      {
        featureCounts[fid]++;
      }
    }

    auto* cellFeatureAM = AttributeMatrix::Create(buildDS, Constants::k_CellFeatureData, {static_cast<usize>(kNumFeatures + 1)}, imageGeom->getId());
    auto* numCells = UnitTest::CreateTestDataArray<int32>(buildDS, Constants::k_NumElements, {static_cast<usize>(kNumFeatures + 1)}, {1}, cellFeatureAM->getId());
    auto& numCellsStore = numCells->getDataStoreRef();
    for(int32 f = 0; f <= kNumFeatures; f++)
    {
      numCellsStore.setValue(static_cast<usize>(f), featureCounts[f]);
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_BenchGeomPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_BenchFeatureIdsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_BenchNumCellsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>("SurfaceAreaVolumeRatio"));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>("Sphericity"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
