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
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <optional>
#include <vector>

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
  auto fidsStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellTupleShape, {1});
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
    auto fidsRefWriteResult = fidsRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(fidsRefWriteResult);
  }

  auto* cellFeatureAM = AttributeMatrix::Create(ds, Constants::k_CellFeatureData, {static_cast<usize>(k_NumOctantFeatures + 1)}, imageGeom->getId());

  // NumCells is small (9 tuples) — per-element writes are fine
  const DataPath numCellsPath = DataPath({k_GeomName, Constants::k_CellFeatureData, Constants::k_NumElements});
  auto numCellsStore = DataStoreUtilities::CreateDataStore<int32>(ds, numCellsPath, {static_cast<usize>(k_NumOctantFeatures + 1)}, {1});
  auto* numCells = DataArray<int32>::Create(ds, Constants::k_NumElements, numCellsStore, cellFeatureAM->getId());
  auto& numCellsRef = numCells->getDataStoreRef();
  std::vector<int32> localNumCells(featureCounts.begin(), featureCounts.end());
  auto numCellsRefWriteResult = numCellsRef.copyFromBuffer(0, nonstd::span<const int32>(localNumCells.data(), localNumCells.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(numCellsRefWriteResult);
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

namespace
{
const DataPath k_MetricGeometryPath({"Surface Metrics"});
const DataPath k_MetricCellPath = k_MetricGeometryPath.createChildPath("Cell Data");
const DataPath k_MetricFeaturePath = k_MetricGeometryPath.createChildPath("Feature Data");
const DataPath k_MetricIdsPath = k_MetricCellPath.createChildPath("Feature Ids");
const DataPath k_MetricCountsPath = k_MetricFeaturePath.createChildPath("Cell Counts");
const DataPath k_MetricRatioPath = k_MetricFeaturePath.createChildPath("Analytical Ratio");
const DataPath k_MetricSphericityPath = k_MetricFeaturePath.createChildPath("Analytical Sphericity");

Arguments BuildSurfaceMetricFixture(DataStructure& dataStructure, const FloatVec3& spacing, const std::vector<int32>& featureIds, const std::vector<int32>& counts, bool calculateSphericity)
{
  REQUIRE(featureIds.size() == 125);
  auto* imageGeom = ImageGeom::Create(dataStructure, k_MetricGeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({5, 5, 5});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeom->setSpacing(spacing);
  auto* cellData = AttributeMatrix::Create(dataStructure, k_MetricCellPath.getTargetName(), {5, 5, 5}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureData = AttributeMatrix::Create(dataStructure, k_MetricFeaturePath.getTargetName(), {counts.size()}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_MetricIdsPath, {5, 5, 5}, {1});
  auto countsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_MetricCountsPath, {counts.size()}, {1});
  REQUIRE(idsStore != nullptr);
  REQUIRE(countsStore != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, k_MetricIdsPath.getTargetName(), idsStore, cellData->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, k_MetricCountsPath.getTargetName(), countsStore, featureData->getId()) != nullptr);
  const auto idsWrite = idsStore->copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  const auto countsWrite = countsStore->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  auto args = ComputeSurfaceAreaToVolumeFilter().getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_MetricGeometryPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MetricIdsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_MetricCountsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_MetricRatioPath.getTargetName()));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_MetricSphericityPath.getTargetName()));
  return args;
}

template <typename T>
std::vector<T> ReadSurfaceMetricArray(DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const UnitTest::AlgorithmTestScope& scope)
{
  CAPTURE(path.toString());
  const DataArray<T>* array = nullptr;
  REQUIRE_NOTHROW(array = &dataStructure.getDataRefAs<DataArray<T>>(path));
  scope.requireExpectedStore(*array);
  REQUIRE(array->getTupleShape() == tupleShape);
  REQUIRE(array->getComponentShape() == ShapeType{1});
  REQUIRE(array->getDataType() == GetDataType<T>());
  const AttributeMatrix* parent = nullptr;
  REQUIRE_NOTHROW(parent = &dataStructure.getDataRefAs<AttributeMatrix>(path.getParent()));
  REQUIRE(array->getParentIds().size() == 1);
  REQUIRE(array->getParentIds().front() == parent->getId());
  const auto& store = array->getDataStoreRef();

  std::vector<T> values(array->getSize());
  const auto readResult = store.copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  return values;
}

