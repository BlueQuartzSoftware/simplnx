#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include "SimplnxCore/Filters/ComputeBoundaryCellsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
const std::string k_ExemplarDataContainer = "SyntheticVolumeDataContainer";
const std::string k_ComputedBoundaryCellsName = "ComputedBoundaryCells";
const DataPath k_GeometryPath({k_ExemplarDataContainer});
const DataPath k_FeatureIdsPath({k_ExemplarDataContainer, Constants::k_CellData, Constants::k_FeatureIds});
const DataPath k_ExemplarBoundaryCellsPath({k_ExemplarDataContainer, Constants::k_CellData, "BoundaryCellsWithBoundary"});
const DataPath k_ComputedBoundaryCellsPath({k_ExemplarDataContainer, Constants::k_CellData, k_ComputedBoundaryCellsName});
} // namespace

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: Valid filter execution", "[ComputeBoundaryCellsFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 65536, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_boundary_cells.tar.gz", "6_6_FindBoundaryCellsExemplar.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_FindBoundaryCellsExemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeBoundaryCellsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>(k_ComputedBoundaryCellsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<int8>(dataStructure, k_ExemplarBoundaryCellsPath, k_ComputedBoundaryCellsPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: Invalid filter execution", "[ComputeBoundaryCellsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_boundary_cells.tar.gz", "6_6_FindBoundaryCellsExemplar.dream3d");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_FindBoundaryCellsExemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry);
  imageGeom->setDimensions({250, 250, 250});
  imageGeom->setSpacing({1, 1, 1});
  imageGeom->setOrigin({0, 0, 0});

  const DataPath k_WrongGeometryPath({Constants::k_ImageGeometry});

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeBoundaryCellsFilter filter;
  Arguments args;

  // test mismatching geometry & featureId dimensions
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_WrongGeometryPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>(k_ComputedBoundaryCellsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: Benchmark 200x200x200", "[ComputeBoundaryCellsFilter][Benchmark]")
{
  UnitTest::LoadPlugins();

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 160000, true);

  const auto benchmarkFile = fs::path(fmt::format("{}/compute_boundary_cells_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  const std::string k_BenchGeomName = "ImageGeom";
  const DataPath k_BenchGeomPath({k_BenchGeomName});
  const DataPath k_BenchFeatureIdsPath = k_BenchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);

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
    for(usize i = 0; i < kTotalVoxels; i++)
    {
      const usize ix = i % kDimX;
      const usize iy = (i / kDimX) % kDimY;
      const usize iz = i / (kDimX * kDimY);
      const int32 octant = static_cast<int32>((iz >= kDimZ / 2 ? 4 : 0) + (iy >= kDimY / 2 ? 2 : 0) + (ix >= kDimX / 2 ? 1 : 0)) + 1;
      fidsStore.setValue(i, ((i * 7 + 13) % 100) < 5 ? 0 : octant);
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    ComputeBoundaryCellsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_BenchGeomPath));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_BenchFeatureIdsPath));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>("BoundaryCells"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
