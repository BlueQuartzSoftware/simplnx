#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

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

constexpr usize k_DimX = 200;
constexpr usize k_DimY = 200;
constexpr usize k_DimZ = 200;
constexpr usize k_TotalVoxels = k_DimX * k_DimY * k_DimZ;
const std::string k_GeomName = "ImageGeom";

void BuildOctantFeatureIds(DataStructure& ds)
{
  const ShapeType cellTupleShape = {k_DimZ, k_DimY, k_DimX};

  auto* imageGeom = ImageGeom::Create(ds, k_GeomName);
  imageGeom->setDimensions({k_DimX, k_DimY, k_DimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({k_GeomName, Constants::k_CellData, Constants::k_FeatureIds});
  auto store = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(ds, Constants::k_FeatureIds, store, cellAM->getId());
  auto& fidsRef = featureIds->getDataStoreRef();
  const usize sliceSize = k_DimY * k_DimX;
  std::vector<int32> sliceBuffer(sliceSize);
  for(usize iz = 0; iz < k_DimZ; iz++)
  {
    for(usize iy = 0; iy < k_DimY; iy++)
    {
      for(usize ix = 0; ix < k_DimX; ix++)
      {
        const usize inSlice = iy * k_DimX + ix;
        const usize globalIdx = iz * sliceSize + inSlice;
        const int32 octant = static_cast<int32>((iz >= k_DimZ / 2 ? 4 : 0) + (iy >= k_DimY / 2 ? 2 : 0) + (ix >= k_DimX / 2 ? 1 : 0)) + 1;
        sliceBuffer[inSlice] = ((globalIdx * 7 + 13) % 100) < 5 ? 0 : octant;
      }
    }
    fidsRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
  }
}
} // namespace

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: Valid filter execution", "[ComputeBoundaryCellsFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_boundary_cells.tar.gz", "6_6_FindBoundaryCellsExemplar.dream3d");

  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_FindBoundaryCellsExemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Configure the filter arguments.
  ComputeBoundaryCellsFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>(k_ComputedBoundaryCellsName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<int8>(dataStructure, k_ExemplarBoundaryCellsPath, k_ComputedBoundaryCellsPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: Invalid filter execution", "[ComputeBoundaryCellsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_boundary_cells.tar.gz", "6_6_FindBoundaryCellsExemplar.dream3d");
  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_FindBoundaryCellsExemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry);
  imageGeom->setDimensions({250, 250, 250});
  imageGeom->setSpacing({1, 1, 1});
  imageGeom->setOrigin({0, 0, 0});

  const DataPath k_WrongGeometryPath({Constants::k_ImageGeometry});

  // Configure the filter arguments.
  ComputeBoundaryCellsFilter filter;
  Arguments args;

  // test mismatching geometry & featureId dimensions
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_WrongGeometryPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>(k_ComputedBoundaryCellsName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: Generate Large Test Dataset", "[SimplnxCore][ComputeBoundaryCellsFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  DataStructure buildDS;
  BuildOctantFeatureIds(buildDS);

  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data";
  fs::create_directories(outputDir);
  const auto outputFile = outputDir / "compute_boundary_cells_data.dream3d";
  UnitTest::WriteTestDataStructure(buildDS, outputFile);
}

TEST_CASE("SimplnxCore::ComputeBoundaryCellsFilter: 200x200x200 octant features", "[SimplnxCore][ComputeBoundaryCellsFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildOctantFeatureIds(dataStructure);

  const DataPath benchGeomPath({k_GeomName});
  const DataPath benchFeatureIdsPath = benchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));

  {
    ComputeBoundaryCellsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(benchGeomPath));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(benchFeatureIdsPath));
    args.insertOrAssign(ComputeBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>("BoundaryCells"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // BoundaryCells must contain varied values.
  const DataPath boundaryCellsPath = benchGeomPath.createChildPath(Constants::k_CellData).createChildPath("BoundaryCells");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int8>>(boundaryCellsPath));
  const auto& boundaryCells = dataStructure.getDataRefAs<DataArray<int8>>(boundaryCellsPath).getDataStoreRef();
  int8 minVal = boundaryCells[0];
  int8 maxVal = boundaryCells[0];
  for(usize i = 1; i < k_TotalVoxels; i++)
  {
    minVal = std::min(minVal, boundaryCells[i]);
    maxVal = std::max(maxVal, boundaryCells[i]);
  }
  REQUIRE(minVal == 0);
  REQUIRE(maxVal > 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