void RequireSurfaceMetricGeometry(DataStructure& dataStructure, const ImageGeom& imageGeom, const FloatVec3& spacing, usize featureCount)
{
  const SizeVec3 dimensions{5, 5, 5};
  const FloatVec3 origin{0.0f, 0.0f, 0.0f};
  REQUIRE(dataStructure.getData(k_MetricGeometryPath) == &imageGeom);
  REQUIRE(imageGeom.getDimensions() == dimensions);
  REQUIRE(imageGeom.getSpacing() == spacing);
  REQUIRE(imageGeom.getOrigin() == origin);
  REQUIRE(imageGeom.getCellDataPath() == k_MetricCellPath);
  const AttributeMatrix* cellData = nullptr;
  const AttributeMatrix* featureData = nullptr;
  REQUIRE_NOTHROW(cellData = &dataStructure.getDataRefAs<AttributeMatrix>(k_MetricCellPath));
  REQUIRE_NOTHROW(featureData = &dataStructure.getDataRefAs<AttributeMatrix>(k_MetricFeaturePath));
  REQUIRE(imageGeom.getCellData() == cellData);
  const ShapeType cellShape{5, 5, 5};
  REQUIRE(cellData->getShape() == cellShape);
  REQUIRE(featureData->getShape() == ShapeType{featureCount});
  REQUIRE(cellData->getParentIds().size() == 1);
  REQUIRE(cellData->getParentIds().front() == imageGeom.getId());
  REQUIRE(featureData->getParentIds().size() == 1);
  REQUIRE(featureData->getParentIds().front() == imageGeom.getId());
}
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: analytical interior features", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool calculateSphericity = GENERATE(false, true);
  DYNAMIC_SECTION(scenario << ", sphericity=" << calculateSphericity)
  {
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure dataStructure;
    const FloatVec3 spacing{1.0f, 1.0f, 1.0f};
    std::vector<int32> featureIds(125, 1);
    // A positive enclosing feature separates the interior shapes from the image boundary.
    featureIds[31] = 2;
    featureIds[92] = 3;
    featureIds[93] = 3;
    const std::vector<int32> counts{0, 122, 1, 2};
    const auto args = BuildSurfaceMetricFixture(dataStructure, spacing, featureIds, counts, calculateSphericity);
    const ImageGeom* imageGeom = nullptr;
    REQUIRE_NOTHROW(imageGeom = &dataStructure.getDataRefAs<ImageGeom>(k_MetricGeometryPath));
    REQUIRE(imageGeom->getIndex(1.5f, 1.5f, 1.5f) == std::optional<usize>{31});
    REQUIRE(imageGeom->getIndex(2.5f, 3.5f, 3.5f) == std::optional<usize>{92});
    REQUIRE(imageGeom->getIndex(3.5f, 3.5f, 3.5f) == std::optional<usize>{93});
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricIdsPath, {5, 5, 5}, scope) == featureIds);
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricCountsPath, {4}, scope) == counts);

    ComputeSurfaceAreaToVolumeFilter filter;
    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(dataStructure.getData(k_MetricRatioPath) == nullptr);
    REQUIRE(dataStructure.getData(k_MetricSphericityPath) == nullptr);
    const auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto ratios = ReadSurfaceMetricArray<float32>(dataStructure, k_MetricRatioPath, {4}, scope);
    REQUIRE(ratios.size() == 4);
    REQUIRE(ratios[0] == 0.0f);
    // The enclosing feature has 16 internal faces; the filter omits its exterior faces.
    REQUIRE(std::abs(static_cast<float64>(ratios[1]) - 8.0 / 61.0) <= 1.0e-7);
    REQUIRE(ratios[2] == 6.0f);
    REQUIRE(ratios[3] == 5.0f);
    if(calculateSphericity)
    {
      const auto sphericities = ReadSurfaceMetricArray<float32>(dataStructure, k_MetricSphericityPath, {4}, scope);
      REQUIRE(sphericities.size() == 4);
      REQUIRE(sphericities[0] == 0.0f);
      const std::array<float64, 4> volumes{0.0, 122.0, 1.0, 2.0};
      const std::array<float64, 4> areas{0.0, 16.0, 6.0, 10.0};
      for(usize featureIdx = 1; featureIdx < volumes.size(); ++featureIdx)
      {
        CAPTURE(featureIdx);
        // The sphere of equal volume has area cbrt(36*pi*V*V).
        const float64 expectedSphericity = std::cbrt(36.0 * std::numbers::pi_v<float64> * volumes[featureIdx] * volumes[featureIdx]) / areas[featureIdx];
        // This tolerance permits float32 arithmetic and the algorithm's rounded fractional exponents.
        REQUIRE(std::abs(static_cast<float64>(sphericities[featureIdx]) - expectedSphericity) <= 5.0e-5 * expectedSphericity);
      }
    }
    else
    {
      REQUIRE(dataStructure.getData(k_MetricSphericityPath) == nullptr);
    }
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricIdsPath, {5, 5, 5}, scope) == featureIds);
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricCountsPath, {4}, scope) == counts);
    RequireSurfaceMetricGeometry(dataStructure, *imageGeom, spacing, counts.size());
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
