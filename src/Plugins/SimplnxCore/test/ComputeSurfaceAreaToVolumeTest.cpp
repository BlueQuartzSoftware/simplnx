#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

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

constexpr usize k_Dim = 200;
constexpr usize k_TotalVoxels = k_Dim * k_Dim * k_Dim;
constexpr int32 k_NumOctantFeatures = 8;
const std::string k_GeomName = "ImageGeom";

void BuildOctantWithNumCells(DataStructure& ds)
{
  const ShapeType cellTupleShape = {k_Dim, k_Dim, k_Dim};

  auto* imageGeom = ImageGeom::Create(ds, k_GeomName);
  imageGeom->setDimensions({k_Dim, k_Dim, k_Dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({k_GeomName, Constants::k_CellData, Constants::k_FeatureIds});
  auto fidsStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(ds, Constants::k_FeatureIds, fidsStore, cellAM->getId());
  auto& fidsRef = featureIds->getDataStoreRef();

  std::vector<int32> featureCounts(k_NumOctantFeatures + 1, 0);
  const usize sliceSize = k_Dim * k_Dim;
  std::vector<int32> sliceBuffer(sliceSize);
  for(usize iz = 0; iz < k_Dim; iz++)
  {
    for(usize iy = 0; iy < k_Dim; iy++)
    {
      for(usize ix = 0; ix < k_Dim; ix++)
      {
        const usize inSlice = iy * k_Dim + ix;
        const usize globalIdx = iz * sliceSize + inSlice;
        const int32 octant = static_cast<int32>((iz >= k_Dim / 2 ? 4 : 0) + (iy >= k_Dim / 2 ? 2 : 0) + (ix >= k_Dim / 2 ? 1 : 0)) + 1;
        const int32 fid = ((globalIdx * 7 + 13) % 100) < 5 ? 0 : octant;
        sliceBuffer[inSlice] = fid;
        if(fid > 0)
        {
          featureCounts[fid]++;
        }
      }
    }
    fidsRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
  }

  auto* cellFeatureAM = AttributeMatrix::Create(ds, Constants::k_CellFeatureData, {static_cast<usize>(k_NumOctantFeatures + 1)}, imageGeom->getId());

  // NumCells is small (9 tuples) — per-element writes are fine
  const DataPath numCellsPath = DataPath({k_GeomName, Constants::k_CellFeatureData, Constants::k_NumElements});
  auto numCellsStore = DataStoreUtilities::CreateDataStore<int32>(ds, numCellsPath, {static_cast<usize>(k_NumOctantFeatures + 1)}, {1}, IDataAction::Mode::Execute);
  auto* numCells = DataArray<int32>::Create(ds, Constants::k_NumElements, numCellsStore, cellFeatureAM->getId());
  auto& numCellsRef = numCells->getDataStoreRef();
  std::vector<int32> localNumCells(featureCounts.begin(), featureCounts.end());
  numCellsRef.copyFromBuffer(0, nonstd::span<const int32>(localNumCells.data(), localNumCells.size()));
}
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolume", "[SimplnxCore][ComputeSurfaceAreaToVolume]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Configure the filter arguments.
  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_NumElementsArrayPath({k_DataContainer, k_CellFeatureData, k_NumElements});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_NumElementsArrayPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_SurfaceAreaVolumeRationArrayNameNX));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_SphericityArrayNameNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayName});
    DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    exemplarPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayName});
    calculatedPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_surface_area_volume_ratio.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Generate Large Test Dataset", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  DataStructure buildDS;
  BuildOctantWithNumCells(buildDS);

  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data";
  fs::create_directories(outputDir);
  const auto outputFile = outputDir / "compute_surface_area_to_volume_data.dream3d";
  UnitTest::WriteTestDataStructure(buildDS, outputFile);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: 200x200x200 octant features", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildOctantWithNumCells(dataStructure);

  const DataPath benchGeomPath({k_GeomName});
  const DataPath benchFeatureIdsPath = benchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
  const DataPath benchCellFeatureAMPath = benchGeomPath.createChildPath(Constants::k_CellFeatureData);
  const DataPath benchNumCellsPath = benchCellFeatureAMPath.createChildPath(Constants::k_NumElements);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));

  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(benchGeomPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(benchFeatureIdsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(benchNumCellsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>("SurfaceAreaVolumeRatio"));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>("Sphericity"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: SA/V ratios and sphericity should be positive for all features
  const DataPath savPath = benchCellFeatureAMPath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = benchCellFeatureAMPath.createChildPath("Sphericity");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(savPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(sphericityPath));
  const auto& savRatio = dataStructure.getDataRefAs<DataArray<float32>>(savPath).getDataStoreRef();
  const auto& sphericity = dataStructure.getDataRefAs<DataArray<float32>>(sphericityPath).getDataStoreRef();
  for(int32 f = 1; f <= k_NumOctantFeatures; f++)
  {
    REQUIRE(savRatio[f] > 0.0f);
    REQUIRE(sphericity[f] > 0.0f);
    REQUIRE(sphericity[f] <= 1.0f);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
